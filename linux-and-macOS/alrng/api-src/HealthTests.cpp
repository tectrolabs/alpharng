/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements health tests 'Repetition Count' and 'Adaptive Proportion' according to NIST SP.800-90B

 */

/**
 *    @file HealthTests.cpp
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Implementation for 'Repetition Count' (RCT) and 'Adaptive Proportion' (APT) tests as described in NIST SP.800-90B
 */

#include "pch.h"
#include <HealthTests.h>

namespace alpharng {

HealthTests::HealthTests() {
	m_max_rct_failures_per_block = 0;
	m_max_apt_failures_per_block = 0;
	m_in_debug_mode = false;
	apt_initialize();
	rct_initialize();
}

/**
 * Retrieve the test health status.
 *
 * @return 0 if no test has failed, 2 - APT test error, 1 - RCT test error
 */
uint8_t HealthTests::get_health_status() {
	if (m_rct.status_byte != 0) {
		return m_rct.status_byte;
	}
	if (m_apt.status_byte != 0) {
		return m_apt.status_byte;
	}
	return 0;
}

/**
 * Check the test status.
 *
 * @return true if at least one test has failed, false otherwise
 */
bool HealthTests::is_error() {
	if (m_rct.status_byte != 0) {
		return true;
	}
	if (m_apt.status_byte != 0) {
		return true;
	}
	return false;
}

/**
 * Run an array of bytes trough the tests
 *
 * @param[in]  in points to an array of bytes to be tested
 * @param[in]  in_length amount of bytes to be tested
 */
void HealthTests::test(uint8_t *in, int in_length) {
	uint8_t value;
	for (int i = 0; i < in_length; ++i) {
		value = in[i];

		//
		// Run 'repetition count' test
		//
		if (!m_rct.is_initialized) {
			m_rct.is_initialized = true;
			m_rct.last_sample = value;
		} else {
			if (m_rct.last_sample == value) {
				m_rct.cur_repetitions++;
				if (m_rct.cur_repetitions >= m_rct.max_repetitions) {
					m_rct.cur_repetitions = 1;
					if (++m_rct.failure_count > c_num_failures_threshold) {
						if (m_rct.status_byte == 0) {
							m_rct.status_byte = m_rct.signature;
						}
					}

					if (m_rct.failure_count > m_max_rct_failures_per_block) {
						// Record the maximum failures per block for statistics
						m_max_rct_failures_per_block = m_rct.failure_count;
					}

					if (m_in_debug_mode) {
						if (m_rct.failure_count >= 1) {
							cerr << "rct.failureCount: " << (int)m_rct.failure_count << " value: " << (int)value << endl;
						}
					}
				}

			} else {
				m_rct.last_sample = value;
				m_rct.cur_repetitions = 1;
			}
		}

		//
		// Run 'adaptive proportion' test
		//
		if (!m_apt.is_initialized) {
			m_apt.is_initialized = true;
			m_apt.first_sample = value;
			m_apt.cur_repetitions = 0;
			m_apt.cur_samples = 0;
		} else {
			if (++m_apt.cur_samples >= m_apt.window_size) {
				m_apt.is_initialized = false;
				if (m_apt.cur_repetitions > m_apt.cutoff_value) {
					// Check to see if we have reached the failure threshold
					if (++m_apt.cycle_failures > c_num_failures_threshold) {
						if (m_apt.status_byte == 0) {
							m_apt.status_byte = m_apt.signature;
						}
					}
					if (m_apt.cycle_failures > m_max_apt_failures_per_block) {
						// Record the maximum failures per block for statistics
						m_max_apt_failures_per_block = m_apt.cycle_failures;
					}

					if (m_in_debug_mode) {
						if (m_apt.cycle_failures >= 1) {
							cerr << "apt.cycleFailures: " << (int)m_apt.cycle_failures << " value: " << (int)value << endl;
						}
					}
				}
			} else {
				if (m_apt.first_sample == value) {
					++m_apt.cur_repetitions;
				}
			}
		}
	}
}

void HealthTests::apt_initialize() {
	memset(&m_apt, 0x00, sizeof(m_apt));
	m_apt.status_byte = 0;
	m_apt.signature = 2;
	m_apt.window_size = 64;
	m_apt.cutoff_value = 5;
	apt_restart();
}

void HealthTests::apt_restart() {
	m_apt.is_initialized = false;
	m_apt.cycle_failures = 0;
}

void HealthTests::rct_initialize() {
	memset(&m_rct, 0x00, sizeof(m_rct));
	m_rct.status_byte = 0;
	m_rct.signature = 1;
	m_rct.max_repetitions = 5;
	rct_restart();
}

void HealthTests::rct_restart() {
	m_rct.is_initialized = false;
	m_rct.cur_repetitions = 1;
	m_rct.failure_count = 0;
}

/**
 * Restart all the test.
 * Should be called when testing a new round (block) of data.
 */
void HealthTests::restart() {
	rct_restart();
	apt_restart();
}

HealthTests::~HealthTests() {
}

} /* namespace alpharng */
