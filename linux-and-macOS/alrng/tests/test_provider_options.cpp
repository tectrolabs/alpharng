/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 The settings written in the provider section of openssl.cnf reach the provider.
 */

/**
 *    @file test_provider_options.cpp
 *
 *    @brief Settings in the provider section of openssl.cnf are read by the provider and shown in its build info.
 */
#include <string>

#include <openssl/crypto.h>
#include <openssl/core_names.h>
#include <openssl/provider.h>
#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

static bool s_configured = pts::use_config(true, true,
		"cipher = aes128\nmac = hmacSha256\nrsa = RSA4096\nkey_file = /some/pub.pem\ndevice = 3\ndata = sha512\nstat_tests = off\n");

static std::string build_info() {
	OSSL_PROVIDER *provider = OSSL_PROVIDER_load(nullptr, "alpharng");
	if (provider == nullptr) {
		return "";
	}
	const char *info = nullptr;
	OSSL_PARAM params[2] = {OSSL_PARAM_construct_utf8_ptr(OSSL_PROV_PARAM_BUILDINFO, const_cast<char **>(&info), 0),
			OSSL_PARAM_construct_end()};
	std::string result;
	if (OSSL_PROVIDER_get_params(provider, params) == 1 && info != nullptr) {
		result = info;
	}
	return result;
}

TEST(the_provider_loads_with_all_settings_given) {
	REQUIRE(s_configured);
	REQUIRE(OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr) == 1);
	CHECK_EQ(OSSL_PROVIDER_available(nullptr, "alpharng"), 1);
}

TEST(the_build_info_shows_the_settings_in_use) {
	CHECK(build_info() == "AlphaRNG random provider: cipher=aes128 mac=hmacSha256 rsa=key_file key_file=/some/pub.pem "
			"device=3 data=sha512 stat_tests=off");
}

TEST(random_bytes_still_come_from_the_provider) {
	alrng_provider::SimStream expected;
	unsigned char want[32], got[32];
	expected.fill(want, sizeof(want));
	REQUIRE(RAND_bytes(got, sizeof(got)) == 1);
	CHECK(std::string(reinterpret_cast<char *>(got), 32) == std::string(reinterpret_cast<char *>(want), 32));
}
