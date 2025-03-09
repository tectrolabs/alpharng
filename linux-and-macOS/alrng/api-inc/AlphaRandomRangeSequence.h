/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

 Copyright (C) 2014-2025 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements an algorithm for generating randomized sequence of integers within a range, based on true random bytes
 produced by an AlphaRNG device. Such sequence does not contain duplicates.

 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/*
 * RandomRangeSequence.h
 * @date 03/09/2025
 * @version 1.1
 *
 * @brief A class for generating random sequences of unique integers based on true random bytes
 * produced by an AlphaRNG device.
 *
 */
#ifndef ALPHA_RANDOMRANGESEQUENCE_H_
#define ALPHA_RANDOMRANGESEQUENCE_H_

#include <RandomRangeSequence.h>
#include <AlphaRngApi.h>
#include <cstdint>
#include <sstream>
#include <iostream>


namespace alpharng {

class AlphaRandomRangeSequence : public tl_algorithm::RandomRangeSequence {
public:
	AlphaRandomRangeSequence(AlphaRngApi *api, const int32_t min_limit, const int32_t max_limit);
	AlphaRandomRangeSequence(const AlphaRandomRangeSequence &seq) = delete;
	AlphaRandomRangeSequence & operator=(const AlphaRandomRangeSequence &seq) = delete;
	bool get_entropy(int32_t *dest, const uint32_t size) override;

	virtual ~AlphaRandomRangeSequence() override;

private:
	AlphaRngApi *m_api;
};

} /* namespace alpharng */

#endif /* ALPHA_RANDOMRANGESEQUENCE_H_ */
