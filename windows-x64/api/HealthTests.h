/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements health tests 'Repetition Count' and 'Adaptive Proportion' according to NIST SP.800-90B

 The health tests are used for inspecting entropy random bytes retrieved form the AlphaRNG device.

 */

/**
 *    @file HealthTests.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Implementation for 'Repetition Count' and 'Adaptive Proportion' tests as described in NIST SP.800-90B
 */

#ifndef COUNTINUOUSHEALTHTEST_H_
#define COUNTINUOUSHEALTHTEST_H_

#include <cstring>
#include <cstdint>
#include <iostream>
#include <Structures.h>

using namespace std;
namespace alpharng {

class HealthTests {
public:
	void test(uint8_t *in, int in_len);
	void restart();
	bool is_error();
	uint8_t get_health_status();
	uint16_t get_max_rct_failures() {return m_max_rct_failures_per_block;}
	uint16_t get_max_apt_failures() {return m_max_apt_failures_per_block;}

	HealthTests();
	virtual ~HealthTests();
private:
	void rct_restart();
	void apt_restart();
	void apt_initialize();
	void rct_initialize();
private:
	AptData m_apt;
	RctData m_rct;
	const uint8_t c_num_failures_threshold = 5;
	uint16_t m_max_rct_failures_per_block;
	uint16_t m_max_apt_failures_per_block;
	bool m_in_debug_mode;
};

} /* namespace alpharng */

#endif /* COUNTINUOUSHEALTHTEST_H_ */
