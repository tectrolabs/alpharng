/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

/**
 *    @file rng_backend_sim.cpp
 *
 *    @brief Stand-in provider backend for tests: a repeatable byte stream and no device.
 *
 *    Two environment variables make it fail on purpose, to test the error paths:
 *      ALRNG_SIM_OPEN_FAIL=1   open() fails, so the provider does not load
 *      ALRNG_SIM_READ_FAIL=1   every read() fails
 *    One makes it call RAND_bytes() while opening and reading, as the real AlphaRNG API does for its session keys and
 *    tokens, to test that the provider does not deadlock or recurse when it is the random provider:
 *      ALRNG_SIM_REENTER=1
 *    And one prints the size of every read to stderr, to see what OpenSSL asks for:
 *      ALRNG_SIM_LOG=1
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

#include <openssl/rand.h>

#include "sim_stream.h"
#include "rng_backend.h"

namespace alrng_provider {

namespace {

bool env_is_set(const char *name) {
	const char *value = std::getenv(name);
	return value != nullptr && std::strcmp(value, "1") == 0;
}

// Stands for the RAND_bytes() calls the real API makes while it works with the device
bool reenter() {
	if (!env_is_set("ALRNG_SIM_REENTER")) {
		return true;
	}
	unsigned char scratch[32];
	return RAND_bytes(scratch, sizeof(scratch)) == 1;
}

class SimBackend : public RngBackend {
public:
	bool open(const Settings &) override {
		return !env_is_set("ALRNG_SIM_OPEN_FAIL") && reenter();
	}

	bool read(unsigned char *buffer, size_t size) override {
		if (env_is_set("ALRNG_SIM_LOG")) {
			std::fprintf(stderr, "sim backend: read of %zu bytes\n", size);
		}
		if (env_is_set("ALRNG_SIM_READ_FAIL")) {
			return false;
		}
		if (!reenter()) {
			return false;
		}
		m_stream.fill(buffer, size);
		return true;
	}

private:
	SimStream m_stream;
};

} // namespace

RngBackend *create_backend() {
	return new (std::nothrow) SimBackend;
}

} // namespace alrng_provider
