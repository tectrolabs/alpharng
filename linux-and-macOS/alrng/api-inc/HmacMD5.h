/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for message authentication in data communication with the AlphaRNG device.

 It uses OpenSSL library.

 */

/**
 *    @file HmacMD5.h
 *    @date 09/16/2023
 *    @Author: Andrian Belinski
 *    @version 1.3
 *
 *    @brief Implements an API used for generating a HmacMD5 message authentication digest for communicating with the AlphaRNG device.
 */

#ifndef ALPHARNG_HMACMD5_H_
#define ALPHARNG_HMACMD5_H_

#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <cstring>
#include <new>
#include <HmacInterface.h>

namespace alpharng {

class HmacMD5 : public HmacInterface {
public:
	bool hmac(const unsigned char *in, int in_byte_count, unsigned char *out) override;
	int get_mac_size() override;
	bool get_mac_key(unsigned char* out) override;
	bool generate_new_key() override;
	bool set_key(unsigned char* in, int in_byte_count) override;
	bool is_initialized() override {return m_initialized;}
	HmacMD5();
	HmacMD5(const HmacMD5 &hmac) = delete;
	HmacMD5 & operator=(const HmacMD5 &hmac) = delete;
	~HmacMD5() override;
private:
	bool m_initialized = false;
	unsigned char *m_key = nullptr;
	const int c_key_size_bytes = 16;

};


} /* namespace alpharng */

#endif /* ALPHARNG_HMACMD5_H_ */
