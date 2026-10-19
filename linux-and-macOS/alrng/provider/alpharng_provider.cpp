/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.
 */

/**
 *    @file alpharng_provider.cpp
 *
 *    @brief OpenSSL provider that makes RAND_bytes() return random bytes from an AlphaRNG device.
 *
 *    OpenSSL 4.0 removed engines, so an OpenSSL engine can no longer be used to supply random bytes. This
 *    provider does it instead. It implements the one provider function that OpenSSL calls for RAND_bytes() and
 *    RAND_priv_bytes() when a provider is named as the random source (OSSL_FUNC_PROVIDER_RANDOM_BYTES), and no
 *    cryptographic algorithms.
 *
 *    Use it by loading the provider and naming it as the random provider in the OpenSSL config file:
 *
 *        openssl_conf = openssl_init
 *        [openssl_init]
 *        random = random_sect
 *        providers = provider_sect
 *        [provider_sect]
 *        alpharng = alpharng_sect
 *        [alpharng_sect]
 *        module = /path/to/alpharng.so
 *        activate = 1
 *        [random_sect]
 *        random_provider = alpharng
 *
 *    Keep 'random' before 'providers' in [openssl_init]. With OpenSSL 4.0.3, listing 'providers' first makes the
 *    provider get unloaded again as soon as random_provider is read, and RAND_bytes() then uses OpenSSL's own source.
 *
 *    If the provider cannot start (for example when no AlphaRNG device is found), OpenSSL falls back to its
 *    built-in random source for RAND_bytes(). Applications that must use the device should check that the
 *    provider is loaded, for example with OSSL_PROVIDER_available().
 */
#include <climits>
#include <cstring>
#include <mutex>
#include <new>
#include <string>
#include <iostream>

#include <openssl/core.h>
#include <openssl/core_dispatch.h>
#include <openssl/core_names.h>
#include <openssl/crypto.h>
#include <openssl/rand.h>

#include "rng_backend.h"
#include "settings.h"

#ifndef OSSL_FUNC_PROVIDER_RANDOM_BYTES
#error "This OpenSSL has no provider random bytes support (OSSL_FUNC_PROVIDER_RANDOM_BYTES). Use OpenSSL 3.5.x with it, or 4.0 and later."
#endif

namespace {

const char c_provider_name[] = "AlphaRNG";
const char c_provider_version[] = "1.0";

struct ProviderContext {
	alrng_provider::RngBackend *backend {nullptr};
	std::mutex lock; // OpenSSL may ask for bytes from many threads, the device has one stream
	// A library context that has no random provider set. It answers the RAND_bytes() calls that the AlphaRNG API
	// makes for its own session keys and tokens while it is reading from the device (see alrng_random_bytes).
	OSSL_LIB_CTX *private_libctx {nullptr};
	std::string buildinfo; // names the settings in use, shown by 'openssl list -providers -verbose'
};

// True while this thread is inside the backend (opening the device or reading from it)
thread_local bool t_in_backend = false;

struct BackendScope {
	BackendScope() {t_in_backend = true;}
	~BackendScope() {t_in_backend = false;}
};

OSSL_FUNC_provider_teardown_fn alrng_teardown;
OSSL_FUNC_provider_gettable_params_fn alrng_gettable_params;
OSSL_FUNC_provider_get_params_fn alrng_get_params;
OSSL_FUNC_provider_query_operation_fn alrng_query_operation;
OSSL_FUNC_provider_random_bytes_fn alrng_random_bytes;

const OSSL_PARAM c_gettable_params[] = {
	OSSL_PARAM_utf8_ptr(OSSL_PROV_PARAM_NAME, nullptr, 0),
	OSSL_PARAM_utf8_ptr(OSSL_PROV_PARAM_VERSION, nullptr, 0),
	OSSL_PARAM_utf8_ptr(OSSL_PROV_PARAM_BUILDINFO, nullptr, 0),
	OSSL_PARAM_int(OSSL_PROV_PARAM_STATUS, nullptr),
	OSSL_PARAM_END
};

const OSSL_DISPATCH c_dispatch_table[] = {
	{OSSL_FUNC_PROVIDER_TEARDOWN, (void (*)(void))alrng_teardown},
	{OSSL_FUNC_PROVIDER_GETTABLE_PARAMS, (void (*)(void))alrng_gettable_params},
	{OSSL_FUNC_PROVIDER_GET_PARAMS, (void (*)(void))alrng_get_params},
	{OSSL_FUNC_PROVIDER_QUERY_OPERATION, (void (*)(void))alrng_query_operation},
	{OSSL_FUNC_PROVIDER_RANDOM_BYTES, (void (*)(void))alrng_random_bytes},
	{0, nullptr}
};

void alrng_teardown(void *provctx) {
	ProviderContext *ctx = static_cast<ProviderContext *>(provctx);
	if (ctx == nullptr) {
		return;
	}
	delete ctx->backend;
	if (ctx->private_libctx != nullptr) {
		OSSL_LIB_CTX_free(ctx->private_libctx);
	}
	delete ctx;
}

const OSSL_PARAM *alrng_gettable_params(void *) {
	return c_gettable_params;
}

int alrng_get_params(void *provctx, OSSL_PARAM params[]) {
	ProviderContext *ctx = static_cast<ProviderContext *>(provctx);
	if (ctx == nullptr) {
		return 0;
	}
	OSSL_PARAM *p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_NAME);
	if (p != nullptr && !OSSL_PARAM_set_utf8_ptr(p, c_provider_name)) {
		return 0;
	}
	p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_VERSION);
	if (p != nullptr && !OSSL_PARAM_set_utf8_ptr(p, c_provider_version)) {
		return 0;
	}
	p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_BUILDINFO);
	if (p != nullptr && !OSSL_PARAM_set_utf8_ptr(p, ctx->buildinfo.c_str())) {
		return 0;
	}
	p = OSSL_PARAM_locate(params, OSSL_PROV_PARAM_STATUS);
	if (p != nullptr && !OSSL_PARAM_set_int(p, 1)) {
		return 0;
	}
	return 1;
}

