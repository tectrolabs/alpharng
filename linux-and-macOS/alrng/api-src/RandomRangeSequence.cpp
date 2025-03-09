/**
 Copyright (C) 2014-2025 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements an algorithm for generating randomized sequence of integers within a range. Such sequence does not contain duplicates.

 */

/**
 *    @file RandomRangeSequence.cpp
 *    @date 03/09/2025
 *    @Author: Andrian Belinski
 *    @version 1.1
 *
 *    @brief Implements a class with an algorithm for generating up to 4294967295 unique sequence of integers within [-2147483647,2147483647] range.
 */

#include <RandomRangeSequence.h>

namespace tl_algorithm {

/**
 * Validate minimum and maximum limits. Allocate memory for arrays.
 *
 * @param int32_t min_limit - the smallest number in the range
 * @param int32_t max_limit - the largest number in the range
 */
RandomRangeSequence::RandomRangeSequence(const int32_t min_limit, const int32_t max_limit)
		: c_min_limit(min_limit) {
	if (min_limit < c_min_range_value) {
		m_error_log_oss << "The smallest number in the range cannot be smaller than " << c_min_range_value << std::endl;
		return;
	}

	if (max_limit > c_max_range_value) {
		m_error_log_oss << "The largest number in the range cannot be bigger than " << c_max_range_value << std::endl;
		return;
	}

	if (min_limit > max_limit) {
		std::cerr << "The largest number in the range cannot be smaller than the smallest number" << std::endl;
		return;
	}

	c_actual_range = (uint32_t)std::llabs(((int64_t)max_limit - (int64_t)min_limit)) + 1;
	if (c_actual_range > c_max_sequences) {
		m_error_log_oss << "The range provided exceeds the " << c_max_sequences << " numbers in a sequence" << std::endl;
		return;
	}

	m_number_buffer_1 = new (std::nothrow) int32_t[c_actual_range];
	if (m_number_buffer_1 == nullptr) {
		m_error_log_oss << "Cannot allocate memory for buffer 1" << std::endl;
		return;
	}

	m_number_buffer_2 = new (std::nothrow) int32_t[c_actual_range];
	if (m_number_buffer_2 == nullptr) {
		m_error_log_oss << "Cannot allocate memory for buffer 2" << std::endl;
		return;
	}

	mn_random_buffer = new (std::nothrow) int32_t[c_actual_range + 1];
	if (mn_random_buffer == nullptr) {
		m_error_log_oss << "Cannot allocate memory for random buffer" << std::endl;
		return;
	}

	m_is_error = false;
}

RandomRangeSequence::~RandomRangeSequence() {
	if (mn_random_buffer != nullptr) {
		delete [] mn_random_buffer;
	}

	if (m_number_buffer_2 != nullptr) {
		delete [] m_number_buffer_2;
	}

	if (m_number_buffer_1 != nullptr) {
		delete [] m_number_buffer_1;
	}
}

/**
 *
 * Remove selected integers (marked as -1) from the buffer
 * and leave only those that have not been pulled out.
 *
 */
void RandomRangeSequence::defragment() {
	uint32_t new_cur_num_buffer_size = 0;
	for (uint32_t i = 0; i < m_current_number_buffer_size; i++) {
		if (m_current_number_buffer[i] != -1) {
			m_other_current_number_buffer[new_cur_num_buffer_size++] = m_current_number_buffer[i];
		}
	}
	m_current_number_buffer_size = new_cur_num_buffer_size;
	int32_t *swap = m_current_number_buffer;
	m_current_number_buffer = m_other_current_number_buffer;
	m_other_current_number_buffer = swap;
}

/**
 * Generate relative random integers and mark positions for those that have been extracted with -1 to
 * prevent duplicates.
 *
 * @param uint32_t *dest - destination buffer
 * @param uint32_t size - how many integers to generate within the range
 */
void RandomRangeSequence::iterate(uint32_t *dest, const uint32_t size) {
	for (uint32_t i = 0; i < size && m_dest_idx < size; i++) {
		uint32_t idx = mn_random_buffer[i] % m_current_number_buffer_size;
		if (m_current_number_buffer[idx] != -1) {
			dest[m_dest_idx++] = m_current_number_buffer[idx];
			m_current_number_buffer[idx] = -1;
		}
	}
}

/**
 * Initialize buffers and variables
 *
 */
void RandomRangeSequence::init() {
	m_current_number_buffer = m_number_buffer_1;
	m_other_current_number_buffer = m_number_buffer_2;
	for (uint32_t i = 0; i < c_actual_range; i++) {
		m_current_number_buffer[i] = i + 1;
	}
	m_current_number_buffer_size = c_actual_range;
	m_dest_idx = 0;
}

void RandomRangeSequence::clear_error_log() {
	m_error_log_oss.str("");
	m_error_log_oss.clear();
}

/**
 * Generate a sequence of random numbers within the range, up to the specified limit `size`
 *
 * @param int32_t *dest - destination memory
 * @param uint32_t size - how many integers to generate within the range
 * @return bool - true when successfully generated
 */
bool RandomRangeSequence::generate_sequence(int32_t *dest, uint32_t size) {
	if (m_is_error) {
		// Unsuccessful object initialization
		return false;
	}
	clear_error_log();

	if (size > c_actual_range || size == 0) {
		m_error_log_oss << "Amount of integers requested " << size << " cannot exceed " << c_actual_range << std::endl;
		return false;
	}

	init();
	while(m_current_number_buffer_size > 0 && m_dest_idx < size) {
		if (false == get_entropy(mn_random_buffer, size)) {
			return false;
		}
		iterate((uint32_t*)dest, size);
		defragment();
	}

	// Transform relative sequence numbers into absolute values
	for (uint32_t i = 0; i < size; i++) {
		dest[i] = dest[i] - 1 + c_min_limit;
	}

	return true;
}


} /* namespace tl_algorithm */
