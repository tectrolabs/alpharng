/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

/**
 *    @file test_provider_settings.cpp
 *
 *    @brief Reading and checking the AlphaRNG provider settings (cipher, mac, rsa, key_file, device, data, stat_tests).
 */
#include <map>
#include <string>

#include "settings.h"
#include "test_framework.h"

using namespace alrng_provider;

static bool parse(const std::map<std::string, std::string> &values, Settings &out, std::string &error) {
	return parse_settings([&](const char *name) -> const char * {
		auto found = values.find(name);
		return found == values.end() ? nullptr : found->second.c_str();
	}, out, error);
}

TEST(nothing_configured_gives_the_alrng_defaults) {
	Settings s;
	std::string error;
	REQUIRE(parse({}, s, error));
	CHECK(s.cipher == Cipher::aes256);
	CHECK(s.mac == Mac::none);
	CHECK(s.rsa == RsaSize::rsa2048);
	CHECK(s.key_file.empty());
	CHECK_EQ(s.device, 0);
	CHECK(s.data == DataType::entropy);
	CHECK(s.stat_tests);
	CHECK(s.describe() == "cipher=aes256 mac=none rsa=RSA2048 key_file=built-in device=0 data=entropy stat_tests=on");
}

TEST(every_setting_can_be_changed) {
	Settings s;
	std::string error;
	REQUIRE(parse({{"cipher", "aes128"}, {"mac", "hmacSha256"}, {"rsa", "RSA4096"}, {"key_file", "/k/pub.pem"},
			{"device", "3"}, {"data", "sha512"}, {"stat_tests", "off"}}, s, error));
	CHECK(s.cipher == Cipher::aes128);
	CHECK(s.mac == Mac::hmacSha256);
	CHECK(s.rsa == RsaSize::rsa2048); // not read, because a key file is given
	CHECK(s.key_file == "/k/pub.pem");
	CHECK_EQ(s.device, 3);
	CHECK(s.data == DataType::sha512);
	CHECK(!s.stat_tests);
	CHECK(s.describe() == "cipher=aes128 mac=hmacSha256 rsa=key_file key_file=/k/pub.pem device=3 data=sha512 stat_tests=off");
}

TEST(all_allowed_values_are_accepted) {
	const char *ciphers[] = {"aes256", "aes128", "none"};
	const char *macs[] = {"none", "hmacSha256", "hmacSha160", "hmacMD5"};
	const char *rsas[] = {"RSA1024", "RSA2048", "RSA3072", "RSA4096"};
	const char *datas[] = {"entropy", "sha256", "sha512"};
	Settings s;
	std::string error;
	for (const char *v : ciphers) { CHECK(parse({{"cipher", v}}, s, error)); }
	for (const char *v : macs) { CHECK(parse({{"mac", v}}, s, error)); }
	for (const char *v : rsas) { CHECK(parse({{"rsa", v}}, s, error)); }
	for (const char *v : datas) { CHECK(parse({{"data", v}}, s, error)); }
	CHECK(parse({{"device", "0"}}, s, error));
	CHECK(parse({{"device", "25"}}, s, error));
}

TEST(stat_tests_accepts_the_usual_spellings) {
	Settings s;
	std::string error;
	const char *on[] = {"on", "yes", "true", "1"};
	const char *off[] = {"off", "no", "false", "0"};
	for (const char *v : on) { REQUIRE(parse({{"stat_tests", v}}, s, error)); CHECK(s.stat_tests); }
	for (const char *v : off) { REQUIRE(parse({{"stat_tests", v}}, s, error)); CHECK(!s.stat_tests); }
}

TEST(values_that_are_not_allowed_are_rejected_with_a_message_that_names_the_setting) {
	struct Bad {const char *name; const char *value;};
	const Bad bad[] = {{"cipher", "aes512"}, {"cipher", "AES256"}, {"mac", "sha256"}, {"rsa", "2048"}, {"rsa", "RSA512"},
			{"data", "raw"}, {"device", "-1"}, {"device", "26"}, {"device", "one"}, {"device", "1x"}, {"device", ""},
			{"stat_tests", "maybe"}};
	for (const Bad &b : bad) {
		Settings s;
		std::string error;
		CHECK(!parse({{b.name, b.value}}, s, error));
		CHECK(error.find(b.name) != std::string::npos);
	}
}

TEST(a_rejected_setting_leaves_the_result_unchanged) {
	Settings s;
	s.device = 7;
	std::string error;
	CHECK(!parse({{"device", "4"}, {"cipher", "bogus"}}, s, error));
	CHECK_EQ(s.device, 7);
}

TEST(the_error_lists_the_allowed_values) {
	Settings s;
	std::string error;
	REQUIRE(!parse({{"mac", "x"}}, s, error));
	CHECK(error.find("hmacSha256") != std::string::npos);
	CHECK(error.find("none") != std::string::npos);
}

TEST(with_a_key_file_the_rsa_setting_is_ignored) {
	Settings s;
	std::string error;
	REQUIRE(parse({{"key_file", "/k/pub.pem"}, {"rsa", "RSA3072"}}, s, error));
	CHECK(s.key_file == "/k/pub.pem");
	CHECK(s.describe().find("rsa=key_file") != std::string::npos);
	// Even a value that is not allowed does not matter, because it is not used
	REQUIRE(parse({{"key_file", "/k/pub.pem"}, {"rsa", "bogus"}}, s, error));
	// Without a key file the same value is rejected
	CHECK(!parse({{"rsa", "bogus"}}, s, error));
}

TEST(without_a_key_file_the_built_in_key_is_used) {
	Settings s;
	std::string error;
	REQUIRE(parse({{"rsa", "RSA1024"}}, s, error));
	CHECK(s.key_file.empty());
	CHECK(s.rsa == RsaSize::rsa1024);
	CHECK(s.describe().find("key_file=built-in") != std::string::npos);
}
