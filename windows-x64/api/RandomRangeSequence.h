/**
 Copyright (C) 2014-2025 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements an algorithm for generating randomized sequence of integers within a range. Such sequence does not contain duplicates.

 */

/**
 *    @file RandomRangeSequence.h
 *    @date 03/09/2025
 *    @Author: Andrian Belinski
 *    @version 1.1
 *
 *    @brief Implements a class with an algorithm for generating up to 4294967295 unique sequence of integers within [-2147483647,2147483647] range.
 */
#ifndef TL_RANDOMRANGESEQUENCE_H_
#define TL_RANDOMRANGESEQUENCE_H_

#include <cstdint>
#include <sstream>
#include <iostream>
#include <cmath>


namespace tl_algorithm {

class RandomRangeSequence {
public:
	bool generate_sequence(int32_t *dest, uint32_t size);
	std::string get_last_err_msg() const {return m_error_log_oss.str();}
	virtual bool get_entropy(int32_t *dest, const uint32_t size) = 0;

	RandomRangeSequence(const int32_t min_limit, const int32_t max_limit);
	virtual ~RandomRangeSequence();

private:
	void init();
	void iterate(uint32_t *dest, uint32_t size);
	void defragment();
	void clear_error_log();

private:
	// Smallest possible value in the randomized sequence.
	const int32_t c_min_range_value {-2147483647};

	// Largest possible value in the randomized sequence
	const int32_t c_max_range_value  {2147483647};

	// Maximum amount of numbers that can be generated in the sequence
	const uint32_t c_max_sequences   {4294967295};

	// Smallest value in the randomized sequence
	const int32_t c_min_limit;

	std::ostringstream m_error_log_oss;
	uint32_t m_dest_idx {0};
	uint32_t c_actual_range {0};
	bool m_is_error {true};
	int32_t *m_number_buffer_1 {nullptr};
	int32_t *m_number_buffer_2 {nullptr};
	int32_t *mn_random_buffer {nullptr};
	int32_t *m_current_number_buffer {nullptr};
	int32_t *m_other_current_number_buffer {nullptr};
	uint32_t m_current_number_buffer_size {0};
};

} /* namespace tl_algorithm */

#endif /* TL_RANDOMRANGESEQUENCE_H_ */
