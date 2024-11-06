/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements an algorithm for generating randomized sequence of integers within a range, based on true random bytes
 produced by a SwiftRNG device. Such sequence does not contain duplicates.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
 * RandomRangeSequence.cpp
 * @date 11/2/2024
 * @version 1.0
 *
 * @brief A class for generating random sequences of unique integers based on true random bytes
 * produced by a SwiftRNG device.
 *
 */
#include <AlphaRandomRangeSequence.h>

namespace alpharng {

/**
 *
 * @param int32_t min_limit - the smallest number in the range
 * @param int32_t max_limit - the largest number in the range
 */
AlphaRandomRangeSequence::AlphaRandomRangeSequence(AlphaRngApi *api, const int32_t min_limit, const int32_t max_limit)
		: RandomRangeSequence(min_limit, max_limit), m_api(api) {
}

/**
 * Implementing a method for retrieving entropy from AlphaRNG device
 *
 * @param int32_t *dest - destination memory
 * @param uint32_t size - how many numbers of entropy to retrieve
 * @return bool - true when entropy successfully retrieved
 *
 */
bool AlphaRandomRangeSequence::get_entropy(int32_t *dest, const uint32_t size) {
	return m_api->get_entropy((uint8_t*)dest, size * 4);
}

AlphaRandomRangeSequence::~AlphaRandomRangeSequence() {
}

} /* namespace alpharng */
