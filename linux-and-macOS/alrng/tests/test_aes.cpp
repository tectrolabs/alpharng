/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the AesCryptor class (AES-GCM used for the secure channel with the device).
 */

/**
 *    @file test_aes.cpp
 *
 *    @brief Round trip, tamper detection and wiring tests for AES-128-GCM and AES-256-GCM.
 */
#include <cstring>
#include <openssl/evp.h>
#include <AesCryptor.h>
#include "test_framework.h"

using namespace alpharng;

static std::vector<unsigned char> sample_plaintext(size_t size) {
	std::vector<unsigned char> data(size);
	for (size_t i = 0; i < size; ++i) {
		data[i] = (unsigned char)(i * 7 + 3);
	}
	return data;
}

static void check_round_trip(KeySize key_size, int expected_key_bytes) {
	AesCryptor aes(key_size);
	REQUIRE(aes.is_initialized());
	CHECK_EQ(aes.get_key_size_bytes(), expected_key_bytes);
	REQUIRE(aes.initialize_iv());

	std::vector<unsigned char> plain = sample_plaintext(200);
	std::vector<unsigned char> cipher(plain.size());
	std::vector<unsigned char> back(plain.size());
	unsigned char tag[16] = {0};
	int cipher_len = 0;
	int back_len = 0;

	REQUIRE(aes.encrypt(plain.data(), (int)plain.size(), cipher.data(), &cipher_len, tag));
	CHECK_EQ(cipher_len, (int)plain.size());
	CHECK(cipher != plain);

	REQUIRE(aes.decrypt(cipher.data(), cipher_len, back.data(), &back_len, tag));
	CHECK_EQ(back_len, (int)plain.size());
	CHECK(back == plain);
}

// Encrypts with a fresh IV, flips one bit as 'tamper' says, and tells whether decryption still succeeds
static bool decrypts_after(AesCryptor &aes, bool tamper_cipher, bool tamper_tag) {
	std::vector<unsigned char> plain = sample_plaintext(64);
	std::vector<unsigned char> cipher(plain.size());
	std::vector<unsigned char> back(plain.size());
	unsigned char tag[16] = {0};
	int cipher_len = 0;
	int back_len = 0;

	if (!aes.initialize_iv() || !aes.encrypt(plain.data(), (int)plain.size(), cipher.data(), &cipher_len, tag)) {
		return false;
	}
	if (tamper_cipher) {
		cipher[10] ^= 0x01;
	}
	if (tamper_tag) {
		tag[3] ^= 0x01;
	}
	return aes.decrypt(cipher.data(), cipher_len, back.data(), &back_len, tag);
}

TEST(aes_128_round_trip) {
	check_round_trip(KeySize::k128, 16);
}

TEST(aes_256_round_trip) {
	check_round_trip(KeySize::k256, 32);
}

TEST(aes_default_constructor_uses_256_bit_key) {
	AesCryptor aes;
	REQUIRE(aes.is_initialized());
	CHECK_EQ(aes.get_key_size_bytes(), 32);
}

TEST(aes_untampered_data_decrypts) {
	AesCryptor aes(KeySize::k256);
	REQUIRE(aes.is_initialized());
	CHECK(decrypts_after(aes, false, false));
}

TEST(aes_modified_ciphertext_is_rejected) {
	AesCryptor aes(KeySize::k256);
	REQUIRE(aes.is_initialized());
	CHECK(!decrypts_after(aes, true, false));
}

TEST(aes_modified_tag_is_rejected) {
	AesCryptor aes(KeySize::k128);
	REQUIRE(aes.is_initialized());
	CHECK(!decrypts_after(aes, false, true));
}

TEST(aes_each_message_gets_a_new_iv) {
	AesCryptor aes(KeySize::k256);
	REQUIRE(aes.is_initialized());
	unsigned char first[12] = {0};
	unsigned char second[12] = {0};
	REQUIRE(aes.initialize_iv());
	REQUIRE(aes.get_iv(first));
	REQUIRE(aes.initialize_iv());
	REQUIRE(aes.get_iv(second));
	CHECK(std::memcmp(first, second, sizeof(first)) != 0);
}

