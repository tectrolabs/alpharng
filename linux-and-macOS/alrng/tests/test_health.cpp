/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the HealthTests class (NIST SP 800-90B 'Repetition Count' and 'Adaptive Proportion' tests).
 The class reports an error only after more than 5 failures (the default threshold), so the tests use
 enough data to cross it. The status byte is 1 for a Repetition Count failure and 2 for an Adaptive
 Proportion failure.
 */

/**
 *    @file test_health.cpp
 *
 *    @brief Detection and non-detection tests for the entropy health tests.
 */
#include <HealthTests.h>
#include "test_framework.h"

using namespace alpharng;

// A fixed, repeatable stream of well mixed bytes (xorshift64)
static std::vector<uint8_t> pseudo_random_bytes(size_t size, uint64_t seed) {
	std::vector<uint8_t> out(size);
	uint64_t x = seed;
	for (size_t i = 0; i < size; ++i) {
		x ^= x << 13;
		x ^= x >> 7;
		x ^= x << 17;
		out[i] = (uint8_t)(x >> 24);
	}
	return out;
}

TEST(health_new_instance_reports_no_error) {
	HealthTests tests;
	CHECK(!tests.is_error());
	CHECK_EQ((int)tests.get_health_status(), 0);
	CHECK_EQ((int)tests.get_max_rct_failures(), 0);
	CHECK_EQ((int)tests.get_max_apt_failures(), 0);
}

TEST(health_random_data_passes) {
	HealthTests tests;
	std::vector<uint8_t> data = pseudo_random_bytes(200000, 0x9e3779b97f4a7c15ULL);
	tests.test(data.data(), (int)data.size());
	CHECK(!tests.is_error());
	CHECK_EQ((int)tests.get_health_status(), 0);
}

TEST(health_random_data_in_small_pieces_passes) {
	HealthTests tests;
	std::vector<uint8_t> data = pseudo_random_bytes(50000, 12345);
	for (size_t offset = 0; offset < data.size(); offset += 1000) {
		tests.test(data.data() + offset, 1000);
	}
	CHECK(!tests.is_error());
}

TEST(health_stuck_value_fails_repetition_count_test) {
	HealthTests tests;
	std::vector<uint8_t> data(200, 0x55);
	tests.test(data.data(), (int)data.size());
	CHECK(tests.is_error());
	CHECK_EQ((int)tests.get_health_status(), 1);
	CHECK(tests.get_max_rct_failures() > 5);
}

TEST(health_short_run_below_the_cutoff_passes) {
	HealthTests tests;
	// Runs of 4 equal bytes stay below the repetition cutoff of 5
	std::vector<uint8_t> data;
	for (int i = 0; i < 2000; ++i) {
		for (int j = 0; j < 4; ++j) {
			data.push_back((uint8_t)(i % 251));
		}
	}
	tests.test(data.data(), (int)data.size());
	CHECK_EQ((int)tests.get_max_rct_failures(), 0);
	CHECK(!tests.is_error());
}

TEST(health_overrepresented_value_fails_adaptive_proportion_test) {
	HealthTests tests;
	// 0xA5 0x3C 0xA5 0x3C ... never repeats a value back to back, but the first value of every
	// 64 sample window recurs about 32 times, far above the cutoff of 5
	std::vector<uint8_t> data;
	for (int i = 0; i < 1000; ++i) {
		data.push_back(i % 2 == 0 ? 0xA5 : 0x3C);
	}
	tests.test(data.data(), (int)data.size());
	CHECK(tests.is_error());
	CHECK_EQ((int)tests.get_health_status(), 2);
	CHECK_EQ((int)tests.get_max_rct_failures(), 0);
	CHECK(tests.get_max_apt_failures() > 5);
}

TEST(health_disabled_tests_ignore_bad_data) {
	HealthTests tests;
	tests.disable_tests();
	std::vector<uint8_t> data(500, 0x00);
	tests.test(data.data(), (int)data.size());
	CHECK(!tests.is_error());
	CHECK_EQ((int)tests.get_max_rct_failures(), 0);
}

TEST(health_reenabled_tests_detect_bad_data_again) {
	HealthTests tests;
	tests.disable_tests();
	tests.enable_tests();
	std::vector<uint8_t> data(200, 0x77);
	tests.test(data.data(), (int)data.size());
	CHECK(tests.is_error());
}

TEST(health_disabling_clears_an_earlier_error) {
	HealthTests tests;
	std::vector<uint8_t> data(200, 0x11);
	tests.test(data.data(), (int)data.size());
	REQUIRE(tests.is_error());
	tests.disable_tests();
	CHECK(!tests.is_error());
}

TEST(health_higher_failure_threshold_tolerates_more_failures) {
	HealthTests tests;
	tests.set_num_failures_threshold(200);
	// 200 equal bytes cause about 50 repetition failures, below the new threshold of 200
	std::vector<uint8_t> data(200, 0x42);
	tests.test(data.data(), (int)data.size());
	CHECK(!tests.is_error());
	CHECK(tests.get_max_rct_failures() > 0);
}

TEST(health_threshold_cannot_be_lowered_below_the_minimum) {
	HealthTests tests;
	tests.set_num_failures_threshold(1);
	// With the minimum threshold still in force, the stuck data is detected
	std::vector<uint8_t> data(200, 0x42);
	tests.test(data.data(), (int)data.size());
	CHECK(tests.is_error());
}

TEST(health_restart_clears_block_counters_but_keeps_a_detected_error) {
	HealthTests tests;
	std::vector<uint8_t> data(200, 0x33);
	tests.test(data.data(), (int)data.size());
	REQUIRE(tests.is_error());
	tests.restart();
	CHECK(tests.is_error());
}

TEST(health_failures_in_one_block_do_not_carry_into_the_next_after_restart) {
	HealthTests tests;
	std::vector<uint8_t> repeats(20, 0x66);
	tests.test(repeats.data(), (int)repeats.size());
	tests.restart();
	// 20 more bytes would add to the earlier failures if the counter were not reset
	tests.test(repeats.data(), (int)repeats.size());
	CHECK(!tests.is_error());
}
