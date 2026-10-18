/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the RsaCryptor class. The class uses raw RSA (no padding), so every block it processes
 must be exactly as long as the key, and its numeric value must be smaller than the modulus. The tests
 start each block with a zero byte to guarantee that.

 The shipped programs only encrypt with a public key (the embedded device keys or a key file); the tests
 for that path are listed first. The others cover generated keys, key files and decryption.
 */

/**
 *    @file test_rsa.cpp
 *
 *    @brief Key generation, embedded key, encryption and key file tests for RsaCryptor.
 */
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <openssl/opensslv.h>
#include <RsaCryptor.h>
#include <RsaKeyRepo.h>
#include "test_framework.h"

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#include <openssl/bn.h>
#include <openssl/core_names.h>
#include <openssl/decoder.h>
#include <openssl/evp.h>
#endif

using namespace alpharng;

static const int c_key_bits = 1024;
static const int c_block_bytes = c_key_bits / 8;

// A block of the given size that starts with a zero byte, so its value is smaller than any modulus
static std::vector<unsigned char> sample_block(int size = c_block_bytes) {
	std::vector<unsigned char> block((size_t)size);
	block[0] = 0;
	for (size_t i = 1; i < block.size(); ++i) {
		block[i] = (unsigned char)(i * 13 + 5);
	}
	return block;
}

// A unique temporary file name that is removed when the object goes out of scope
struct TempFile {
	std::string path;
	TempFile() {
		char name[] = "/tmp/alrng_test_key_XXXXXX";
		int fd = mkstemp(name);
		if (fd >= 0) {
			close(fd);
			path = name;
		}
	}
	~TempFile() {
		if (!path.empty()) {
			std::remove(path.c_str());
		}
	}
};

static bool public_encrypt(RsaCryptor &rsa, const std::vector<unsigned char> &plain, std::vector<unsigned char> &cipher) {
	cipher.assign(plain.size(), 0);
	int cipher_len = 0;
	std::vector<unsigned char> in = plain;
	return rsa.encrypt_with_public_key(in.data(), (int)in.size(), cipher.data(), &cipher_len) && cipher_len == (int)plain.size();
}

// --- The path used by the shipped programs: the embedded device public keys -------------------------------

static void check_embedded_key(const unsigned char *pem, unsigned int pem_len, RsaKeySize expected_size) {
	RsaCryptor rsa(pem, (int)pem_len, true);
	REQUIRE(rsa.is_initialized());
	CHECK(rsa.get_rsa_key_size() == expected_size);

	int block = (int)expected_size;
	std::vector<unsigned char> plain = sample_block(block);
	std::vector<unsigned char> cipher;
	REQUIRE(public_encrypt(rsa, plain, cipher));
	CHECK(cipher != plain);

	// Raw RSA without padding is deterministic
	std::vector<unsigned char> again;
	REQUIRE(public_encrypt(rsa, plain, again));
	CHECK(cipher == again);
}

TEST(rsa_embedded_1024_bit_device_key) {
	RsaKeyRepo repo;
	check_embedded_key(repo.c_rsapub_1024_pem, repo.c_rsapub_1024_pem_len, RsaKeySize::rsa1024);
}

TEST(rsa_embedded_2048_bit_device_key) {
	RsaKeyRepo repo;
	check_embedded_key(repo.c_rsapub_2048_pem, repo.c_rsapub_2048_pem_len, RsaKeySize::rsa2048);
}

TEST(rsa_embedded_3072_bit_device_key) {
	RsaKeyRepo repo;
	check_embedded_key(repo.c_rsapub_3072_pem, repo.c_rsapub_3072_pem_len, RsaKeySize::rsa3072);
}

TEST(rsa_embedded_4096_bit_device_key) {
	RsaKeyRepo repo;
	check_embedded_key(repo.c_rsapub_4096_pem, repo.c_rsapub_4096_pem_len, RsaKeySize::rsa4096);
}

TEST(rsa_embedded_keys_are_different) {
	RsaKeyRepo repo;
	RsaCryptor a(repo.c_rsapub_2048_pem, (int)repo.c_rsapub_2048_pem_len, true);
	RsaCryptor b(repo.c_rsapub_3072_pem, (int)repo.c_rsapub_3072_pem_len, true);
	REQUIRE(a.is_initialized() && b.is_initialized());
	CHECK(a.get_rsa_key_size() != b.get_rsa_key_size());
}

// --- Generated keys and key files ---------------------------------------------------------------------------

TEST(rsa_generated_key_is_usable) {
	RsaCryptor rsa(c_key_bits);
	CHECK(rsa.is_initialized());
}

