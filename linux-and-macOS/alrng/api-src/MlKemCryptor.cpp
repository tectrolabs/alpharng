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
 *    @file MlKemCryptor.cpp
 *    @date 11/03/2026
 *    @Author: Andrian Belinski
 *    @version 1.1
 *
 *    @brief Used for establishing a secure session between the host computer and the AlphaRNG PQ device using
 *    ML-KEM-1024 key encapsulation.
 *
 */

#include <MlKemCryptor.h>
#include <fstream>
#include <cstring>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/crypto.h>
#include "mlkem/mlkem_native.h"

using namespace std;

namespace alpharng {

namespace {

// DER prefix of an ML-KEM-1024 SubjectPublicKeyInfo (RFC 9935), followed by the 1568 key bytes:
// SEQUENCE { SEQUENCE { OID 2.16.840.1.101.3.4.4.3 }, BIT STRING { 0 unused bits, key } }
const unsigned char k_mlkem1024_spki_prefix[] = {
		0x30, 0x82, 0x06, 0x32, 0x30, 0x0b, 0x06, 0x09, 0x60, 0x86, 0x48, 0x01,
		0x65, 0x03, 0x04, 0x04, 0x03, 0x03, 0x82, 0x06, 0x21, 0x00
};

static_assert(MlKemCryptor::c_public_key_size_bytes == MLKEM1024_PUBLICKEYBYTES, "public key size mismatch");
static_assert(MlKemCryptor::c_capsule_size_bytes == MLKEM1024_CIPHERTEXTBYTES, "capsule size mismatch");
static_assert(MlKemCryptor::c_shared_secret_size_bytes == MLKEM_BYTES, "shared secret size mismatch");

} /* namespace */

/**
 * Construct using a public key provided as bytes, either PEM encoded or as the raw 1568-byte key
 *
 * @param[in] key points to location of the key
 * @param[in] key_size_bytes size of the key in bytes
 */
MlKemCryptor::MlKemCryptor(const unsigned char *key, int key_size_bytes) {
	initialize_with_key(key, key_size_bytes);
}

/**
 * Construct using a public key file, either PEM encoded or containing the raw 1568-byte key
 *
 * @param[in] public_key_file_name file pathname to a public key file
 */
MlKemCryptor::MlKemCryptor(const string &public_key_file_name) {
	ifstream is_file(public_key_file_name.c_str(), ios::in | ios::binary);
	if (!is_file.good()) {
		return;
	}

	is_file.seekg(0, ifstream::end);
	int length = (int)is_file.tellg();
	is_file.seekg(0, ifstream::beg);

	if (length <= 0 || length > c_m_file_key_max_size_bytes) {
		return;
	}

	unsigned char *file_bytes = new (nothrow) unsigned char[length];
	if (file_bytes == nullptr) {
		return;
	}

	is_file.read((char*)file_bytes, length);
	if (is_file.good()) {
		initialize_with_key(file_bytes, length);
	}
	delete[] file_bytes;

	m_is_public_key_file = m_is_key_initialized;
}

/**
 * Initialize using a public key provided as bytes
 *
 * @param[in] key points to location of the key, PEM encoded or the raw 1568-byte key
 * @param[in] key_size_bytes size of the key in bytes
 */
void MlKemCryptor::initialize_with_key(const unsigned char *key, int key_size_bytes) {
	m_is_key_initialized = false;
	if (key == nullptr || key_size_bytes <= 0) {
		return;
	}

	if (key_size_bytes == c_public_key_size_bytes) {
		memcpy(m_public_key, key, c_public_key_size_bytes);
	} else if (!decode_pem_public_key(key, key_size_bytes)) {
		return;
	}

	// FIPS 203 encapsulation key check (all coefficients must be reduced modulo q)
	if (mlkem_check_pk(m_public_key) != 0) {
		return;
	}
	m_is_key_initialized = true;
}

/**
 * Decode a PEM encoded ML-KEM-1024 public key into m_public_key
 *
 * @param[in] pem points to the PEM text
 * @param[in] pem_size_bytes size of the PEM text in bytes
 *
 * @return true if a valid ML-KEM-1024 public key was found
 */
bool MlKemCryptor::decode_pem_public_key(const unsigned char *pem, int pem_size_bytes) {
	BIO *bio = BIO_new_mem_buf(pem, pem_size_bytes);
	if (bio == nullptr) {
		return false;
	}

	char *name = nullptr;
	char *header = nullptr;
	unsigned char *der = nullptr;
	long der_size_bytes = 0;
	bool status = false;

	if (PEM_read_bio(bio, &name, &header, &der, &der_size_bytes) == 1
			&& strcmp(name, "PUBLIC KEY") == 0
			&& der_size_bytes == (long)(sizeof(k_mlkem1024_spki_prefix) + c_public_key_size_bytes)
			&& memcmp(der, k_mlkem1024_spki_prefix, sizeof(k_mlkem1024_spki_prefix)) == 0) {
		memcpy(m_public_key, der + sizeof(k_mlkem1024_spki_prefix), c_public_key_size_bytes);
		status = true;
	}

	OPENSSL_free(name);
	OPENSSL_free(header);
	OPENSSL_free(der);
	BIO_free(bio);
	return status;
}

/**
 * Generate a new random shared secret and the capsule that carries it to the holder of the private key.
 * Each call produces a different shared secret and capsule.
 *
 * @param[out] capsule points to location for the capsule (ML-KEM ciphertext)
 * @param[in] capsule_size_bytes size of the capsule buffer, at least c_capsule_size_bytes
 * @param[out] shared_secret points to location for the shared secret
 * @param[in] shared_secret_size_bytes size of the shared secret buffer, at least c_shared_secret_size_bytes
 *
 * @return true if encapsulation was successful
 */
bool MlKemCryptor::encapsulate(unsigned char *capsule, int capsule_size_bytes, unsigned char *shared_secret, int shared_secret_size_bytes) {
	if (!m_is_key_initialized || capsule == nullptr || shared_secret == nullptr
			|| capsule_size_bytes < c_capsule_size_bytes
			|| shared_secret_size_bytes < c_shared_secret_size_bytes) {
		return false;
	}
	if (mlkem_enc(capsule, shared_secret, m_public_key) != 0) {
		OPENSSL_cleanse(shared_secret, c_shared_secret_size_bytes);
		return false;
	}
	return true;
}

/**
 * De-allocate resources.
 */
MlKemCryptor::~MlKemCryptor() {
}

} /* namespace alpharng */
