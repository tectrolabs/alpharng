/**
 Copyright (C) 2014-2021 TectroLabs L.L.C. https://tectrolabs.com

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
 *    @version 1.1
 *
 *    @brief Implements an API used for generating a HmacSHA160 message authentication digest for communicating with the AlphaRNG device.
 */

#ifndef API_INC_HMACSHA1_H_
#define API_INC_HMACSHA1_H_

#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <cstring>
#include <new>
#include <HmacInterface.h>

using namespace std;

namespace alpharng {

class HmacSha1 : public HmacInterface {
public:
	bool hmac(const unsigned char *in, int in_byte_count, unsigned char *out) override;
	int get_mac_size() override;
	bool get_mac_key(unsigned char* out) override;
	bool generate_new_key() override;
	bool is_initialized() override {return m_initialized;}
	HmacSha1();
	HmacSha1(const HmacSha1 &hmac) = delete;
	HmacSha1 & operator=(const HmacSha1 &hmac) = delete;

	virtual ~HmacSha1();
private:
	bool m_initialized = false;
	unsigned char *m_key = nullptr;
	const int c_key_size_bytes = 20;

};

} /* namespace alpharng */

#endif /* API_INC_HMACSHA1_H_ */
