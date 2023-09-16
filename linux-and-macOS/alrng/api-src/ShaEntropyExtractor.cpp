/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This interface may only be used in conjunction with TectroLabs devices.

 This interface is used for extracting entropy from the AlphaRNG device for seeding a DRBG.

 */

/**
 *    @file ShaEntropyExtractor.cpp
 *    @date 9/16/2023
 *    @Author: Andrian Belinski
 *    @version 1.2
 *
 *    @brief Provides an API for extracting entropy from the AlphaRNG device for seeding a DRBG.
 */

#include <ShaEntropyExtractor.h>

namespace alpharng {

ShaEntropyExtractor::ShaEntropyExtractor(AlphaRngApi *rng_api, ShaInterface *sha_api, int in_out_ratio)
		: m_rng_api(rng_api), m_sha_api(sha_api),
		  m_in_out_ratio(in_out_ratio),
		  m_noise_buff_bytes(in_out_ratio * sha_api->get_hash_size() * 1000),
		  m_cur_sha_size (sha_api->get_hash_size()) {
}

bool ShaEntropyExtractor::initialize() {
	if (m_is_initialized) {
		m_error_log_oss << "ShaEntropyExtractor.initialize() already initialized" << std::endl;
		return false;
	}

	if (m_in_out_ratio < 1) {
		m_error_log_oss << "ShaEntropyExtractor.initialize(): invalid input/output ratio " << m_in_out_ratio << ", must be 1 or grater value" << std::endl;
		return false;
	}

	m_noise_buff = new (std::nothrow) unsigned char [m_noise_buff_bytes];
	if (m_noise_buff == nullptr) {
		m_error_log_oss << "ShaEntropyExtractor.initialize(): could not allocate " << m_noise_buff_bytes << " bytes for the noise buffer" << std::endl;
		return false;
	}

	m_hash_value = new (std::nothrow) unsigned char [m_cur_sha_size];
	if (m_hash_value == nullptr) {
		m_error_log_oss << "ShaEntropyExtractor.initialize(): could not allocate " << m_cur_sha_size << " bytes for the hash output value" << std::endl;
		return false;
	}

	m_is_initialized = true;
	return true;
}

ShaEntropyExtractor::~ShaEntropyExtractor() {
	if (m_noise_buff != nullptr) {
		delete [] m_noise_buff;
	}
	if (m_hash_value != nullptr) {
		delete [] m_hash_value;
	}
}

/**
 * Extract entropy bytes by applying specific SHA method to RAW random bytes retrieved from the AlphaRNG device.
 * The entropy bytes are produced based on In/Out ratio defined by m_in_out_ratio variable.
 *
 * @param[out] out points to a byte array for storing the extracted entropy bytes
 * @param[in] len how many entropy bytes to extract
 *
 * @return true for successful operation
 */
bool ShaEntropyExtractor::extract_entropy(unsigned char *out, int len) {
	if (!m_is_initialized) {
		if (!initialize()) {
			return false;
		}
	}
	clear_error_log();
	if (out == nullptr) {
		m_error_log_oss << "ShaEntropyExtractor.extract_entropy(): 'out' argument cannot be null" << std::endl;
		return false;
	}

	if (len < 1) {
		m_error_log_oss << "ShaEntropyExtractor.extract_entropy(): invalid 'len' argument value: " << len << std::endl;
		return false;
	}

	// Total number of SHA values we will need
	int sha_qty = len / m_cur_sha_size;
	if (len % m_cur_sha_size) {
		// Round up for a complete hash
		++sha_qty;
	}

	// Number of noise bytes required to produce one SHA value
	int in_sha_byte_qty = m_cur_sha_size * m_in_out_ratio;

	// Total number of noise bytes required to produce SHA values
	long total_in_sha_byte_qty = in_sha_byte_qty * sha_qty;

	// Total number of "get noise" requests
	int total_noise_req_qty = (int) (total_in_sha_byte_qty / m_noise_buff_bytes);

	// Number of noise bytes to request for the last incomplete buffer
	int last_noise_req_bytes = total_in_sha_byte_qty % m_noise_buff_bytes;

	// Number of hash values to produce per the last incomplete buffer
	int last_sha_qty = last_noise_req_bytes / in_sha_byte_qty;

	// Total number of hash values to produce per buffer
	int total_hash_qty_per_buffer = m_noise_buff_bytes / in_sha_byte_qty;


	unsigned char *o = out;
	int entropy_bytes_needed = len;
	for (int req = 0; req < total_noise_req_qty; ++req) {
		// Retrieve noise bytes from device
		if (!m_rng_api->get_noise(m_noise_buff, m_noise_buff_bytes)) {
			return false;
		}

		if (!extract_hash_values(total_hash_qty_per_buffer, in_sha_byte_qty, &entropy_bytes_needed, &o)) {
			return false;
		}
	}

	if (last_noise_req_bytes > 0 && entropy_bytes_needed > 0) {
		// Retrieve noise bytes from device
		if (!m_rng_api->get_noise(m_noise_buff, last_noise_req_bytes)) {
			return false;
		}

		if (!extract_hash_values(last_sha_qty, in_sha_byte_qty, &entropy_bytes_needed, &o)) {
			return false;
		}
	}
	return true;
}

/**
 * Extract entropy bytes by applying specific SHA method to RAW random bytes retrieved from the AlphaRNG device.
 * The entropy bytes are produced based on In/Out ratio defined by m_in_out_ratio variable.
 *
 * @param[in] sha_qty number of SHA values to produce
 * @param[in] in_sha_byte_qty number of noise bytes required to produce one SHA value
 * @param[in/out] entropy_bytes_needed number of entropy bytes requested
 * @param[out] o pointer to a byte array to store the entropy bytes produced
 *
 * @return true for successful operation
 */
bool ShaEntropyExtractor::extract_hash_values(int sha_qty, int in_sha_byte_qty, int *entropy_bytes_needed, unsigned char **o) {
	const unsigned char *in = m_noise_buff;
	for (int sh = 0; sh < sha_qty; ++sh) {
		// Hash the noise bytes
		if (!m_sha_api->hash(in, in_sha_byte_qty, m_hash_value)) {
			m_error_log_oss << "ShaEntropyExtractor.process(): could not hash requested bytes" << std::endl;
			return false;
		}
		in += in_sha_byte_qty;
		if (*entropy_bytes_needed >= m_cur_sha_size) {
			// Return all hash value bytes to caller's output byte array
			memcpy(*o, m_hash_value, m_cur_sha_size);
			*o = *o + m_cur_sha_size;
			*entropy_bytes_needed = *entropy_bytes_needed - m_cur_sha_size;
		} else {
			if (*entropy_bytes_needed > 0) {
				// Return truncated hash bytes
				memcpy(*o, m_hash_value, *entropy_bytes_needed);
				*o = *o + *entropy_bytes_needed;
				*entropy_bytes_needed = 0;
			}
		}
	}
	return true;
}

void ShaEntropyExtractor::clear_error_log() {
	m_error_log_oss.str("");
	m_error_log_oss.clear();
}

} /* namespace alpharng */
