/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for message authentication in data communication with the AlphaRNG device.

 It uses OpenSSL library.

 */

/**
 *    @file HmacSha1.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Implements an API used for generating a HmacSHA160 message authentication digest for communicating with the AlphaRNG device.
 */

#ifndef API_INC_HMACSHA1_H_
#define API_INC_HMACSHA1_H_

#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <string.h>
#include "HmacInterface.h"

namespace alpharng {

class HmacSha1 : public HmacInterface {
public:
	bool hmac(const unsigned char *in, int in_byte_count, unsigned char *out);
	int get_mac_size();
	bool get_mac_key(unsigned char* out);
	bool generate_new_key();
	bool is_initialized() {return m_initialized;}
	HmacSha1();
	virtual ~HmacSha1();
private:
	bool m_initialized = false;
	unsigned char *m_key = nullptr;
	const int c_key_size_bytes = 20;

};

} /* namespace alpharng */

#endif /* API_INC_HMACSHA1_H_ */
