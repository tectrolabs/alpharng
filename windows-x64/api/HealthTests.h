/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements health tests 'Repetition Count' and 'Adaptive Proportion' according to NIST SP.800-90B

 The health tests are used for inspecting entropy random bytes retrieved from the AlphaRNG device.

 */

/**
 *    @file HealthTests.h
 *    @date 11/17/2023
 *    @Author: Andrian Belinski
 *    @version 1.4
 *
 *    @brief Implementation for 'Repetition Count' and 'Adaptive Proportion' tests as described in NIST SP.800-90B
 */

#ifndef ALPHARNG_COUNTINUOUSHEALTHTEST_H_
#define ALPHARNG_COUNTINUOUSHEALTHTEST_H_

#include <cstring>
#include <cstdint>
#include <iostream>
#include <Structures.h>

namespace alpharng {

class HealthTests {
public:
	void test(const uint8_t *in, int in_length);
	void restart();
	bool is_error() const;
	void enable_tests();
	void disable_tests();
	void set_num_failures_threshold(uint8_t num_failures_threshold);
	uint8_t get_health_status() const;
	uint16_t get_max_rct_failures() const {return m_max_rct_failures_per_block;}
	uint16_t get_max_apt_failures() const {return m_max_apt_failures_per_block;}

	HealthTests();
	virtual ~HealthTests() = default;
private:
	void rct_restart();
	void apt_restart();
	void apt_initialize();
	void rct_initialize();
public:
	static const uint8_t s_min_num_failures_threshold {5};
private:
	AptData m_apt;
	RctData m_rct;
	uint8_t m_num_failures_threshold {s_min_num_failures_threshold};
	uint16_t m_max_rct_failures_per_block {0};
	uint16_t m_max_apt_failures_per_block {0};
	bool m_in_debug_mode {false};
	bool m_tests_enabled {true};
};

} /* namespace alpharng */

#endif /* ALPHARNG_COUNTINUOUSHEALTHTEST_H_ */
