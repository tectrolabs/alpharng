/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.
 */

/**
 *    @file rng_backend_device.cpp
 *
 *    @brief Provider backend that reads random bytes from the first connected AlphaRNG device.
 *
 *    How the device is used (cipher, MAC, public key, device number, kind of data, statistical tests) comes from the
 *    provider settings. The defaults are those of the alrng utility: AES-256, no MAC, the device's own processed output.
 */
#include <climits>
#include <iostream>
#include <memory>
#include <new>

#include <AlphaRngApi.h>
#include <RsaCryptor.h>

#include "rng_backend.h"

namespace alrng_provider {

namespace {

// Largest request handed to the API in one call (its length argument is an int)
const size_t c_max_chunk_bytes = 1 << 24;

class DeviceBackend : public RngBackend {
public:
	bool open(const Settings &settings) override {
		alpharng::AlphaRngConfig cfg;
		switch (settings.cipher) {
		case Cipher::none: cfg.e_aes_key_size = alpharng::KeySize::None; break;
		case Cipher::aes128: cfg.e_aes_key_size = alpharng::KeySize::k128; break;
		default: cfg.e_aes_key_size = alpharng::KeySize::k256; break;
		}
		switch (settings.mac) {
		case Mac::hmacMD5: cfg.e_mac_type = alpharng::MacType::hmacMD5; break;
		case Mac::hmacSha160: cfg.e_mac_type = alpharng::MacType::hmacSha160; break;
		case Mac::hmacSha256: cfg.e_mac_type = alpharng::MacType::hmacSha256; break;
		default: cfg.e_mac_type = alpharng::MacType::None; break;
		}
		switch (settings.rsa) {
		case RsaSize::rsa1024: cfg.e_rsa_key_size = alpharng::RsaKeySize::rsa1024; break;
		case RsaSize::rsa3072: cfg.e_rsa_key_size = alpharng::RsaKeySize::rsa3072; break;
		case RsaSize::rsa4096: cfg.e_rsa_key_size = alpharng::RsaKeySize::rsa4096; break;
		default: cfg.e_rsa_key_size = alpharng::RsaKeySize::rsa2048; break;
		}
		cfg.pub_key_file_name = settings.key_file;
		if (!settings.key_file.empty()) {
			// As alrng does: the size of the key in the file decides the RSA size
			alpharng::RsaCryptor rsa(settings.key_file, true);
			if (!rsa.is_initialized()) {
				std::cerr << "AlphaRNG provider: could not load the RSA public key file " << settings.key_file << std::endl;
				return false;
			}
			cfg.e_rsa_key_size = rsa.get_rsa_key_size();
		}
		m_data = settings.data;

		m_api.reset(new (std::nothrow) alpharng::AlphaRngApi(cfg));
		if (!m_api || !m_api->is_initialized()) {
			std::cerr << "AlphaRNG provider: could not initialize the AlphaRNG API" << std::endl;
			return false;
		}
		if (!m_api->connect(settings.device)) {
			std::cerr << "AlphaRNG provider: " << m_api->get_last_error() << std::endl;
			return false;
		}
		if (!settings.stat_tests) {
			m_api->disable_stat_tests();
		}
		return true;
	}

	bool read(unsigned char *buffer, size_t size) override {
		if (!m_api) {
			return false;
		}
		while (size > 0) {
			size_t chunk = size < c_max_chunk_bytes ? size : c_max_chunk_bytes;
			int length = static_cast<int>(chunk);
			bool ok;
			switch (m_data) {
			case DataType::sha256: ok = m_api->extract_sha256_entropy(buffer, length); break;
			case DataType::sha512: ok = m_api->extract_sha512_entropy(buffer, length); break;
			default: ok = m_api->get_entropy(buffer, length); break;
			}
			if (!ok) {
				std::cerr << "AlphaRNG provider: failed to read from the device. " << m_api->get_last_error() << std::endl;
				return false;
			}
			buffer += chunk;
			size -= chunk;
		}
		return true;
	}

private:
	std::unique_ptr<alpharng::AlphaRngApi> m_api;
	DataType m_data {DataType::entropy};
};

} // namespace

RngBackend *create_backend() {
	return new (std::nothrow) DeviceBackend;
}

} // namespace alrng_provider
