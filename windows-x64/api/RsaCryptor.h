/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for establishing a secure session between the host computer and the AlphaRNG device using RSA PK encryption.

 It uses OpenSSL library.

 */

/**
 *    @file RsaCryptor.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Used for establishing a secure session between the host computer and the AlphaRNG device suing RSA PK encryption.
 */

#ifndef RSACRYPTOR_H_
#define RSACRYPTOR_H_

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <string>

using namespace std;

namespace alpharng {

class RsaCryptor {
public:
	RsaCryptor();
	RsaCryptor(int key_size);
	RsaCryptor(const string &key_file_name, bool is_public);
	RsaCryptor(const unsigned char *key, int key_size_bytes, bool is_public);
	bool is_initialized();
	bool export_private_key_to_file(const string &key_file_name);
	bool export_public_key_to_file(const string &key_file_name);
	bool encrypt_with_public_key(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes);
	bool decrypt_with_public_key(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes);
	bool encrypt_with_private_key(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes);
	bool decrypt_with_private_key(unsigned char *in, int in_size_bytes,	unsigned char *out, int *out_size_bytes);
	bool is_public_key_file() {return m_is_public_key_file;}
	virtual ~RsaCryptor();
private:
	void initialize();
	void crete_new_key(int key_size);
private:
    RSA *m_rsa = nullptr;
    BIO *m_kbio_rsa = nullptr;
    BIGNUM *m_bignum = nullptr;
    bool m_is_key_initialized = false;
    int m_padding;
    bool m_is_public_key_file = false;
};

} /* namespace alpharng */

#endif /* RSACRYPTOR_H_ */
