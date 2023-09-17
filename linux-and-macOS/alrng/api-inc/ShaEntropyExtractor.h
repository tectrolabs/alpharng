/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for extracting entropy from the AlphaRNG device using SHA for seeding a DRBG.

 It uses OpenSSL library.

 */

/**
 *    @file ShaEntropyExtractor.h
 *    @date 09/16/2023
 *    @Author: Andrian Belinski
 *    @version 1.1
 *
 *    @brief Implements an API used for extracting entropy from the AlphaRNG device using SHA for seeding a DRBG.
 */

#ifndef ALPHARNG_API_INC_EXTR_SHA_IMPL_H_
#define ALPHARNG_API_INC_EXTR_SHA_IMPL_H_

#include <AlphaRngApi.h>
#include <ShaInterface.h>

namespace alpharng {

class AlphaRngApi;

class ShaEntropyExtractor {
public:
	bool extract_entropy(unsigned char *out, int len);
	ShaEntropyExtractor(AlphaRngApi *rng_api, ShaInterface *sha_api, int in_out_ratio = 2);
	std::string get_last_error() const {return m_error_log_oss.str();};
	int get_hash_size() {return m_sha_api->get_hash_size();};
	virtual ~ShaEntropyExtractor();

private:
	bool initialize();
	void clear_error_log();
	bool extract_hash_values(int sha_qty, int in_sha_byte_qty, int *entropy_bytes_needed, unsigned char **o);

private:
	AlphaRngApi *m_rng_api = nullptr;
	ShaInterface *m_sha_api = nullptr;
	std::ostringstream m_error_log_oss;

	// How many input bytes are used for extracting one byte of entropy.
	// When set to 2 then for each byte in the output there will be 2 bytes used for input.
	// It cannot be less then 1.
	const int m_in_out_ratio;

	// A byte array for collecting noise from the noise sources.
	unsigned char *m_noise_buff = nullptr;

	// The size of the noise buffer in bytes.
	const int m_noise_buff_bytes;

	// A byte array to temporary store the hash value produced.
	unsigned char *m_hash_value = nullptr;

	// The digest output size of the current SHA algorithm in bytes.
	const int m_cur_sha_size;

	bool m_is_initialized = false;

};

} /* namespace alpharng */

#endif /* ALPHARNG_API_INC_EXTR_SHA_IMPL_H_ */
