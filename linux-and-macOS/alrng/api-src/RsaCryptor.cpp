/**
 Copyright (C) 2014-2021 TectroLabs L.L.C. https://tectrolabs.com

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
 *    @version 1.3
 *
 *    @brief Used for establishing a secure session between the host computer and the AlphaRNG device.
 */

#include "pch.h"
#include <RsaCryptor.h>

namespace alpharng {

/**
 * Construct using a key provided as bytes
 *
 * @param[in] key points to location of the key
 * @param[in] key_size_bytes size of the key in bytes
 * @param[in] is_public true if the key is public, false if the key is private
 */
RsaCryptor::RsaCryptor(const unsigned char *key, int key_size_bytes, bool is_public) {
	initialize_with_key(key, key_size_bytes, is_public);
}

/**
 * Initialize using a key provided as bytes
 *
 * @param[in] key points to location of the key
 * @param[in] key_size_bytes size of the key in bytes
 * @param[in] is_public true if the key is public, false if the key is private
 */
void RsaCryptor::initialize_with_key(const unsigned char* key, int key_size_bytes, bool is_public) {
	m_kbio_rsa = BIO_new_mem_buf((char*)key, key_size_bytes);
	if (m_kbio_rsa == nullptr) {
		return;
	}

#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	if (is_public) {
		m_rsa = PEM_read_bio_PUBKEY(m_kbio_rsa, nullptr, nullptr, nullptr);
	}
	else {
		m_rsa = PEM_read_bio_PrivateKey(m_kbio_rsa, nullptr, 0, nullptr);
	}
#else
	if (is_public) {
		m_rsa = PEM_read_bio_RSAPublicKey(m_kbio_rsa, nullptr, 0, nullptr);
	}
	else {
		m_rsa = PEM_read_bio_RSAPrivateKey(m_kbio_rsa, nullptr, 0, nullptr);
	}

#endif

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

	ifstream is_file(key_file_name.c_str(), ios::in | ios::binary);
	if (!is_file.good()) {
		return;
	}


	m_file_pub_key_bytes = new (nothrow) unsigned char[c_m_file_pub_key_max_size_bytes];
	if (m_file_pub_key_bytes == nullptr) {
		return;
	}

	is_file.seekg(0, is_file.end);
	int length = (int)is_file.tellg();
	is_file.seekg(0, is_file.beg);

	if (length <= 0 || length > c_m_file_pub_key_max_size_bytes) {
		return;
	}

	is_file.read((char*)m_file_pub_key_bytes, length);
	if (!is_file.good()) {
		return;
	}

	initialize_with_key(m_file_pub_key_bytes, length, is_public);
	if (m_is_key_initialized) {
		m_is_public_key_file = is_public;
	}

}

#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
/**
 * Encrypt bytes with RSA key
 *
 * @param[in] in points to the byte array to encrypt
 * @param[in] in_size_bytes number of bytes to encrypt
 * @param[out] out points to the byte array for encrypted results
 * @param[out] out_size_bytes a pointer to store the number of encrypted bytes
 *
 * @return true if encryption was successful
 */
bool RsaCryptor::evp_key_encrypt(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes) {

	EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(m_rsa, nullptr);
	if (ctx == nullptr) {
		return false;
	}
	if (EVP_PKEY_encrypt_init(ctx) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return false;
	}
	if (EVP_PKEY_CTX_set_rsa_padding(ctx, m_padding) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return false;
	}
	size_t outlen;
	if (EVP_PKEY_encrypt(ctx, nullptr, &outlen, in, in_size_bytes) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return false;
	}
	if (EVP_PKEY_encrypt(ctx, out, &outlen, in, in_size_bytes) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return false;
	}
	*out_size_bytes = outlen;
	EVP_PKEY_CTX_free(ctx);
	return true;
}
#endif

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

#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	return evp_key_encrypt(in, in_size_bytes, out, out_size_bytes);
#else
    int result = RSA_public_encrypt(in_size_bytes, (unsigned char*)in, out, m_rsa, m_padding);
	if (result == -1) {
		return false;
	}
	*out_size_bytes = result;
	return true;
