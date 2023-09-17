/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for message authentication in data communication with the AlphaRNG device.

 It uses OpenSSL library.

 */

/**
 *    @file HmacMD5.cpp
 *    @date 9/16/2023
 *    @Author: Andrian Belinski
 *    @version 1.4
 *
 *    @brief Implements an API used for generating a HmacMD5 message authentication digest for communicating with the AlphaRNG device.
 */

#include <HmacMD5.h>

namespace alpharng {

/*
 * Generate new random MAC key of the expected size.
 * A new key should be generated for each new session.
 *
 * @return true for successful operation
 */
bool HmacMD5::generate_new_key() {
	if (m_key != nullptr) {
		if (!RAND_bytes(m_key, c_key_size_bytes)) {
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
bool HmacMD5::hmac(const unsigned char *in, int in_byte_count, unsigned char *out) {

	const unsigned char *result = HMAC(EVP_md5(), m_key, c_key_size_bytes, in, in_byte_count, nullptr, nullptr);
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
int HmacMD5::get_mac_size() {
	return c_key_size_bytes;
}

/**
 * Retrieve the current MAC key
 *
 * @param[out] out points to a location to store the key
 *
 * @return true for successful operation
 */
bool HmacMD5::get_mac_key(unsigned char* out) {
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
bool HmacMD5::set_key(unsigned char* in, int in_byte_count) {
	if (in_byte_count != c_key_size_bytes) {
		return false;
	}
	memcpy(m_key, in, c_key_size_bytes);
	return true;
}

HmacMD5::HmacMD5() {
	m_key = new (std::nothrow) unsigned char[c_key_size_bytes];
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

HmacMD5::~HmacMD5() {
	if (m_key) {
		delete [] m_key;
	}
}

} /* namespace alpharng */
