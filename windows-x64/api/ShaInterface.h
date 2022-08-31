/**
 Copyright (C) 2014-2022 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This interface may only be used in conjunction with TectroLabs devices.

 This interface is used for generating a message digest used with the AlphaRNG device.

 */

/**
 *    @file ShaInterface.h
 *    @date 08/27/2022
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Provides an API for generating a message digest used with the AlphaRNG device.
 */

#ifndef ALPHARNG_API_INC_SHAINTERFACE_H_
#define ALPHARNG_API_INC_SHAINTERFACE_H_

namespace alpharng {

class ShaInterface {
public:
	virtual bool hash(const unsigned char *in, int in_byte_count, unsigned char *out) = 0;
	virtual int get_hash_size() = 0;

	ShaInterface() {};
	virtual ~ShaInterface() {};
};

} /* namespace alpharng */

#endif /* ALPHARNG_API_INC_SHAINTERFACE_H_ */
