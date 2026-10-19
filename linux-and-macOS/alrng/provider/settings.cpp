/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.
 */

/**
 *    @file settings.cpp
 *
 *    @brief Reads and checks the AlphaRNG provider settings. See settings.h.
 */
#include <cstdlib>

#include "settings.h"

namespace alrng_provider {

const char *const c_setting_names[] = {"cipher", "mac", "rsa", "key_file", "device", "data", "stat_tests"};
const int c_setting_count = 7;

namespace {

struct Choice {
	const char *name;
	int value;
};

const Choice c_ciphers[] = {{"aes256", static_cast<int>(Cipher::aes256)}, {"aes128", static_cast<int>(Cipher::aes128)},
		{"none", static_cast<int>(Cipher::none)}};
const Choice c_macs[] = {{"none", static_cast<int>(Mac::none)}, {"hmacSha256", static_cast<int>(Mac::hmacSha256)},
		{"hmacSha160", static_cast<int>(Mac::hmacSha160)}, {"hmacMD5", static_cast<int>(Mac::hmacMD5)}};
const Choice c_rsas[] = {{"RSA2048", static_cast<int>(RsaSize::rsa2048)}, {"RSA1024", static_cast<int>(RsaSize::rsa1024)},
		{"RSA3072", static_cast<int>(RsaSize::rsa3072)}, {"RSA4096", static_cast<int>(RsaSize::rsa4096)}};
const Choice c_datas[] = {{"entropy", static_cast<int>(DataType::entropy)}, {"sha256", static_cast<int>(DataType::sha256)},
		{"sha512", static_cast<int>(DataType::sha512)}};

template <size_t N>
const char *name_of(const Choice (&choices)[N], int value) {
	for (const Choice &choice : choices) {
		if (choice.value == value) {
			return choice.name;
		}
	}
	return "?";
}

// Finds 'text' among 'choices'. On failure, 'error' lists the allowed values.
template <size_t N>
bool choose(const char *setting, const char *text, const Choice (&choices)[N], int &out, std::string &error) {
	for (const Choice &choice : choices) {
		if (std::string(text) == choice.name) {
			out = choice.value;
			return true;
		}
	}
	error = std::string("unexpected value '") + text + "' for '" + setting + "', must be ";
	for (size_t i = 0; i < N; ++i) {
		error += (i == 0 ? "" : (i + 1 == N ? " or " : ", ")) + std::string(choices[i].name);
	}
	return false;
}

} // namespace

std::string Settings::describe() const {
	return std::string("cipher=") + name_of(c_ciphers, static_cast<int>(cipher))
			+ " mac=" + name_of(c_macs, static_cast<int>(mac))
			+ " rsa=" + (key_file.empty() ? name_of(c_rsas, static_cast<int>(rsa)) : "key_file")
			+ " key_file=" + (key_file.empty() ? "built-in" : key_file)
			+ " device=" + std::to_string(device)
			+ " data=" + name_of(c_datas, static_cast<int>(data))
			+ " stat_tests=" + (stat_tests ? "on" : "off");
}

bool parse_settings(const std::function<const char *(const char *)> &lookup, Settings &out, std::string &error) {
	Settings result;
	int value = 0;
	const char *text = nullptr;

	if ((text = lookup("cipher")) != nullptr) {
		if (!choose("cipher", text, c_ciphers, value, error)) {
			return false;
		}
		result.cipher = static_cast<Cipher>(value);
	}
	if ((text = lookup("mac")) != nullptr) {
		if (!choose("mac", text, c_macs, value, error)) {
			return false;
		}
		result.mac = static_cast<Mac>(value);
	}
	if ((text = lookup("key_file")) != nullptr) {
		result.key_file = text;
	}
	// With a key file, the size of the key in the file is used and 'rsa' is ignored
	if (result.key_file.empty() && (text = lookup("rsa")) != nullptr) {
		if (!choose("rsa", text, c_rsas, value, error)) {
			return false;
		}
		result.rsa = static_cast<RsaSize>(value);
	}
	if ((text = lookup("device")) != nullptr) {
		char *end = nullptr;
		long number = std::strtol(text, &end, 10);
		if (end == text || *end != '\0' || number < 0 || number > 25) {
			error = std::string("unexpected value '") + text + "' for 'device', must be a number from 0 to 25";
			return false;
		}
		result.device = static_cast<int>(number);
	}
	if ((text = lookup("data")) != nullptr) {
		if (!choose("data", text, c_datas, value, error)) {
			return false;
		}
		result.data = static_cast<DataType>(value);
	}
	if ((text = lookup("stat_tests")) != nullptr) {
		std::string flag(text);
		if (flag == "on" || flag == "yes" || flag == "true" || flag == "1") {
			result.stat_tests = true;
		} else if (flag == "off" || flag == "no" || flag == "false" || flag == "0") {
			result.stat_tests = false;
		} else {
			error = std::string("unexpected value '") + text + "' for 'stat_tests', must be on or off";
			return false;
		}
	}
	out = result;
	return true;
}

} // namespace alrng_provider
