/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the Sha256 and Sha512 classes. The expected digests are known answers generated
 independently with Python's hashlib.
 */

/**
 *    @file test_sha.cpp
 *
 *    @brief Known answer tests for the SHA-256 and SHA-512 wrappers.
 */
#include <cstring>
#include <Sha256.h>
#include <Sha512.h>
#include "test_framework.h"

using namespace alpharng;

static const char c_fox[] = "The quick brown fox jumps over the lazy dog";

static std::string digest(ShaInterface &sha, const char *text) {
	unsigned char out[64] = {0};
	if (!sha.hash((const unsigned char *)text, (int)strlen(text), out)) {
		return "hash failed";
	}
	return tf::to_hex(out, (size_t)sha.get_hash_size());
}

TEST(sha256_hash_size) {
	Sha256 sha;
	CHECK_EQ(sha.get_hash_size(), 32);
}

TEST(sha256_empty_input) {
	Sha256 sha;
	CHECK_EQ(digest(sha, ""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(sha256_abc) {
	Sha256 sha;
	CHECK_EQ(digest(sha, "abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(sha256_pangram) {
	Sha256 sha;
	CHECK_EQ(digest(sha, c_fox), "d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");
}

TEST(sha256_different_inputs_differ) {
	Sha256 sha;
	CHECK(digest(sha, "abc") != digest(sha, "abd"));
}

TEST(sha256_is_repeatable) {
	Sha256 sha;
	CHECK_EQ(digest(sha, c_fox), digest(sha, c_fox));
}

TEST(sha512_hash_size) {
	Sha512 sha;
	CHECK_EQ(sha.get_hash_size(), 64);
}

TEST(sha512_empty_input) {
	Sha512 sha;
	CHECK_EQ(digest(sha, ""),
			"cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f"
			"63b931bd47417a81a538327af927da3e");
}

TEST(sha512_abc) {
	Sha512 sha;
	CHECK_EQ(digest(sha, "abc"),
			"ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd"
			"454d4423643ce80e2a9ac94fa54ca49f");
}

TEST(sha512_pangram) {
	Sha512 sha;
	CHECK_EQ(digest(sha, c_fox),
			"07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6"
			"e1bfd7097821233fa0538f3db854fee6");
}

TEST(sha512_different_inputs_differ) {
	Sha512 sha;
	CHECK(digest(sha, "abc") != digest(sha, "abd"));
}
