/**
 Copyright (C) 2014-2022 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for message authentication in data communication with the AlphaRNG device.

 It uses OpenSSL library.

 */

/**
 *    @file HmacSha1.cpp
 *    @date 08/27/2022
 *    @Author: Andrian Belinski
 *    @version 1.2
 *
 *    @brief Implements an API used for generating a HmacSHA160 message authentication digest for communicating with the AlphaRNG device.
 */

#include "pch.h"
#include <HmacSha1.h>

namespace alpharng {

/*
 * Generate new random MAC key of the expected size.
 * A new key should be generated for each new session.
 *
 * @return true for successful operation
 */
bool HmacSha1::generate_new_key() {
	if (m_key != nullptr) {
		if (!RAND_bytes(m_key, (int)c_key_size_bytes)) {
			return false;
		}
		return true;
	}
	return false;
}

/**
 * Compute a MAC digest for the bytes requested
 *
 * @param[in] in points to the byte array as input data
 * @param[in] in_byte_count size of the input byte array
 * @param[out] out points to a location with enough storage for the resulting MAC code
 *
 * @return true if MAC generated successfully
 */
bool HmacSha1::hmac(const unsigned char *in, int in_byte_count, unsigned char *out) {

	unsigned char *result = HMAC(EVP_sha1(), m_key, c_key_size_bytes, in, in_byte_count, NULL, NULL);
	if (result == nullptr) {
		return false;
	}
	memcpy(out, result, c_key_size_bytes);
	return true;
}

/**
 * Retrieve the size of the MAC key in bytes
 *
 * @return MAC key size
 */
int HmacSha1::get_mac_size() {
	return c_key_size_bytes;
}

/**
 * Retrieve the current MAC key
 *
 * @param[out] out points to a location to store the key
 *
 * @return true for successful operation
 */
bool HmacSha1::get_mac_key(unsigned char* out) {
	if (!m_initialized) {
		return false;
	}
	memcpy(out, m_key, c_key_size_bytes);
	return true;
}

/**
 * Set new MAC key
 *
 * @param[in] in points to the byte array of the new key
 * @param[in] in_byte_count size of the new key in bytes
 *
 * @return true for successful operation
 */
bool HmacSha1::set_key(unsigned char* in, int in_byte_count) {
	if (in_byte_count != c_key_size_bytes) {
		return false;
	}
	memcpy(m_key, in, c_key_size_bytes);
	return true;
}

HmacSha1::HmacSha1() {
	m_key = new (nothrow) unsigned char[c_key_size_bytes];
	if (m_key == nullptr) {
		m_initialized = false;
		return;
	}
	if (!generate_new_key()) {
		m_initialized = false;
		return;
	}
	m_initialized = true;

}

HmacSha1::~HmacSha1() {
	if (m_key) {
		delete [] m_key;
	}
}

} /* namespace alpharng */
