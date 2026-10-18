/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the RandomRangeSequence class, which generates a sequence of unique integers within a
 range. The entropy source is virtual, so the tests supply a repeatable pseudo random source (xorshift64)
 and a source that always fails. No device is needed.
 */

/**
 *    @file test_range_sequence.cpp
 *
 *    @brief Uniqueness, range, error handling and distribution tests for RandomRangeSequence.
 */
#include <algorithm>
#include <set>
#include <RandomRangeSequence.h>
#include "test_framework.h"

using namespace tl_algorithm;

// Entropy source with a fixed seed, so the results are repeatable
class SeededSequence : public RandomRangeSequence {
public:
	SeededSequence(int32_t min_limit, int32_t max_limit, uint64_t seed)
			: RandomRangeSequence(min_limit, max_limit), m_state(seed) {}

	bool get_entropy(int32_t *dest, const uint32_t size) override {
		for (uint32_t i = 0; i < size; ++i) {
			m_state ^= m_state << 13;
			m_state ^= m_state >> 7;
			m_state ^= m_state << 17;
			dest[i] = (int32_t)(m_state >> 16);
		}
		return true;
	}

private:
	uint64_t m_state;
};

// Entropy source that fails, like a device that cannot be read
class FailingSequence : public RandomRangeSequence {
public:
	FailingSequence(int32_t min_limit, int32_t max_limit) : RandomRangeSequence(min_limit, max_limit) {}
	bool get_entropy(int32_t *, const uint32_t) override { return false; }
};

static bool all_unique_and_within(const std::vector<int32_t> &values, int32_t min_value, int32_t max_value) {
	std::set<int32_t> seen(values.begin(), values.end());
	if (seen.size() != values.size()) {
		return false;
	}
	for (int32_t v : values) {
		if (v < min_value || v > max_value) {
			return false;
		}
	}
	return true;
}

TEST(sequence_covers_the_whole_range_exactly_once) {
	SeededSequence seq(1, 100, 42);
	std::vector<int32_t> out(100);
	REQUIRE(seq.generate_sequence(out.data(), 100));
	std::vector<int32_t> sorted = out;
	std::sort(sorted.begin(), sorted.end());
	for (int32_t i = 0; i < 100; ++i) {
		CHECK_EQ(sorted[(size_t)i], i + 1);
	}
}

TEST(sequence_is_shuffled_not_sorted) {
	SeededSequence seq(1, 100, 42);
	std::vector<int32_t> out(100);
	REQUIRE(seq.generate_sequence(out.data(), 100));
	CHECK(!std::is_sorted(out.begin(), out.end()));
}

TEST(sequence_partial_result_is_unique_and_in_range) {
	SeededSequence seq(1, 1000, 7);
	std::vector<int32_t> out(10);
	REQUIRE(seq.generate_sequence(out.data(), 10));
	CHECK(all_unique_and_within(out, 1, 1000));
}

TEST(sequence_works_with_negative_numbers) {
	SeededSequence seq(-5, 5, 99);
	std::vector<int32_t> out(11);
	REQUIRE(seq.generate_sequence(out.data(), 11));
	CHECK(all_unique_and_within(out, -5, 5));
	std::vector<int32_t> sorted = out;
	std::sort(sorted.begin(), sorted.end());
	for (int32_t i = 0; i < 11; ++i) {
		CHECK_EQ(sorted[(size_t)i], i - 5);
	}
}

TEST(sequence_single_number_range) {
	SeededSequence seq(7, 7, 1);
	int32_t out = 0;
	REQUIRE(seq.generate_sequence(&out, 1));
	CHECK_EQ(out, 7);
}

TEST(sequence_can_be_generated_repeatedly_with_one_instance) {
	SeededSequence seq(1, 50, 5);
	std::vector<int32_t> first(50);
	std::vector<int32_t> second(50);
	REQUIRE(seq.generate_sequence(first.data(), 50));
	REQUIRE(seq.generate_sequence(second.data(), 50));
	CHECK(all_unique_and_within(first, 1, 50));
	CHECK(all_unique_and_within(second, 1, 50));
	CHECK(first != second);
}

TEST(sequence_same_seed_gives_same_result) {
	SeededSequence a(1, 200, 1234);
	SeededSequence b(1, 200, 1234);
	std::vector<int32_t> out_a(200);
	std::vector<int32_t> out_b(200);
	REQUIRE(a.generate_sequence(out_a.data(), 200));
	REQUIRE(b.generate_sequence(out_b.data(), 200));
	CHECK(out_a == out_b);
}

TEST(sequence_different_seed_gives_different_result) {
	SeededSequence a(1, 200, 1234);
	SeededSequence b(1, 200, 4321);
	std::vector<int32_t> out_a(200);
	std::vector<int32_t> out_b(200);
	REQUIRE(a.generate_sequence(out_a.data(), 200));
	REQUIRE(b.generate_sequence(out_b.data(), 200));
	CHECK(out_a != out_b);
}

TEST(sequence_rejects_more_numbers_than_the_range_holds) {
	SeededSequence seq(1, 10, 3);
	std::vector<int32_t> out(20);
	CHECK(!seq.generate_sequence(out.data(), 11));
	CHECK(!seq.get_last_err_msg().empty());
}

TEST(sequence_rejects_a_zero_size_request) {
	SeededSequence seq(1, 10, 3);
	int32_t out = 0;
	CHECK(!seq.generate_sequence(&out, 0));
}

TEST(sequence_recovers_after_a_rejected_request) {
	SeededSequence seq(1, 10, 3);
	std::vector<int32_t> out(10);
	REQUIRE(!seq.generate_sequence(out.data(), 11));
	REQUIRE(seq.generate_sequence(out.data(), 10));
	CHECK(all_unique_and_within(out, 1, 10));
	CHECK(seq.get_last_err_msg().empty());
}

TEST(sequence_rejects_a_reversed_range) {
	SeededSequence seq(10, 1, 3);
	std::vector<int32_t> out(10);
	CHECK(!seq.generate_sequence(out.data(), 1));
}

TEST(sequence_rejects_a_range_below_the_allowed_minimum) {
	SeededSequence seq(INT32_MIN, 0, 3);
	std::vector<int32_t> out(1);
	CHECK(!seq.generate_sequence(out.data(), 1));
	CHECK(seq.get_last_err_msg().find("smallest") != std::string::npos);
}

TEST(sequence_reports_a_failing_entropy_source) {
	FailingSequence seq(1, 10);
	std::vector<int32_t> out(10);
	CHECK(!seq.generate_sequence(out.data(), 10));
}

TEST(sequence_every_value_is_equally_likely_in_first_place) {
	// Over many runs each of the 10 numbers should come first about 10% of the time
	const int runs = 5000;
	int first_place[11] = {0};
	SeededSequence seq(1, 10, 2024);
	for (int run = 0; run < runs; ++run) {
		int32_t out[10];
		REQUIRE(seq.generate_sequence(out, 10));
		first_place[out[0]]++;
	}
	for (int value = 1; value <= 10; ++value) {
		// Expected 500, standard deviation about 21: allow roughly five deviations
		CHECK(first_place[value] > 390 && first_place[value] < 610);
	}
}
