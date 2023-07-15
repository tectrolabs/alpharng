/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for generating a SHA-512 message digest used with the AlphaRNG device.

 It uses OpenSSL library.

 */

/**
 *    @file Sha512.cpp
 *    @date 7/15/2023
 *    @Author: Andrian Belinski
 *    @version 1.1
 *
 *    @brief Implements an API used for generating a SHA-512 message digest used with the AlphaRNG device.
 */

#include <Sha512.h>

namespace alpharng {

/**
 * Compute a SHA-512 message digest for the bytes requested
 *
 * @param[in] in points to the byte array as input data
 * @param[in] in_byte_count size of the input byte array
 * @param[out] out points to a location with enough storage for the resulting HASH value
 *
 * @return true if message digest generated successfully
 */
bool Sha512::hash(const unsigned char *in, int in_byte_count, unsigned char *out) {
	unsigned char *result = SHA512(in, in_byte_count, out);
	if (result == nullptr) {
		return false;
	}
	return true;
}

/**
 * Retrieve the size of the HASH message digest in bytes
 *
 * @return HASH value size
 */
int Sha512::get_hash_size() {
	return c_hash_size_bytes;
}

Sha512::Sha512() {
}

Sha512::~Sha512() {
}

} /* namespace alpharng */