// This provider offers no algorithms: it only supplies random bytes
const OSSL_ALGORITHM *alrng_query_operation(void *, int, int *no_cache) {
	*no_cache = 0;
	return nullptr;
}

// Called by RAND_bytes() (which == OSSL_PROV_RANDOM_PUBLIC) and RAND_priv_bytes() (OSSL_PROV_RANDOM_PRIVATE).
// Both are served from the same hardware stream. The device output does not depend on the requested strength.
//
// The AlphaRNG API itself calls RAND_bytes() while it talks to the device (session keys, tokens, initialization
// vectors). With this provider set as the random provider, those calls come back here on the same thread. They are
// answered by OpenSSL's built-in generator through a library context that has no random provider, which is also the
// right source for session keys. Anything else is served from the device.
int alrng_random_bytes(void *provctx, int, void *buf, size_t n, unsigned int strength) {
	ProviderContext *ctx = static_cast<ProviderContext *>(provctx);
	if (ctx == nullptr || ctx->backend == nullptr || (buf == nullptr && n != 0)) {
		return 0;
	}
	if (n == 0) {
		return 1;
	}
	if (t_in_backend) {
		return RAND_bytes_ex(ctx->private_libctx, static_cast<unsigned char *>(buf), n, strength) == 1 ? 1 : 0;
	}
	std::lock_guard<std::mutex> guard(ctx->lock);
	BackendScope scope;
	return ctx->backend->read(static_cast<unsigned char *>(buf), n) ? 1 : 0;
}

} // namespace

extern "C" OSSL_provider_init_fn OSSL_provider_init;

extern "C" int OSSL_provider_init(const OSSL_CORE_HANDLE *handle, const OSSL_DISPATCH *in, const OSSL_DISPATCH **out,
		void **provctx) {
	// The settings are the entries of the provider section in openssl.cnf, which OpenSSL passes on as parameters
	OSSL_FUNC_core_get_params_fn *core_get_params = nullptr;
	for (; in != nullptr && in->function_id != 0; ++in) {
		if (in->function_id == OSSL_FUNC_CORE_GET_PARAMS) {
			core_get_params = OSSL_FUNC_core_get_params(in);
		}
	}
	std::string values[16]; // room for every setting
	auto lookup = [&](const char *name) -> const char * {
		char *value = nullptr;
		OSSL_PARAM params[2] = {OSSL_PARAM_construct_utf8_ptr(name, &value, 0), OSSL_PARAM_construct_end()};
		if (core_get_params == nullptr || core_get_params(handle, params) != 1 || value == nullptr) {
			return nullptr;
		}
		for (int i = 0; i < alrng_provider::c_setting_count; ++i) {
			if (std::string(name) == alrng_provider::c_setting_names[i]) {
				values[i] = value;
				return values[i].c_str();
			}
		}
		return nullptr;
	};
	alrng_provider::Settings settings;
	std::string error;
	if (!alrng_provider::parse_settings(lookup, settings, error)) {
		std::cerr << "AlphaRNG provider: " << error << std::endl;
		return 0;
	}

	ProviderContext *ctx = new (std::nothrow) ProviderContext;
	if (ctx == nullptr) {
		return 0;
	}
	ctx->buildinfo = "AlphaRNG random provider: " + settings.describe();
	ctx->private_libctx = OSSL_LIB_CTX_new();
	ctx->backend = alrng_provider::create_backend();
	if (ctx->private_libctx == nullptr || ctx->backend == nullptr) {
		alrng_teardown(ctx);
		return 0;
	}
	// Opening the device also makes RAND_bytes() calls (RSA key, session), so treat it like a read
	bool opened;
	{
		BackendScope scope;
		opened = ctx->backend->open(settings);
	}
	if (!opened) {
		alrng_teardown(ctx);
		return 0;
	}
	*provctx = ctx;
	*out = c_dispatch_table;
	return 1;
}
