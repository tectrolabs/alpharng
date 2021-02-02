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
 *    @file AesCryptor.cpp
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Encrypts or decrypts session data using AES with 128 or 256 bit keys.
 */

#include <AesCryptor.h>

namespace alpharng {


/**
 * Initialize the AES cipher
 *
 * @param[in] e_key_size AES key size
 * @param[in] iv AES initialization vector
 * @param[in] key pointer to the AES key, it should match the `e_key_size`
 */
void AesCryptor::initialize(KeySize e_key_size, const unsigned char *iv, const unsigned char *key) {
	m_ctx_enc = EVP_CIPHER_CTX_new();
	if (m_ctx_enc) {
		const EVP_CIPHER *evpc = nullptr;
		switch(e_key_size) {
		case KeySize::k128:
		default:
			evpc = EVP_aes_128_ecb();
			break;
		case KeySize::k256:
			evpc = EVP_aes_256_ecb();
			break;
		}

		if (EVP_EncryptInit_ex(m_ctx_enc, evpc, nullptr, key, iv) != 1) {
			return;
		}
		EVP_CIPHER_CTX_set_padding(m_ctx_enc, 0);
	} else {
		return;
	}

	m_ctx_dec = EVP_CIPHER_CTX_new();
	if (m_ctx_dec) {
		const EVP_CIPHER *evpc = nullptr;
		switch(e_key_size) {
		case KeySize::k128:
		default:
			evpc = EVP_aes_128_ecb();
			break;
		case KeySize::k256:
			evpc = EVP_aes_256_ecb();
			break;
		}

		if (EVP_DecryptInit_ex(m_ctx_dec, evpc, nullptr, key, iv) != 1) {
			return;
		}
		EVP_CIPHER_CTX_set_padding(m_ctx_dec, 0);
		m_initialized = true;
	}

}

/**
 * Generate and store a new random AES key used in a new session
 *
 * @param[in] e_key_size AES key size
 */
void AesCryptor::initialize(KeySize e_key_size) {

	if (e_key_size == KeySize::None) {
		return;
	}
	m_e_key_size = e_key_size;
	m_key = new unsigned char[(int)m_e_key_size];
	if (m_key == nullptr) {
		m_initialized = false;
		return;
	}
	if (!RAND_bytes(m_key, (int)e_key_size)) {
		m_initialized = false;
		return;
	}
	initialize(e_key_size, nullptr, m_key);
}

AesCryptor::AesCryptor(KeySize e_key_size) {
	initialize(e_key_size);
}

AesCryptor::AesCryptor() {
	initialize(KeySize::k128);
}

/**
 * Encrypt bytes with AES using the current key and no padding.
 * The amount of bytes for encryption should match the key size.
 *
 * @param[in] in points to the bytes to be encrypted
 * @param[out] out points to location for encrypted bytes
 * @param[out] out_byte_count points to location for storing the number of encrypted bytes
 *
 * @return true if encrypted successfully
 */
bool AesCryptor::encrypt(const unsigned char *in, int in_byte_count, unsigned char *out, int *out_byte_count) {
	if (!m_initialized || in_byte_count == 0 || in == nullptr || out == nullptr) {
		return false;
	}
	if (EVP_EncryptUpdate(m_ctx_enc, out, out_byte_count, in, in_byte_count) != 1) {
		return false;
	}

	return true;
}

/**
 * Decrypt bytes with AES using the current key and no padding.
 * The amount of bytes for encryption should match the key size.
 *
 * @param[in] in points to the bytes to be decrypted
 * @param[out] out points to location for decrypted bytes
 * @param[out] out_byte_count points to location for storing the number of decrypted bytes
 *
 * @return true if decrypted successfully
 */
bool AesCryptor::decrypt(const unsigned char *in, int in_byte_count, unsigned char *out, int *out_byte_count) {
	if (!m_initialized || in_byte_count == 0 || in == nullptr || out == nullptr) {
		return false;
	}

	int len;

	if (EVP_DecryptUpdate(m_ctx_dec, out, &len, in, in_byte_count) != 1) {
		return false;
	}

	if (len != in_byte_count) {
		if (EVP_DecryptFinal_ex(m_ctx_dec, out + len, &len) != 1) {
			return false;
		}
	}

	*out_byte_count = len;
	return true;
}

/**
 * Retrieve the current AES key.
 *
 * @param[out] out points to location with enough space to store the key
 *
 * @return[out] true if key retrieved successfully
 */
bool AesCryptor::get_key(unsigned char* out) {
	if (!m_initialized) {
		return false;
	}
	memcpy(out, m_key, get_key_size_bytes());
	return true;
}

AesCryptor::~AesCryptor() {
	if (m_ctx_enc) {
		EVP_CIPHER_CTX_free(m_ctx_enc);
	}
	if (m_ctx_dec) {
		EVP_CIPHER_CTX_free(m_ctx_dec);
	}
	if (m_key) {
		delete [] m_key;
	}
}

} /* namespace alpharng */
