/**
 Copyright (C) 2014-2022 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for generating a SHA-512 message digest used with the AlphaRNG device.

 It uses OpenSSL library.

 */

/**
 *    @file Sha512.h
 *    @date 08/27/2022
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Implements an API used for generating a SHA-512 message digest used with the AlphaRNG device.
 */

#ifndef ALPHARNG_API_INC_SHA512_H_
#define ALPHARNG_API_INC_SHA512_H_

#include <openssl/sha.h>

#include "ShaInterface.h"

namespace alpharng {

class Sha512 : public ShaInterface {
public:
	bool hash(const unsigned char *in, int in_byte_count, unsigned char *out) override;
	int get_hash_size() override;
	Sha512();
	Sha512(const Sha512 &hmac) = delete;
	Sha512 & operator=(const Sha512 &hmac) = delete;

	virtual ~Sha512();
private:
	const int c_hash_size_bytes = 64;
};

} /* namespace alpharng */

#endif /* ALPHARNG_API_INC_SHA512_H_ */
