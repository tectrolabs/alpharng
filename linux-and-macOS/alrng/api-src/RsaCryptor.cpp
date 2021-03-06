/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for establishing a secure session between the host computer and the AlphaRNG device.

 It uses OpenSSL library.

 */

/**
 *    @file RsaCryptor.cpp
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Used for establishing a secure session between the host computer and the AlphaRNG device.
 */

#include "pch.h"
#include <RsaCryptor.h>

namespace alpharng {

void RsaCryptor::initialize() {
	m_rsa = nullptr;
	m_bignum = nullptr;
	m_kbio_rsa = nullptr;
	m_padding = RSA_NO_PADDING;
	m_is_key_initialized = false;
}

/**
 * Construct using a key provided as bytes
 *
 * @param[in] key points to location of the key
 * @param[in] key_size_bytes size of the key in bytes
 * @param[in] is_public true if the key is public, false if the key is private
 */
RsaCryptor::RsaCryptor(const unsigned char *key, int key_size_bytes, bool is_public) {
	initialize();
	m_kbio_rsa = BIO_new_mem_buf((char *)key, key_size_bytes);
	if (m_kbio_rsa == nullptr) {
		return;
	}

	if (is_public) {
		m_rsa = PEM_read_bio_RSAPublicKey(m_kbio_rsa, nullptr, 0, nullptr);
	} else {
		m_rsa = PEM_read_bio_RSAPrivateKey(m_kbio_rsa, nullptr, 0, nullptr);
	}

	if (m_rsa == nullptr) {
		return;
	}
	m_is_key_initialized = true;
}

/**
 *
 * Construct using a key file specified
 *
 * @param[in] key_file_name file pathname to a key file
 * @param[in] is_public true if the key is public, false if the key is private
 *
 */
RsaCryptor::RsaCryptor(const string &key_file_name, bool is_public) {
	initialize();

	FILE *fp = nullptr;
	m_is_public_key_file = is_public;

	if (is_public) {
		fp = fopen(key_file_name.c_str(), "r");
		if (fp != nullptr) {
			if (PEM_read_RSAPublicKey(fp, &m_rsa, nullptr, nullptr) == nullptr) {
				fclose(fp);
				return;
			} else {
				fclose(fp);
				m_is_key_initialized = true;
				return;
			}
		}
	}

	fp = fopen(key_file_name.c_str(), "r");
	if (fp != nullptr) {
		if (PEM_read_RSAPrivateKey(fp, &m_rsa, nullptr, nullptr) == nullptr) {
			fclose(fp);
			return;
		}
		fclose(fp);
		m_is_key_initialized = true;
		return;
	}
}

/**
 * Encrypt bytes with RSA using public key
 *
 * @param[in] in points to the byte array to encrypt
 * @param[in] in_size_bytes number of bytes to encrypt
 * @param[out] out points to the byte array for encrypted results
 * @param[out] out_size_bytes a pointer to store the number of encrypted bytes
 *
 * @return true if encryption was successful
 */
bool RsaCryptor::encrypt_with_public_key(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes) {

	if (!m_is_key_initialized || in_size_bytes == 0 || out == nullptr || in == 0) {
		return false;
	}

    int result = RSA_public_encrypt(in_size_bytes, (unsigned char*)in, out, m_rsa, m_padding);
	if (result == -1) {
		return false;
	}
	*out_size_bytes = result;
	return true;
}

/**
 * Encrypt bytes with RSA using private key
 *
 * @param[in] in points to the byte array to encrypt
 * @param[in] in_size_bytes number of bytes to encrypt
 * @param[out] out points to the byte array for encrypted results
 * @param[out] out_size_bytes a pointer to store the actual number of encrypted bytes
 *
 * @return true if encryption was successful
 */
bool RsaCryptor::encrypt_with_private_key(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes) {

	if (!m_is_key_initialized || in_size_bytes == 0 || out == nullptr || in == 0) {
		return false;
	}

    int result = RSA_private_encrypt(in_size_bytes, in, out, m_rsa, m_padding);
	if (result == -1) {
		return false;
	}
	*out_size_bytes = result;
	return true;
}

/**
 * Decrypt bytes with RSA using the public key
 *
 * @param[in] in points to the byte array to decrypt
 * @param[in] in_size_bytes amount of bytes to decrypt
 * @param[out] out points to location for the decrypted bytes
 * @param[out] out_size_bytes points to location for actual number of bytes decrypted
 *
 * @return true if decryption was successful
 */
bool RsaCryptor::decrypt_with_public_key(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes) {
	if (!m_is_key_initialized || in_size_bytes == 0 || out == nullptr || in == 0) {
		return false;
	}

    int result = RSA_public_decrypt(in_size_bytes, in, out, m_rsa, m_padding);
	if (result == -1) {
		return false;
	}
	*out_size_bytes = result;
	return true;
}

/**
 * Decrypt bytes with RSA using private key
 *
 * @param[in] in points to the byte array to decrypt
 * @param[in] in_size_bytes amount of bytes to decrypt
 * @param[out] out points to location for the decrypted bytes
 * @param[out] out_size_bytes points to location for actual number of bytes decrypted
 *
 * @return true if decryption was successful
 */
bool RsaCryptor::decrypt_with_private_key(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes) {
	if (!m_is_key_initialized || in_size_bytes == 0 || out == nullptr || in == 0) {
		return false;
	}

    int result = RSA_private_decrypt(in_size_bytes, in, out, m_rsa, m_padding);
	if (result == -1) {
		return false;
	}
	*out_size_bytes = result;
	return true;
}

/**
 *
 * Construct and generate new RSA key of requested size
 *
 * @param[in] key_size RSA key size
 *
 */
RsaCryptor::RsaCryptor(int key_size) {
	initialize();
	crete_new_key(key_size);
}

/**
 *
 * Construct and generate new RSA 2048 bit key
 *
 */
RsaCryptor::RsaCryptor() {
	initialize();
	crete_new_key(2048);
}

/**
 * Export private key to a file
 *
 * @param[in] key_file_name file name used for storing the private key
 *
 * @return true if export was successful
 */
bool RsaCryptor::export_private_key_to_file(const string &key_file_name) {
	if (!m_is_key_initialized) {
		return false;
	}

	BIO *bp = BIO_new_file(key_file_name.c_str(), "w+");
	if (PEM_write_bio_RSAPrivateKey(bp, m_rsa, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
		BIO_free_all(bp);
		return false;
	}
	BIO_free_all(bp);
	return true;
}

/**
 * Export public key to a file
 *
 * @param[in] key_file_name file name used for storing the public key
 *
 * @return true if export was successful
 */
bool RsaCryptor::export_public_key_to_file(const string &key_file_name) {
	if (!m_is_key_initialized) {
		return false;
	}

	BIO *bp = BIO_new_file(key_file_name.c_str(), "w+");
	if (PEM_write_bio_RSAPublicKey(bp, m_rsa) != 1) {
		BIO_free_all(bp);
		return false;
	}
	BIO_free_all(bp);
	return true;
}

/**
 *
 * Create a new key pair
 *
 * @param[in] key_size RSA key size
 *
 */
void RsaCryptor::crete_new_key(int key_size) {
	m_is_key_initialized = false;

	unsigned long e = RSA_F4;

	m_bignum = BN_new();

	if (BN_set_word(m_bignum, e) != 1) {
		return;
	}

	m_rsa = RSA_new();
	if (RSA_generate_key_ex(m_rsa, key_size, m_bignum, nullptr) != 1) {
		return;
	}

	m_is_key_initialized = true;
}

/**
 *
 * Check to see if the instance is initialized
 *
 * @return true if initialized
 *
 */
bool RsaCryptor::is_initialized() {
	return m_is_key_initialized;
}

/**
 * De-allocate resources.
 */
RsaCryptor::~RsaCryptor() {
	if (m_rsa) {
		RSA_free(m_rsa);
	}
	if (m_bignum) {
		BN_free(m_bignum);
	}
	if (m_kbio_rsa) {
		BIO_free(m_kbio_rsa);
	}
}

} /* namespace alpharng */
