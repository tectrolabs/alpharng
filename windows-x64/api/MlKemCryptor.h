/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for establishing a secure session between the host computer and the AlphaRNG PQ device
 using ML-KEM-1024 (FIPS 203) key encapsulation.

 It uses mlkem-native for ML-KEM and the OpenSSL library for PEM decoding and random numbers.

 */

/**
 *    @file MlKemCryptor.h
 *    @date 11/03/2026
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Used for establishing a secure session between the host computer and the AlphaRNG PQ device using
 *    ML-KEM-1024 key encapsulation.
 */

#ifndef ALPHARNG_MLKEMCRYPTOR_H_
#define ALPHARNG_MLKEMCRYPTOR_H_

#include <string>

namespace alpharng {

class MlKemCryptor {
public:
	static const int c_public_key_size_bytes = 1568;	// ML-KEM-1024 encapsulation key
	static const int c_capsule_size_bytes = 1568;		// ML-KEM-1024 ciphertext
	static const int c_shared_secret_size_bytes = 32;

	explicit MlKemCryptor(const std::string &public_key_file_name);
	MlKemCryptor(const unsigned char *key, int key_size_bytes);
	MlKemCryptor(const MlKemCryptor &cryptor) = delete;
	MlKemCryptor & operator=(const MlKemCryptor &cryptor) = delete;
	bool is_initialized() const {return m_is_key_initialized;}
	bool is_public_key_file() const {return m_is_public_key_file;}

	bool encapsulate(unsigned char *capsule, int capsule_size_bytes, unsigned char *shared_secret, int shared_secret_size_bytes);
	virtual ~MlKemCryptor();

private:
	void initialize_with_key(const unsigned char *key, int key_size_bytes);
	bool decode_pem_public_key(const unsigned char *pem, int pem_size_bytes);

private:
	unsigned char m_public_key[c_public_key_size_bytes];
	bool m_is_key_initialized = false;
	const int c_m_file_key_max_size_bytes = 16384;
	bool m_is_public_key_file = false;
};

} /* namespace alpharng */

#endif /* ALPHARNG_MLKEMCRYPTOR_H_ */
