/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.
 */

/**
 *    @file settings.h
 *
 *    @brief How the AlphaRNG provider talks to the device, as chosen in the provider section of openssl.cnf.
 *
 *    The values use the same spellings as the options of the alrng utility:
 *
 *        cipher      = aes256 | aes128 | none                      (default aes256)
 *        mac         = none | hmacSha256 | hmacSha160 | hmacMD5     (default none)
 *        rsa         = RSA2048 | RSA1024 | RSA3072 | RSA4096        (default RSA2048; ignored with key_file)
 *        key_file    = path of a device public key file             (default: the key built into the API;
 *                                                                    its key size is used instead of 'rsa')
 *        device      = device number, 0 to 25                       (default 0)
 *        data        = entropy | sha256 | sha512                    (default entropy)
 *        stat_tests  = on | off                                     (default on)
 */
#ifndef ALPHARNG_PROVIDER_SETTINGS_H_
#define ALPHARNG_PROVIDER_SETTINGS_H_

#include <functional>
#include <string>

namespace alrng_provider {

enum class Cipher {none, aes128, aes256};
enum class Mac {none, hmacMD5, hmacSha160, hmacSha256};
enum class RsaSize {rsa1024, rsa2048, rsa3072, rsa4096};
enum class DataType {entropy, sha256, sha512};

struct Settings {
	Cipher cipher {Cipher::aes256};
	Mac mac {Mac::none};
	RsaSize rsa {RsaSize::rsa2048};
	std::string key_file;
	int device {0};
	DataType data {DataType::entropy};
	bool stat_tests {true};

	// One line that names every setting, for example "cipher=aes256 mac=none ..."
	std::string describe() const;
};

// The names of the settings, in the order of the table above
extern const char *const c_setting_names[];
extern const int c_setting_count;

// 'lookup' returns the value configured for a setting name, or nullptr when there is none. Settings that are not
// configured keep their defaults. Returns false with a message in 'error' when a value is not allowed.
bool parse_settings(const std::function<const char *(const char *)> &lookup, Settings &out, std::string &error);

} // namespace alrng_provider

#endif /* ALPHARNG_PROVIDER_SETTINGS_H_ */
