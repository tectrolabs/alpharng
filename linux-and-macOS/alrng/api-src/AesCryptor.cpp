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
 *    @brief Encrypts or decrypts session data using AES-GCM with 128 or 256 bit keys.
 */

#include "pch.h"
#include <AesCryptor.h>

namespace alpharng {


/**
 * Initialize the AES cipher
 *
 * @param[in] e_key_size AES key size
 */
void AesCryptor::initialize_aes(KeySize e_key_size) {
	m_ctx_enc = EVP_CIPHER_CTX_new();
	if (m_ctx_enc) {
		switch(e_key_size) {
		case KeySize::k128:
		default:
			m_enc_evpc = EVP_aes_128_gcm();
			break;
		case KeySize::k256:
			m_enc_evpc = EVP_aes_256_gcm();
			break;
		}
		if (EVP_EncryptInit_ex(m_ctx_enc, m_enc_evpc, nullptr, nullptr, nullptr) != 1) {
			return;
		}
		if (EVP_CIPHER_CTX_ctrl(m_ctx_enc, EVP_CTRL_AEAD_SET_IVLEN, sizeof(m_iv), nullptr) != 1) {
			return;
		}

	} else {
		return;
	}

	m_ctx_dec = EVP_CIPHER_CTX_new();
	if (m_ctx_dec) {
		switch(e_key_size) {
		case KeySize::k128:
		default:
			m_dec_evpc = EVP_aes_128_gcm();
			break;
		case KeySize::k256:
			m_dec_evpc = EVP_aes_256_gcm();
			break;
		}
		if (EVP_DecryptInit_ex(m_ctx_dec, m_dec_evpc, nullptr, nullptr, nullptr) != 1) {
			return;
		}
		if (EVP_CIPHER_CTX_ctrl(m_ctx_dec, EVP_CTRL_AEAD_SET_IVLEN, sizeof(m_iv), nullptr) != 1) {
			return;
		}
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
	m_key = new (nothrow)unsigned char[(int)m_e_key_size];
	if (m_key == nullptr) {
		m_initialized = false;
		return;
	}
	if (!RAND_bytes(m_key, (int)e_key_size)) {
		m_initialized = false;
		return;
	}

	if (!RAND_bytes((unsigned char *)&m_iv_serial_number, sizeof(m_iv_serial_number))) {
		m_initialized = false;
		return;
	}

	if (!RAND_bytes(m_aad, sizeof(m_aad))) {
		m_initialized = false;
		return;
	}

	initialize_aes(e_key_size);
}

AesCryptor::AesCryptor(KeySize e_key_size) {
	initialize(e_key_size);
}

AesCryptor::AesCryptor() {
	initialize(KeySize::k256);
}

/**
 * Encrypt bytes with AES using the current key.
 *
 * @param[in] in points to the bytes to be encrypted
 * @param[in] in_byte_count how many bytes to encrypt
 * @param[out] out points to location for encrypted bytes
 * @param[out] out_byte_count points to location for storing the number of encrypted bytes
 * @param[out] out_tag points to location for storing 16 bytes tag
 *
 * @return true if encrypted successfully
 */
bool AesCryptor::encrypt(const unsigned char *in, int in_byte_count, unsigned char *out, int *out_byte_count, unsigned char *out_tag) {
	if (!m_initialized || in_byte_count == 0 || in == nullptr || out == nullptr || out_byte_count == nullptr || out_tag == nullptr) {
		return false;
	}

	int aad_len;
	if (EVP_EncryptUpdate(m_ctx_enc, nullptr, &aad_len, m_aad, sizeof(m_aad)) != 1) {
		return false;
	}

	if (EVP_EncryptUpdate(m_ctx_enc, out, out_byte_count, in, in_byte_count) != 1) {
		return false;
	}

	int out_len;
	if (EVP_EncryptFinal_ex(m_ctx_enc, out, &out_len) != 1) {
		return false;
	}

	*out_byte_count += out_len;


	if (EVP_CIPHER_CTX_ctrl(m_ctx_enc, EVP_CTRL_AEAD_GET_TAG, 16, out_tag) != 1) {
		return false;
	}

	return true;
}

/**
 * Generate new IV value for encryption/decryption
 *
 * @return true if IV updated successfully
 */
bool AesCryptor::initialize_iv() {
	if (!m_initialized) {
		return false;
	}

	time_t seconds = time(nullptr);
	uint64_t token_sn = seconds << 32 | m_iv_serial_number++;

	if (!RAND_bytes((unsigned char*)&m_iv + sizeof(token_sn), sizeof(m_iv) - sizeof(token_sn))) {
		return false;
	}
	memcpy(m_iv, &token_sn, sizeof(token_sn));


	// Initialize key and IV

	if (EVP_EncryptInit_ex(m_ctx_enc, nullptr, nullptr, m_key, m_iv) != 1) {
		return false;
	}

	if (EVP_DecryptInit_ex(m_ctx_dec, nullptr, nullptr, m_key, m_iv) != 1) {
		return false;
	}

	return true;
}

/**
 * Decrypt bytes with AES using the current key
 *
 * @param[in] in points to the bytes to be decrypted
 * @param[in] in_byte_count how many bytes to decrypt
 * @param[out] out points to location for decrypted bytes
 * @param[out] out_byte_count points to location for storing the number of decrypted bytes
 * @param[in] in_tag points to location for 16 bytes tag
 *
 * @return true if decrypted successfully
 */
bool AesCryptor::decrypt(const unsigned char *in, int in_byte_count, unsigned char *out, int *out_byte_count, unsigned char *in_tag) {
	if (!m_initialized || in_byte_count == 0 || in == nullptr || out == nullptr || out_byte_count == nullptr || in_tag == nullptr) {
		return false;
	}

	int aad_len;
	if (EVP_DecryptUpdate(m_ctx_dec, nullptr, &aad_len, m_aad, sizeof(m_aad)) != 1) {
		return false;
	}

	if (EVP_DecryptUpdate(m_ctx_dec, out, out_byte_count, in, in_byte_count) != 1) {
		return false;
	}

	if (EVP_CIPHER_CTX_ctrl(m_ctx_dec, EVP_CTRL_AEAD_SET_TAG, 16, (void*)in_tag) != 1) {
		return false;
	}

	int out_len;
	if (EVP_DecryptFinal_ex(m_ctx_dec, out, &out_len) != 1) {
		return false;
	}
	*out_byte_count += out_len;

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

/**
 * Retrieve current IV.
 *
 * @param[out] out points to location with of 12 bytes to store the IV
 *
 * @return[out] true if IV retrieved successfully
 */
bool AesCryptor::get_iv(unsigned char* out) {
	if (!m_initialized) {
		return false;
	}
	memcpy(out, m_iv, sizeof(m_iv));
	return true;
}

/**
 * Retrieve current AAD.
 *
 * @param[out] out points to location of 16 bytes to store the AAD
 *
 * @return[out] true if AAD retrieved successfully
 */
bool AesCryptor::get_aad(unsigned char* out) {
	if (!m_initialized) {
		return false;
	}
	memcpy(out, m_aad, sizeof(m_aad));
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
