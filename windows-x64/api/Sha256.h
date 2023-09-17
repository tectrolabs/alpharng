/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for generating a SHA-256 message digest used with the AlphaRNG device.

 It uses OpenSSL library.

 */

/**
 *    @file Sha256.h
 *    @date 09/16/2023
 *    @Author: Andrian Belinski
 *    @version 1.1
 *
 *    @brief Implements an API used for generating a SHA-256 message digest used with the AlphaRNG device.
 */

#ifndef ALPHARNG_API_INC_SHA256_H_
#define ALPHARNG_API_INC_SHA256_H_

#include <openssl/sha.h>

#include <ShaInterface.h>

namespace alpharng {

class Sha256 : public ShaInterface {
public:
	bool hash(const unsigned char *in, int in_byte_count, unsigned char *out) override;
	int get_hash_size() override;
	Sha256(const Sha256 &hmac) = delete;
	Sha256 & operator=(const Sha256 &hmac) = delete;
	Sha256() = default;
	virtual ~Sha256() = default;
private:
	const int c_hash_size_bytes = 32;
};

} /* namespace alpharng */

#endif /* ALPHARNG_API_INC_SHA256_H_ */
