/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the HmacSha256, HmacSha1 and HmacMD5 classes. The expected MACs are known answers
 generated independently with Python's hmac module, using the keys 00 01 02 ... (key size bytes).
 */

/**
 *    @file test_hmac.cpp
 *
 *    @brief Known answer and key handling tests for the HMAC wrappers.
 */
#include <cstring>
#include <HmacSha256.h>
#include <HmacSha1.h>
#include <HmacMD5.h>
#include "test_framework.h"

using namespace alpharng;

static const char c_fox[] = "The quick brown fox jumps over the lazy dog";

// Keys of the size each class expects: 00 01 02 ...
static std::vector<unsigned char> sequential_key(int size) {
	std::vector<unsigned char> key((size_t)size);
	for (int i = 0; i < size; ++i) {
		key[(size_t)i] = (unsigned char)i;
	}
	return key;
}

static std::string mac_of(HmacInterface &hmac, const char *text) {
	unsigned char out[64] = {0};
	if (!hmac.hmac((const unsigned char *)text, (int)strlen(text), out)) {
		return "hmac failed";
	}
	return tf::to_hex(out, (size_t)hmac.get_mac_size());
}

// Checks shared by all three classes
static void check_key_handling(HmacInterface &hmac, int key_size) {
	REQUIRE(hmac.is_initialized());
	CHECK_EQ(hmac.get_mac_size(), key_size);

	std::vector<unsigned char> key = sequential_key(key_size);
	CHECK(hmac.set_key(key.data(), key_size));
	unsigned char stored[64] = {0};
	CHECK(hmac.get_mac_key(stored));
	CHECK_EQ(tf::to_hex(stored, (size_t)key_size), tf::to_hex(key.data(), (size_t)key_size));

	// A key of the wrong size must be refused and must not replace the current key
	std::vector<unsigned char> wrong = sequential_key(key_size + 1);
	CHECK(!hmac.set_key(wrong.data(), key_size + 1));
	CHECK(!hmac.set_key(wrong.data(), key_size - 1));
	CHECK(hmac.get_mac_key(stored));
	CHECK_EQ(tf::to_hex(stored, (size_t)key_size), tf::to_hex(key.data(), (size_t)key_size));

	// The MAC depends on the message and on the key
	std::string first = mac_of(hmac, c_fox);
	CHECK(first != mac_of(hmac, "The quick brown fox jumps over the lazy cat"));
	CHECK(hmac.generate_new_key());
	CHECK(first != mac_of(hmac, c_fox));
}

static void check_new_keys_differ(HmacInterface &hmac, int key_size) {
	unsigned char first[64] = {0};
	unsigned char second[64] = {0};
	REQUIRE(hmac.generate_new_key());
	REQUIRE(hmac.get_mac_key(first));
	REQUIRE(hmac.generate_new_key());
	REQUIRE(hmac.get_mac_key(second));
	CHECK(std::memcmp(first, second, (size_t)key_size) != 0);
}

TEST(hmac_sha256_known_answer) {
	HmacSha256 hmac;
	REQUIRE(hmac.is_initialized());
	std::vector<unsigned char> key = sequential_key(32);
	REQUIRE(hmac.set_key(key.data(), 32));
	CHECK_EQ(mac_of(hmac, c_fox), "f87ad256151fc7b4c5dffa4adb3ebe911a8eeb8a8ebdee3c2a4a8e5f5ec02c32");
}

TEST(hmac_sha256_key_handling) {
	HmacSha256 hmac;
	check_key_handling(hmac, 32);
}

TEST(hmac_sha256_new_keys_differ) {
	HmacSha256 hmac;
	check_new_keys_differ(hmac, 32);
}

TEST(hmac_sha1_known_answer) {
	HmacSha1 hmac;
	REQUIRE(hmac.is_initialized());
	std::vector<unsigned char> key = sequential_key(20);
	REQUIRE(hmac.set_key(key.data(), 20));
	CHECK_EQ(mac_of(hmac, c_fox), "2d0f4cf304f8b20d41e9c37465f697f207e84661");
}

TEST(hmac_sha1_key_handling) {
	HmacSha1 hmac;
	check_key_handling(hmac, 20);
}

TEST(hmac_sha1_new_keys_differ) {
	HmacSha1 hmac;
	check_new_keys_differ(hmac, 20);
}

TEST(hmac_md5_known_answer) {
	HmacMD5 hmac;
	REQUIRE(hmac.is_initialized());
	std::vector<unsigned char> key = sequential_key(16);
	REQUIRE(hmac.set_key(key.data(), 16));
	CHECK_EQ(mac_of(hmac, c_fox), "0d7fd86c3dd67264d5873c72716a3aa7");
}

TEST(hmac_md5_key_handling) {
	HmacMD5 hmac;
	check_key_handling(hmac, 16);
}

TEST(hmac_md5_new_keys_differ) {
	HmacMD5 hmac;
	check_new_keys_differ(hmac, 16);
}