TEST(rsa_generated_key_reports_its_size) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	CHECK(rsa.get_rsa_key_size() == RsaKeySize::rsa1024);
}

TEST(rsa_impossible_key_size_is_not_usable) {
	// OpenSSL cannot generate a 100 bit RSA key, so the object must not claim to be ready
	RsaCryptor rsa(100);
	CHECK(!rsa.is_initialized());
}

TEST(rsa_public_encrypt_gives_key_sized_output) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	std::vector<unsigned char> plain = sample_block();
	std::vector<unsigned char> cipher;
	REQUIRE(public_encrypt(rsa, plain, cipher));
	CHECK(cipher != plain);
}

TEST(rsa_different_keys_give_different_ciphertext) {
	RsaCryptor a(c_key_bits);
	RsaCryptor b(c_key_bits);
	REQUIRE(a.is_initialized() && b.is_initialized());
	std::vector<unsigned char> plain = sample_block();
	std::vector<unsigned char> cipher_a;
	std::vector<unsigned char> cipher_b;
	REQUIRE(public_encrypt(a, plain, cipher_a));
	REQUIRE(public_encrypt(b, plain, cipher_b));
	CHECK(cipher_a != cipher_b);
}

TEST(rsa_rejects_invalid_arguments) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	std::vector<unsigned char> block = sample_block();
	std::vector<unsigned char> out((size_t)c_block_bytes);
	int out_len = 0;
	CHECK(!rsa.encrypt_with_public_key(block.data(), 0, out.data(), &out_len));
	CHECK(!rsa.encrypt_with_public_key(nullptr, c_block_bytes, out.data(), &out_len));
	CHECK(!rsa.encrypt_with_public_key(block.data(), c_block_bytes, nullptr, &out_len));
	CHECK(!rsa.decrypt_with_private_key(block.data(), 0, out.data(), &out_len));
	CHECK(!rsa.decrypt_with_private_key(nullptr, c_block_bytes, out.data(), &out_len));
}

TEST(rsa_public_key_file_loads) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	TempFile file;
	REQUIRE(!file.path.empty());
	REQUIRE(rsa.export_public_key_to_file(file.path));

	RsaCryptor loaded(file.path, true);
	REQUIRE(loaded.is_initialized());
	CHECK(loaded.is_public_key_file());
	CHECK(loaded.get_rsa_key_size() == RsaKeySize::rsa1024);
}

TEST(rsa_loaded_public_key_encrypts_like_the_original) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	TempFile file;
	REQUIRE(!file.path.empty());
	REQUIRE(rsa.export_public_key_to_file(file.path));
	RsaCryptor loaded(file.path, true);
	REQUIRE(loaded.is_initialized());

	std::vector<unsigned char> plain = sample_block();
	std::vector<unsigned char> from_original;
	std::vector<unsigned char> from_loaded;
	REQUIRE(public_encrypt(rsa, plain, from_original));
	REQUIRE(public_encrypt(loaded, plain, from_loaded));
	CHECK(from_original == from_loaded);
}

TEST(rsa_private_key_file_loads) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	TempFile file;
	REQUIRE(!file.path.empty());
	REQUIRE(rsa.export_private_key_to_file(file.path));

	RsaCryptor loaded(file.path, false);
	REQUIRE(loaded.is_initialized());
	CHECK(!loaded.is_public_key_file());
	CHECK(loaded.get_rsa_key_size() == RsaKeySize::rsa1024);
}

TEST(rsa_missing_key_file_is_not_usable) {
	RsaCryptor loaded("/nonexistent/alrng_no_such_key.pem", true);
	CHECK(!loaded.is_initialized());
	std::vector<unsigned char> block = sample_block();
	std::vector<unsigned char> out((size_t)c_block_bytes);
	int out_len = 0;
	CHECK(!loaded.encrypt_with_public_key(block.data(), c_block_bytes, out.data(), &out_len));
}

TEST(rsa_garbage_key_file_is_not_usable) {
	TempFile file;
	REQUIRE(!file.path.empty());
	FILE *f = std::fopen(file.path.c_str(), "w");
	REQUIRE(f != nullptr);
	std::fputs("this is not a PEM key\n", f);
	std::fclose(f);
	RsaCryptor loaded(file.path, true);
	CHECK(!loaded.is_initialized());
}

// --- Independent check of the arithmetic (OpenSSL 3 only) ---------------------------------------------------

