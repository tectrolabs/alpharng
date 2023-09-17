/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This interface may only be used in conjunction with TectroLabs devices.

 This interface is used for message authentication in data communication with the AlphaRNG device.

 */

/**
 *    @file HmacInterface.h
 *    @date 09/16/2023
 *    @Author: Andrian Belinski
 *    @version 1.3
 *
 *    @brief Provides an API for generating message authentication digests for communicating with the AlphaRNG device.
 */

#ifndef ALPHARNG_API_INC_HMACINTERFACE_H_
#define ALPHARNG_API_INC_HMACINTERFACE_H_

namespace alpharng {

class HmacInterface {
public:
	virtual bool hmac(const unsigned char *in, int in_byte_count, unsigned char *out) = 0;
	virtual int get_mac_size() = 0;
	virtual bool get_mac_key(unsigned char* out) = 0;
	virtual bool generate_new_key() = 0;
	virtual bool set_key(unsigned char* in, int in_byte_count) = 0;
	virtual bool is_initialized() = 0;

	HmacInterface() = default;
	virtual ~HmacInterface() = default;
};

} /* namespace alpharng */

#endif /* ALPHARNG_API_INC_HMACINTERFACE_H_ */