#endif
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
#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	return evp_key_encrypt(in, in_size_bytes, out, out_size_bytes);
#else
    int result = RSA_private_encrypt(in_size_bytes, in, out, m_rsa, m_padding);
	if (result == -1) {
		return false;
	}
	*out_size_bytes = result;
	return true;
#endif
}

#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
/**
 * Decrypt bytes with RSA key
 *
 * @param[in] in points to the byte array to decrypt
 * @param[in] in_size_bytes amount of bytes to decrypt
 * @param[out] out points to location for the decrypted bytes
 * @param[out] out_size_bytes points to location for actual number of bytes decrypted
 *
 * @return true if decryption was successful
 */
bool RsaCryptor::evp_key_decrypt(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes) {
	EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new(m_rsa, nullptr);
	if (ctx == nullptr) {
		return false;
	}
	if (EVP_PKEY_encrypt_init(ctx) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return false;
	}
	if (EVP_PKEY_CTX_set_rsa_padding(ctx, m_padding) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return false;
	}
	size_t outlen;
	if (EVP_PKEY_decrypt(ctx, nullptr, &outlen, in, in_size_bytes) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return false;
	}
	if (EVP_PKEY_decrypt(ctx, out, &outlen, in, in_size_bytes) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		return false;
	}
	*out_size_bytes = outlen;
	EVP_PKEY_CTX_free(ctx);
	return true;
}
#endif

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
#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	return evp_key_decrypt(in, in_size_bytes, out, out_size_bytes);
#else

    int result = RSA_public_decrypt(in_size_bytes, in, out, m_rsa, m_padding);
	if (result == -1) {
		return false;
	}
	*out_size_bytes = result;
	return true;
#endif
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
#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	return evp_key_decrypt(in, in_size_bytes, out, out_size_bytes);
#else
    int result = RSA_private_decrypt(in_size_bytes, in, out, m_rsa, m_padding);
	if (result == -1) {
		return false;
	}
	*out_size_bytes = result;
	return true;
#endif
}

/**
 *
 * Construct and generate new RSA key of requested size
 *
 * @param[in] key_size RSA key size
 *
 */
RsaCryptor::RsaCryptor(int key_size) {
	crete_new_key(key_size);
}

/**
 *
 * Construct and generate new RSA 2048 bit key
 *
 */
RsaCryptor::RsaCryptor() {
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
#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	if (PEM_write_bio_PrivateKey(bp, m_rsa, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
		BIO_free_all(bp);
		return false;
	}
#else
	if (PEM_write_bio_RSAPrivateKey(bp, m_rsa, nullptr, nullptr, 0, nullptr, nullptr) != 1) {
		BIO_free_all(bp);
		return false;
	}
#endif
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

#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	if (PEM_write_bio_PUBKEY(bp, m_rsa) != 1) {
		BIO_free_all(bp);
		return false;
	}
#else
	if (PEM_write_bio_RSAPublicKey(bp, m_rsa) != 1) {
		BIO_free_all(bp);
		return false;
	}
#endif
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

#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	m_rsa = EVP_RSA_gen(key_size);
#else
	unsigned long e = RSA_F4;

	m_bignum = BN_new();

	if (BN_set_word(m_bignum, e) != 1) {
		return;
	}

	m_rsa = RSA_new();
	if (RSA_generate_key_ex(m_rsa, key_size, m_bignum, nullptr) != 1) {
		return;
	}
#endif

	m_is_key_initialized = true;
}

/**
 *
 * Check to see if the instance is initialized
 *
 * @return true if initialized
 *
 */
bool RsaCryptor::is_initialized() const {
	return m_is_key_initialized;
}

/**
 * De-allocate resources.
 */
RsaCryptor::~RsaCryptor() {
#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	if (m_rsa) {
		EVP_PKEY_free(m_rsa);
	}
#else
	if (m_rsa) {
		RSA_free(m_rsa);
	}
	if (m_bignum) {
		BN_free(m_bignum);
	}
#endif
	if (m_kbio_rsa) {
		BIO_free(m_kbio_rsa);
	}
	if (m_file_pub_key_bytes) {
		delete[] m_file_pub_key_bytes;
	}
}

} /* namespace alpharng */
