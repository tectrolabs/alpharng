/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for establishing a data communication secure channel between a host system and
 the AlphaRNG device.

 It uses OpenSSL library.

 */

/**
 *    @file AesCryptor.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Encrypts and decrypts session data using AES with 128 or 256 bit keys.
 */

#ifndef API_SRC_AESCRYPTOR_H_
#define API_SRC_AESCRYPTOR_H_

#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <string.h>

#include "Structures.h"

namespace alpharng {

class AesCryptor {
public:
	bool encrypt(const unsigned char *in, int in_byte_count, unsigned char *out, int *out_byte_count);
	bool decrypt(const unsigned char *in, int in_byte_count, unsigned char *out, int *out_byte_count);
	bool is_initialized() {return m_initialized;}
	bool get_key(unsigned char* out);
	int get_key_size_bytes() {return (int)m_e_key_size;}
	AesCryptor(KeySize e_key_size);
	AesCryptor();
	virtual ~AesCryptor();
private:
	void initialize(KeySize e_key_size, const unsigned char *iv, const unsigned char *key);
	void initialize(KeySize e_key_size);
private:
	EVP_CIPHER_CTX *m_ctx_enc = nullptr;
	EVP_CIPHER_CTX *m_ctx_dec = nullptr;
	bool m_initialized = false;
	unsigned char *m_key = nullptr;
	KeySize m_e_key_size;
};

} /* namespace alpharng */

#endif /* API_SRC_AESCRYPTOR_H_ */
