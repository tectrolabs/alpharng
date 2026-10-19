/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 The AlphaRNG API calls RAND_bytes() itself while it opens and reads the device (session keys, tokens). When the
 provider is the random provider, those calls come back to the provider on the same thread. They must not deadlock
 or take bytes from the device stream. The stand-in backend makes such calls when ALRNG_SIM_REENTER=1.
 */

/**
 *    @file test_provider_reentrant.cpp
 *
 *    @brief RAND_bytes() calls made by the backend itself do not deadlock and do not consume the device stream.
 */
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <thread>
#include <vector>

#include <openssl/crypto.h>
#include <openssl/provider.h>
#include <openssl/rand.h>

#include "provider_test_support.h"
#include "test_framework.h"

static bool s_configured = setenv("ALRNG_SIM_REENTER", "1", 1) == 0 && pts::use_config(true, true);

TEST(the_provider_loads_although_opening_the_device_calls_rand_bytes) {
	REQUIRE(s_configured);
	REQUIRE(OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, nullptr) == 1);
	CHECK_EQ(OSSL_PROVIDER_available(nullptr, "alpharng"), 1);
}

TEST(reads_that_call_rand_bytes_return_exactly_the_device_stream) {
	alrng_provider::SimStream expected;
	for (int i = 0; i < 20; ++i) {
		std::vector<unsigned char> got(40, 0), want(40, 0);
		expected.fill(want.data(), want.size());
		REQUIRE(RAND_bytes(got.data(), static_cast<int>(got.size())) == 1);
		CHECK(got == want);
	}
}

TEST(concurrent_reads_that_call_rand_bytes_complete_and_use_each_word_once) {
	const int threads = 4, requests = 50;
	const size_t words_per_request = 8;
	std::vector<std::vector<uint64_t>> received(threads);
	std::atomic<int> failures{0};
	std::vector<std::thread> workers;
	for (int t = 0; t < threads; ++t) {
		workers.emplace_back([&, t]() {
			std::vector<unsigned char> buffer(words_per_request * 8);
			for (int r = 0; r < requests; ++r) {
				if (RAND_bytes(buffer.data(), static_cast<int>(buffer.size())) != 1) {
					failures++;
					return;
				}
				for (size_t w = 0; w < words_per_request; ++w) {
					uint64_t word = 0;
					for (int b = 7; b >= 0; --b) {
						word = (word << 8) | buffer[w * 8 + static_cast<size_t>(b)];
					}
					received[static_cast<size_t>(t)].push_back(word);
				}
			}
		});
	}
	for (std::thread &worker : workers) {
		worker.join();
	}
	CHECK_EQ(failures.load(), 0);

	std::vector<uint64_t> all;
	for (const auto &list : received) {
		all.insert(all.end(), list.begin(), list.end());
	}
	std::sort(all.begin(), all.end());

	// The first 20 requests of the previous test (20 * 40 bytes = 100 words) already took the start of the stream
	alrng_provider::SimStream stream;
	for (int i = 0; i < 100; ++i) {
		stream.next_word();
	}
	std::vector<uint64_t> expected;
	for (size_t i = 0; i < all.size(); ++i) {
		expected.push_back(stream.next_word());
	}
	std::sort(expected.begin(), expected.end());
	CHECK(all == expected);
}
