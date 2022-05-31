/**
 Copyright (C) 2014-2022 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for establishing a secure session between the host computer and the AlphaRNG device using RSA PK encryption.

 It uses OpenSSL library.

 */

/**
 *    @file RsaCryptor.h
 *    @date 05/31/2022
 *    @Author: Andrian Belinski
 *    @version 1.3
 *
 *    @brief Used for establishing a secure session between the host computer and the AlphaRNG device suing RSA PK encryption.
 */

#ifndef RSACRYPTOR_H_
#define RSACRYPTOR_H_

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <string>
#include <fstream>

using namespace std;

namespace alpharng {

class RsaCryptor {
public:
	RsaCryptor();
	explicit RsaCryptor(int key_size);
	RsaCryptor(const RsaCryptor &cryptor) = delete;
	RsaCryptor & operator=(const RsaCryptor &cryptor) = delete;
	RsaCryptor(const string &key_file_name, bool is_public);
	RsaCryptor(const unsigned char *key, int key_size_bytes, bool is_public);
	bool is_initialized() const;
	bool export_private_key_to_file(const string &key_file_name);
	bool export_public_key_to_file(const string &key_file_name);
	bool encrypt_with_public_key(unsigned char *in, int in_size_bytes, unsigned char *out, int *out_size_bytes);
	bool decrypt_with_public_key(unsigned char *in, int in_size_bytes, unsigned char *out, int *out_size_bytes);
	bool encrypt_with_private_key(unsigned char *in, int in_size_bytes, unsigned char *out, int *out_size_bytes);
	bool decrypt_with_private_key(unsigned char *in, int in_size_bytes, unsigned char *out, int *out_size_bytes);
	bool is_public_key_file() const {return m_is_public_key_file;}
	virtual ~RsaCryptor();
private:
	void crete_new_key(int key_size);
	void initialize_with_key(const unsigned char* key, int key_size_bytes, bool is_public);
#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	bool evp_key_encrypt(unsigned char *in, int in_size_bytes, unsigned char *out, int *out_size_bytes);
	bool evp_key_decrypt(unsigned char *in, int in_size_bytes, unsigned char *out, int *out_size_bytes);
#endif

private:
#if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L
	EVP_PKEY *m_rsa = nullptr;
#else
    RSA *m_rsa = nullptr;
    BIGNUM *m_bignum = nullptr;
#endif

    BIO *m_kbio_rsa = nullptr;
    bool m_is_key_initialized = false;
    int m_padding = RSA_NO_PADDING;
    bool m_is_public_key_file = false;
	unsigned char* m_file_pub_key_bytes = nullptr;
	const int c_m_file_pub_key_max_size_bytes = 1024*2;
};

} /* namespace alpharng */

#endif /* RSACRYPTOR_H_ */