TEST(aes_two_instances_use_different_keys) {
	AesCryptor a(KeySize::k256);
	AesCryptor b(KeySize::k256);
	REQUIRE(a.is_initialized() && b.is_initialized());
	unsigned char key_a[32] = {0};
	unsigned char key_b[32] = {0};
	REQUIRE(a.get_key(key_a));
	REQUIRE(b.get_key(key_b));
	CHECK(std::memcmp(key_a, key_b, sizeof(key_a)) != 0);
}

// The output must equal AES-GCM computed directly with OpenSSL using the same key, IV and AAD,
// which checks that the class wires the 12 byte IV, the AAD and the 16 byte tag correctly.
static void check_matches_openssl(KeySize key_size) {
	AesCryptor aes(key_size);
	REQUIRE(aes.is_initialized());
	REQUIRE(aes.initialize_iv());

	unsigned char key[32] = {0};
	unsigned char iv[12] = {0};
	unsigned char aad[16] = {0};
	REQUIRE(aes.get_key(key));
	REQUIRE(aes.get_iv(iv));
	REQUIRE(aes.get_aad(aad));

	std::vector<unsigned char> plain = sample_plaintext(100);
	std::vector<unsigned char> cipher(plain.size());
	unsigned char tag[16] = {0};
	int cipher_len = 0;
	REQUIRE(aes.encrypt(plain.data(), (int)plain.size(), cipher.data(), &cipher_len, tag));

	std::vector<unsigned char> expected(plain.size());
	unsigned char expected_tag[16] = {0};
	int len = 0;
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
	REQUIRE(ctx != nullptr);
	const EVP_CIPHER *cipher_type = key_size == KeySize::k128 ? EVP_aes_128_gcm() : EVP_aes_256_gcm();
	bool ok = EVP_EncryptInit_ex(ctx, cipher_type, nullptr, nullptr, nullptr) == 1
			&& EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_IVLEN, 12, nullptr) == 1
			&& EVP_EncryptInit_ex(ctx, nullptr, nullptr, key, iv) == 1
			&& EVP_EncryptUpdate(ctx, nullptr, &len, aad, sizeof(aad)) == 1
			&& EVP_EncryptUpdate(ctx, expected.data(), &len, plain.data(), (int)plain.size()) == 1
			&& EVP_EncryptFinal_ex(ctx, expected.data() + len, &len) == 1
			&& EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG, 16, expected_tag) == 1;
	EVP_CIPHER_CTX_free(ctx);
	REQUIRE(ok);

	CHECK_EQ(tf::to_hex(cipher.data(), cipher.size()), tf::to_hex(expected.data(), expected.size()));
	CHECK_EQ(tf::to_hex(tag, sizeof(tag)), tf::to_hex(expected_tag, sizeof(expected_tag)));
}

TEST(aes_128_matches_direct_openssl_gcm) {
	check_matches_openssl(KeySize::k128);
}

TEST(aes_256_matches_direct_openssl_gcm) {
	check_matches_openssl(KeySize::k256);
}

TEST(aes_rejects_invalid_arguments) {
	AesCryptor aes(KeySize::k256);
	REQUIRE(aes.is_initialized());
	REQUIRE(aes.initialize_iv());
	unsigned char data[16] = {1, 2, 3};
	unsigned char out[16] = {0};
	unsigned char tag[16] = {0};
	int out_len = 0;
	CHECK(!aes.encrypt(data, 0, out, &out_len, tag));
	CHECK(!aes.encrypt(nullptr, 16, out, &out_len, tag));
	CHECK(!aes.encrypt(data, 16, nullptr, &out_len, tag));
	CHECK(!aes.encrypt(data, 16, out, nullptr, tag));
	CHECK(!aes.encrypt(data, 16, out, &out_len, nullptr));
	CHECK(!aes.decrypt(data, 0, out, &out_len, tag));
	CHECK(!aes.decrypt(nullptr, 16, out, &out_len, tag));
	CHECK(!aes.decrypt(data, 16, out, &out_len, nullptr));
}

TEST(aes_without_a_key_size_is_not_usable) {
	AesCryptor aes(KeySize::None);
	CHECK(!aes.is_initialized());
	CHECK(!aes.initialize_iv());
	unsigned char data[16] = {1};
	unsigned char out[16] = {0};
	unsigned char tag[16] = {0};
	int out_len = 0;
	CHECK(!aes.encrypt(data, 16, out, &out_len, tag));
}