#if OPENSSL_VERSION_NUMBER >= 0x30000000L
// Raw RSA computed by hand: plain^e mod n, with e and n read straight from the exported public key file
static bool raw_rsa_public(const std::string &pem_path, const std::vector<unsigned char> &plain, std::vector<unsigned char> &out) {
	EVP_PKEY *pkey = nullptr;
	OSSL_DECODER_CTX *decoder = OSSL_DECODER_CTX_new_for_pkey(&pkey, "PEM", nullptr, "RSA", EVP_PKEY_PUBLIC_KEY, nullptr, nullptr);
	if (decoder == nullptr) {
		return false;
	}
	FILE *f = std::fopen(pem_path.c_str(), "r");
	bool ok = f != nullptr && OSSL_DECODER_from_fp(decoder, f) == 1 && pkey != nullptr;
	if (f != nullptr) {
		std::fclose(f);
	}
	OSSL_DECODER_CTX_free(decoder);
	if (!ok) {
		return false;
	}

	BIGNUM *n = nullptr;
	BIGNUM *e = nullptr;
	ok = EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_N, &n) == 1 && EVP_PKEY_get_bn_param(pkey, OSSL_PKEY_PARAM_RSA_E, &e) == 1;
	BN_CTX *ctx = BN_CTX_new();
	BIGNUM *m = BN_bin2bn(plain.data(), (int)plain.size(), nullptr);
	BIGNUM *r = BN_new();
	ok = ok && ctx != nullptr && m != nullptr && r != nullptr && BN_mod_exp(r, m, e, n, ctx) == 1;
	if (ok) {
		out.assign(plain.size(), 0);
		ok = BN_bn2binpad(r, out.data(), (int)out.size()) == (int)out.size();
	}
	BN_free(r);
	BN_free(m);
	BN_CTX_free(ctx);
	BN_free(e);
	BN_free(n);
	EVP_PKEY_free(pkey);
	return ok;
}

TEST(rsa_public_encrypt_matches_raw_rsa_arithmetic) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	TempFile file;
	REQUIRE(!file.path.empty());
	REQUIRE(rsa.export_public_key_to_file(file.path));

	std::vector<unsigned char> plain = sample_block();
	std::vector<unsigned char> cipher;
	std::vector<unsigned char> expected;
	REQUIRE(public_encrypt(rsa, plain, cipher));
	REQUIRE(raw_rsa_public(file.path, plain, expected));
	CHECK_EQ(tf::to_hex(cipher.data(), cipher.size()), tf::to_hex(expected.data(), expected.size()));
}
#endif

// --- Decryption ----------------------------------------------------------------------------------------------

TEST(rsa_public_encrypt_private_decrypt_round_trip) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	std::vector<unsigned char> plain = sample_block();
	std::vector<unsigned char> cipher;
	std::vector<unsigned char> back((size_t)c_block_bytes);
	int back_len = 0;
	REQUIRE(public_encrypt(rsa, plain, cipher));
	REQUIRE(rsa.decrypt_with_private_key(cipher.data(), (int)cipher.size(), back.data(), &back_len));
	CHECK_EQ(back_len, c_block_bytes);
	CHECK(back == plain);
}

TEST(rsa_private_encrypt_public_decrypt_round_trip) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	std::vector<unsigned char> plain = sample_block();
	std::vector<unsigned char> cipher((size_t)c_block_bytes);
	std::vector<unsigned char> back((size_t)c_block_bytes);
	int cipher_len = 0;
	int back_len = 0;
	REQUIRE(rsa.encrypt_with_private_key(plain.data(), c_block_bytes, cipher.data(), &cipher_len));
	REQUIRE(rsa.decrypt_with_public_key(cipher.data(), cipher_len, back.data(), &back_len));
	CHECK(back == plain);
}

TEST(rsa_key_file_round_trip) {
	RsaCryptor rsa(c_key_bits);
	REQUIRE(rsa.is_initialized());
	TempFile pub_file;
	TempFile priv_file;
	REQUIRE(!pub_file.path.empty() && !priv_file.path.empty());
	REQUIRE(rsa.export_public_key_to_file(pub_file.path));
	REQUIRE(rsa.export_private_key_to_file(priv_file.path));

	RsaCryptor pub(pub_file.path, true);
	RsaCryptor priv(priv_file.path, false);
	REQUIRE(pub.is_initialized() && priv.is_initialized());

	std::vector<unsigned char> plain = sample_block();
	std::vector<unsigned char> cipher;
	std::vector<unsigned char> back((size_t)c_block_bytes);
	int back_len = 0;
	REQUIRE(public_encrypt(pub, plain, cipher));
	REQUIRE(priv.decrypt_with_private_key(cipher.data(), (int)cipher.size(), back.data(), &back_len));
	CHECK(back == plain);
}
