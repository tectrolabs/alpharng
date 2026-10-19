/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 A setting with a value that is not allowed keeps the provider from loading, so the device is never used with
 settings other than the ones asked for. OpenSSL then keeps its own random source and the provider prints why.
 */

/**
 *    @file test_provider_bad_setting.cpp
 *
 *    @brief A misspelled setting value makes the provider unavailable instead of being ignored.
 */
#include <vector>

#include <openssl/crypto.h>
#include <openssl/provider.h>
#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

static bool s_configured = pts::use_config(true, true, "cipher = aes512\n");

TEST(the_provider_does_not_load) {
	REQUIRE(s_configured);
	REQUIRE(OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr) == 1);
	CHECK_EQ(OSSL_PROVIDER_available(nullptr, "alpharng"), 0);
}

TEST(random_bytes_come_from_the_default_source) {
	std::vector<unsigned char> got(64, 0);
	std::vector<unsigned char> stream(64, 0);
	alrng_provider::SimStream expected;
	expected.fill(stream.data(), stream.size());
	REQUIRE(RAND_bytes(got.data(), static_cast<int>(got.size())) == 1);
	CHECK(got != stream);
}
