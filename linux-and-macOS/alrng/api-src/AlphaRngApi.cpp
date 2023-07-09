/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the API for interacting with the AlphaRNG device over a secure data communication channel.

  */

/**
 *    @file AlphaRngApi.cpp
 *    @date 07/8/2023
 *    @Author: Andrian Belinski
 *    @version 1.6
 *
 *    @brief Implements the API for securely interacting with the AlphaRNG device.
 */

#include "pch.h"
#include <AlphaRngApi.h>

namespace alpharng {

AlphaRngApi::AlphaRngApi(const AlphaRngConfig &cfg) {
	initialize(cfg);
}

AlphaRngApi::AlphaRngApi() {
	initialize(AlphaRngConfig {MacType::hmacSha256, RsaKeySize::rsa2048, KeySize::k256, ""});
}

void AlphaRngApi::initialize(const AlphaRngConfig &cfg) {
	m_cfg = cfg;
	if (m_cfg.pub_key_file_name.size() > 0) {
		if (!initialize_rsa_keyfile()) {
			m_error_log_oss << "Could not initialize RSA from a public key file" << ". " << endl;
			return;
		}
	} else {
		if (!initialize_rsa()) {
			m_error_log_oss << "Could not initialize RSA" << ". " << endl;
			return;
		}
	}
	if (!initialize_aes(m_cfg.e_aes_key_size)) {
		m_error_log_oss << "Could not initialize cipher" << ". " << endl;
		return;
	}
	if (!initialize_hmac(m_cfg.e_mac_type)) {
		m_error_log_oss << "Could not initialize MAC generator" << ". " << endl;
		return;
	}
	if (!initialize_serial_device()) {
		m_error_log_oss << "Could not initialize serial device" << ". " << endl;
		return;
	}

	if (!initialize_sha()) {
		m_error_log_oss << "Could not initialize serial SHA" << ". " << endl;
		return;
	}

	if (!RAND_bytes((unsigned char *)&m_token_serial_number, sizeof(m_token_serial_number))) {
		m_error_log_oss << "Could not initialize token serial number" << ". " << endl;
		return;
	}

	m_is_initialized = true;
	clear_error_log();
}

bool AlphaRngApi::initialize_serial_device() {
#ifdef _WIN64
	m_device = new (nothrow) WinUsbSerialDevice();
#else
	m_device = new (nothrow) UsbSerialDevice();
#endif

	
	if (m_device == nullptr) {
		return false;
	}
	m_device_name = new (nothrow) char [m_max_device_name_size];
	if (m_device_name == nullptr) {
		return false;
	}
	m_device_count = 0;
	return true;
}

bool AlphaRngApi::initialize_rsa_keyfile() {
	m_rsa_cryptor = new (nothrow) RsaCryptor(m_cfg.pub_key_file_name, true);
	if (m_rsa_cryptor == nullptr || !m_rsa_cryptor->is_initialized()) {
		return false;
	}
	return true;
}

bool AlphaRngApi::initialize_sha() {
	m_sha_256 = new (nothrow) Sha256();
	if (m_sha_256 == nullptr) {
		return false;
	}

	m_sha_512 = new (nothrow) Sha512();
	if (m_sha_512 == nullptr) {
		return false;
	}

	return true;
}

bool AlphaRngApi::initialize_rsa() {
	switch(m_cfg.e_rsa_key_size) {
	case RsaKeySize::rsa2048:
	default:
		m_rsa_cryptor = new (nothrow) RsaCryptor(m_rsa_key_repo.c_rsapub_2048_pem, m_rsa_key_repo.c_rsapub_2048_pem_len, true);
		break;
	case RsaKeySize::rsa1024:
		m_rsa_cryptor = new (nothrow) RsaCryptor(m_rsa_key_repo.c_rsapub_1024_pem, m_rsa_key_repo.c_rsapub_1024_pem_len, true);
		break;
	}
	if (m_rsa_cryptor == nullptr || !m_rsa_cryptor->is_initialized()) {
		return false;
	}
	return true;
}

PacketType AlphaRngApi::get_rsa_request_type() const {
	if (m_rsa_cryptor->is_public_key_file()) {
		return PacketType::pkAltRSA2048;
	}
	switch(m_cfg.e_rsa_key_size) {
	case RsaKeySize::rsa2048:
	default:
		return PacketType::pkRSA2048;
	case RsaKeySize::rsa1024:
		return PacketType::pkRSA1024;
	}
}

bool AlphaRngApi::initialize_hmac(MacType e_mac_type) {
	switch(e_mac_type) {
	case MacType::hmacSha256:
	default:
		m_hmac = new (nothrow) HmacSha256();
		if (m_hmac == nullptr || !m_hmac->is_initialized()) {
			return false;
		}
		break;
	case MacType::hmacSha160:
		m_hmac = new (nothrow) HmacSha1();
		if (m_hmac == nullptr || !m_hmac->is_initialized()) {
			return false;
		}
		break;
	case MacType::hmacMD5:
		m_hmac = new (nothrow) HmacMD5();
		if (m_hmac == nullptr || !m_hmac->is_initialized()) {
			return false;
		}
		break;
	}
	return true;
}

bool AlphaRngApi::initialize_aes(KeySize e_aes_key_size) {
	if (e_aes_key_size != KeySize::None) {
		if (m_aes_cryptor != nullptr) {
			delete m_aes_cryptor;
		}
		m_aes_cryptor = new (nothrow) AesCryptor(e_aes_key_size);
		if (m_aes_cryptor == nullptr || !m_aes_cryptor->is_initialized()) {
			return false;
		}
	}
	return true;
}

bool AlphaRngApi::initialize_entropy_extractor(ShaInterface *sha_api) {

	if (m_sha_ent_extr != nullptr && m_sha_ent_extr->get_hash_size() != sha_api->get_hash_size()) {
		// The SHA implementation has changed, create a new instance
		delete m_sha_ent_extr;
		m_sha_ent_extr = nullptr;
	}
	if (m_sha_ent_extr == nullptr) {
		m_sha_ent_extr = new (nothrow) ShaEntropyExtractor(this, sha_api);
		if (m_sha_ent_extr == nullptr) {
			return false;
		}
	}
	return true;
}

/**
 * Check to see if connection is successfully established with the device
 *
 * @return true if connection has been successfully established
 */
bool AlphaRngApi::is_connected() {
	if (!is_initialized()) {
		return false;
	}
	return m_device->is_connected();
}

/**
 * Retrieve AlphaRNG internal health status.
 *
 * It may have one of the following values:
 * 0 - healthy
 * 4 - frequency distribution test failure
 * 2 - APT test error
 * 1 - RCT test error
 *
 * @param[out] status points to location for storing the RNG status
 *
 * @return true for successful operation
 */
bool AlphaRngApi::retrieve_rng_status(unsigned char *status) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}

	clear_error_log();

	Command cmd;
	Response resp;

	clear_command(&cmd);
	clear_response(&resp);

	cmd.e_type = CommandType::getDeviceHealthStatus;
	cmd.payload_size = 0;

	if (!execute_command(&resp, &cmd, 1)) {
		return false;
	}
	*status = resp.payload[0];
	return true;
}

/**
 * Retrieve digitized raw (unprocessed) random bytes from the first noise source of the AlphaRNG device.
 * It is used to evaluate the quality of the first random noise source.
 *
 * @param[out] out points to a byte arrays for storing the resulting bytes
 * @param[in] out_length how many bytes to retrieve
 *
 * @return true for successful operation
 */
bool AlphaRngApi::get_noise_source_1(unsigned char *out, int out_length) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	clear_error_log();
	if (m_cfg.e_mac_type != MacType::None || m_cfg.e_aes_key_size != KeySize::None) {
		return get_bytes(CommandType::getNoiseSourceOne, out, out_length, c_rnd_data_block_size_bytes, true);
	} else {
		return get_unpacked_bytes_with_retry('1', out, out_length, c_rnd_data_block_size_bytes, true);
	}
}

/**
 * Retrieve digitized raw (unprocessed) random bytes from the second noise source of the AlphaRNG device.
 * It is used to evaluate the quality of the second random noise source.
 *
 * @param[out] out points to a byte arrays for storing the resulting bytes
 * @param[in] out_length how many bytes to retrieve
 *
 * @return true for successful operation
 */
bool AlphaRngApi::get_noise_source_2(unsigned char *out, int out_length) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	clear_error_log();
	if (m_cfg.e_mac_type != MacType::None || m_cfg.e_aes_key_size != KeySize::None) {
		return get_bytes(CommandType::getNoiseSourceTwo, out, out_length, c_rnd_data_block_size_bytes, true);
	} else {
		return get_unpacked_bytes_with_retry('2', out, out_length, c_rnd_data_block_size_bytes, true);
	}
}

/**
 * Retrieve entropy bytes from the AlphaRNG device.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or can feed the entropy inputs of DRBG(s).
 *
 * @param[out] out points to a byte array for storing the random bytes retrieved
 * @param[in] out_length how many random bytes to retrieve
 *
 * @return true for successful operation
 */
bool AlphaRngApi::get_entropy(unsigned char *out, int out_length) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	clear_error_log();
	if (m_cfg.e_mac_type != MacType::None || m_cfg.e_aes_key_size != KeySize::None) {
		return get_bytes(CommandType::getEntropy, out, out_length, c_rnd_data_block_size_bytes, true);
	} else {
		return get_unpacked_bytes_with_retry('x', out, out_length, c_rnd_data_block_size_bytes, true);
	}
}

/**
 * Extract entropy bytes by applying SHA-256 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for generating high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[out] out points to a byte array for storing the random bytes extracted
 * @param[in] out_length how many entropy bytes to retrieve
 *
 * @return true for successful operation
 */
bool AlphaRngApi::extract_sha256_entropy(unsigned char *out, int out_length) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	clear_error_log();
	return extract_entropy(m_sha_256, out, out_length);
}

/**
 * Extract entropy bytes by applying SHA-512 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for generating high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[out] out points to a byte array for storing the random bytes extracted
 * @param[in] out_length how many entropy bytes to retrieve
 *
 * @return true for successful operation
 */
bool AlphaRngApi::extract_sha512_entropy(unsigned char *out, int out_length) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	clear_error_log();
	return extract_entropy(m_sha_512, out, out_length);
}

/**
 * Extract entropy bytes by applying SHA specific method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for generating high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[in] hash_api SHA specific implementation
 * @param[out] out points to a byte array for storing the random bytes extracted
 * @param[in] out_length how many entropy bytes to retrieve
 *
 * @return true for successful operation
 */
bool AlphaRngApi::extract_entropy(ShaInterface *sha_api, unsigned char *out, int out_length) {
	if (out == nullptr) {
		m_error_log_oss << "AlphaRngApi.extract_entropy(): 'out' argument cannot be null" << endl;
		return false;
	}
	if (out_length < 1) {
		m_error_log_oss << "AlphaRngApi.extract_entropy(): invalid 'out_length' argument value" << endl;
		return false;
	}

	if (!initialize_entropy_extractor(sha_api)) {
		m_error_log_oss << "AlphaRngApi.extract_entropy(): could not initialize entropy extractor" << endl;
		return false;
	}

	if (!m_sha_ent_extr->extract_entropy(out, out_length)) {
		m_error_log_oss << m_sha_ent_extr->get_last_error();
		return false;
	}
	return true;
}

/**
 * Retrieve concatenated raw random bytes of both noise sources from the AlphaRNG device.
 * It is used to evaluate the quality of the raw random byte stream produced by both noise sources
 * before any post-processing or conditioning.
 * The byte stream can also be used as an input for alternative post-processing or conditioning algorithms
 * such as SHA and HMAC extractors.
 *
 * @param[out] out points to a byte array for storing the random bytes retrieved
 * @param[in] out_length how many random bytes to retrieve
 *
 * @return true for successful operation
 */
bool AlphaRngApi::get_noise(unsigned char *out, int out_length) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	clear_error_log();
	if (m_cfg.e_mac_type != MacType::None || m_cfg.e_aes_key_size != KeySize::None) {
		return get_bytes(CommandType::getNoise, out, out_length, c_rnd_data_block_size_bytes, true);
	} else {
		return get_unpacked_bytes_with_retry('n', out, out_length, c_rnd_data_block_size_bytes, true);
	}
}

/**
 * This method is only used for testing the correctness of the data communication with the
 * SecireRNG device. Each byte retrieved, starting with 0, represents an incremented value of
 * the previous byte value.
 *
 * @param[out] out points to a byte array for storing the test data
 * @param[in] out_length how many bytes of test data to receive
 *
 * @return true for successful operation
 *
 */
bool AlphaRngApi::get_test_data(unsigned char *out, int out_length) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	clear_error_log();
	return get_bytes(CommandType::getTestData, out, out_length, c_test_data_block_size_bytes, false);
}

/**
 * Retrieve entropy bytes from the AlphaRNG device and store those into a file.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or can feed the entropy inputs of DRBG(s).
 *
 * @param[out] out file_path_name file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return true for successful operation
 */
bool AlphaRngApi::entropy_to_file(const string &file_path_name, int64_t num_bytes) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	return to_file(CommandType::getEntropy, file_path_name, num_bytes);
}

/**
 * Extract entropy bytes into a file by applying SHA-256 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[out] out file_path_name file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return true for successful operation
 */
bool AlphaRngApi::extract_sha256_entropy_to_file(const string &file_path_name, int64_t num_bytes) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	return to_file(CommandType::extractSha256Entropy, file_path_name, num_bytes);
}

/**
 * Extract entropy bytes into a file by applying SHA-512 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[out] out file_path_name file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return true for successful operation
 */
bool AlphaRngApi::extract_sha512_entropy_to_file(const string &file_path_name, int64_t num_bytes) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	return to_file(CommandType::extractSha512Entropy, file_path_name, num_bytes);
}

/**
 * Retrieve raw (unprocessed) bytes from the AlphaRNG device and store those into a file.
 * It is used to evaluate the quality of the raw random byte stream produced by both noise sources
 * before any post-processing or conditioning.
 * The byte stream can also be used as an input for alternative post-processing or conditioning algorithms.
 *
 * @param[out] out file_path_name file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return true for successful operation
 */
bool AlphaRngApi::noise_to_file(const string &file_path_name, int64_t num_bytes) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	return to_file(CommandType::getNoise, file_path_name, num_bytes);
}

bool AlphaRngApi::get_data(CommandType cmd_type, unsigned char *out, int out_length) {
	switch(cmd_type) {
	case CommandType::getEntropy:
		return get_entropy(out, out_length);
	case CommandType::getNoise:
		return get_noise(out, out_length);
	case CommandType::extractSha256Entropy:
		return extract_sha256_entropy(out, out_length);
	case CommandType::extractSha512Entropy:
		return extract_sha512_entropy(out, out_length);
	case CommandType::getNoiseSourceOne:
		return get_noise_source_1(out, out_length);
	case CommandType::getNoiseSourceTwo:
		return get_noise_source_2(out, out_length);
	default:
		m_error_log_oss << "Invalid command: " << (int)cmd_type << ". " << endl;
		return false;
	}
}

/**
 * Retrieve digitized raw (unprocessed) random bytes from the first noise source of the AlphaRNG device
 * and store the bytes received into a file.
 * It is used to evaluate the quality of the first random noise source.
 *
 * @param[in] file_path_name file path name for storing the retrieved bytes
 * @param[in] num_bytes how many bytes to retrieve, 0 - for continuous operation
 *
 * @return true for successful operation
 */
bool AlphaRngApi::noise_source_one_to_file(const string &file_path_name, const int64_t num_bytes) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	return to_file(CommandType::getNoiseSourceOne, file_path_name, num_bytes);
}

/**
 * Retrieve digitized raw (unprocessed) random bytes from the second noise source of the AlphaRNG device
 * and store the bytes received into a file.
 * It is used to evaluate the quality of the second random noise source.
 *
 * @param[in] file_path_name file path name for storing the retrieved bytes
 * @param[in] num_bytes how many bytes to retrieve, 0 - for continuous operation
 *
 * @return true for successful operation
 */
bool AlphaRngApi::noise_source_two_to_file(const string &file_path_name, const int64_t num_bytes) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	return to_file(CommandType::getNoiseSourceTwo, file_path_name, num_bytes);
}

bool AlphaRngApi::to_file(CommandType cmd_type, const string &file_path_name, int64_t num_bytes) {
	clear_error_log();
	if (num_bytes < 0) {
		m_error_log_oss << "Invalid amount of bytes requested: " << num_bytes << ". " << endl;
		return false;
	}
	if (num_bytes > c_max_file_ouput_bytes) {
		m_error_log_oss << "Amount of bytes cannot exceed: " << c_max_file_ouput_bytes << ". " << endl;
		return false;
	}

	if (m_file_buffer == nullptr) {
		m_file_buffer = new (nothrow) unsigned char[c_file_output_buff_size_bytes];
		if (m_file_buffer == nullptr) {
			m_error_log_oss << "Could not allocate memory for the file buffer" << ". " << endl;
			return false;
		}
	}

	ofstream os_file(file_path_name.c_str(), ios::out | ios::binary);
	if(!os_file.good()) {
		m_error_log_oss << "Could not open file: " << file_path_name << ". " << endl;
		return false;
	}

	if (num_bytes == 0) {
		// Infinite loop
		while (1) {
			if(!get_data(cmd_type, m_file_buffer, c_file_output_buff_size_bytes)) {
				return false;
			}
			os_file.write((const char*)m_file_buffer, c_file_output_buff_size_bytes);
			if(!os_file.good()) {
				m_error_log_oss << "Could not continuously write " << c_file_output_buff_size_bytes << " bytes to file: " << file_path_name << ". " << endl;
				return false;
			}
		}
	}

	int64_t num_chunks = num_bytes / c_file_output_buff_size_bytes;
	int64_t num_remaining_bytes = num_bytes % c_file_output_buff_size_bytes;

	for (int64_t i = 0; i < num_chunks; ++i) {
		if(!get_data(cmd_type, m_file_buffer, c_file_output_buff_size_bytes)) {
			return false;
		}
		os_file.write((const char*)m_file_buffer, c_file_output_buff_size_bytes);
		if(!os_file.good()) {
			m_error_log_oss << "Could not write " << c_file_output_buff_size_bytes << " bytes to file: " << file_path_name << ". " << endl;
			return false;
		}
	}
	if (num_remaining_bytes) {
		if(!get_data(cmd_type, m_file_buffer, (int)num_remaining_bytes)) {
			return false;
		}
		os_file.write((const char*)m_file_buffer, num_remaining_bytes);
		if(!os_file.good()) {
			m_error_log_oss << "Could not write last " <<  num_remaining_bytes << " to file: " << file_path_name << ". " << endl;
			return false;
		}
	}
	os_file.close();
	if(!os_file.good()) {
		m_error_log_oss << "Could not close file: " << file_path_name << ". " << endl;
		return false;
	}

	return true;
}

bool AlphaRngApi::get_payload_bytes_with_retry(char cmd, unsigned char *out, int out_length) {
	for (int tries = 0; tries < c_max_command_retry_count; ++tries) {
		clear_error_log();

		int actual_bytes_sent;
		if (!m_device->send_data((unsigned char *)&cmd, 1, &actual_bytes_sent)) {
			int actual_bytes_received;
			if (!m_device->receive_data(out, out_length, &actual_bytes_received)) {
				return true;
			} else {
				m_error_log_oss << "Could not receive response from device. " << endl;
			}
		} else {
			m_error_log_oss << "Could not sent one byte command to device. " << endl;
		}

		m_op_retry_count++;
		sleep_usecs(1000 * 100);
		clear_receiver();
		sleep_usecs(1000 * 100);
	}
	return false;
}

bool AlphaRngApi::get_unpacked_bytes_with_retry(char cmd, unsigned char *out, int out_length, int block_size_bytes, bool test_data) {
	if (out_length < 1) {
		m_error_log_oss << "Amount of bytes requested is invalid: " << out_length << ". " << endl;
		return false;
	}

	for (int tries = 0; tries < c_max_command_retry_count; ++tries) {
		if(get_unpacked_bytes(cmd, out, out_length, block_size_bytes, test_data)) {
			return true;
		}

		sleep_usecs(1000);
		clear_receiver();
		sleep_usecs(1000);
	}
	return false;

}

bool AlphaRngApi::get_unpacked_bytes(char cmd, unsigned char *out, int out_length, int block_size_bytes, bool test_data) {

	Response resp;
	unsigned char *dest = out;
	int chunks = out_length / block_size_bytes;
	int ramaining_bytes = out_length % block_size_bytes;

	for (int i = 0; i < chunks; ++i) {
		if (test_data) {
			m_health_test.restart();
		}
		if (!get_payload_bytes_with_retry(cmd, resp.payload, block_size_bytes + 1)) {
			return false;
		}
		uint8_t rng_status = resp.payload[block_size_bytes];
		if (rng_status) {
			m_error_log_oss << "Device rng status: " << (int)rng_status << ". " << endl;
			return false;
		}
		memcpy(dest, resp.payload, block_size_bytes);
		dest += block_size_bytes;
		if (test_data) {
			m_health_test.test(resp.payload, block_size_bytes);
			if (m_health_test.is_error()) {
				m_error_log_oss << "Health test error 1: " << (int)m_health_test.get_health_status() << ". " << endl;
				return false;
			}
		}
	}

	if (ramaining_bytes > 0) {
		if (test_data) {
			m_health_test.restart();
		}
		if (!get_payload_bytes_with_retry(cmd, resp.payload, block_size_bytes + 1)) {
			return false;
		}
		uint8_t rng_status = resp.payload[block_size_bytes];
		if (rng_status) {
			m_error_log_oss << "Device rng status: " << (int)rng_status << ". " << endl;
			return false;
		}
		memcpy(dest, resp.payload, ramaining_bytes);
		if (test_data) {
			m_health_test.test(resp.payload, ramaining_bytes);
			if (m_health_test.is_error()) {
				m_error_log_oss << "Health test error 2: " << (int)m_health_test.get_health_status() << ". " << endl;
				return false;
			}
		}
	}
	return true;
}

bool AlphaRngApi::get_bytes(CommandType cmd_type, unsigned char *out, int out_length, int block_size_bytes, bool test_data) {
	if (out_length < 1) {
		m_error_log_oss << "Invalid amount of bytes requested: " << out_length << ". " << endl;
		return false;
	}

	Command cmd;
	Response resp;
	unsigned char *dest = out;
	int chunks = out_length / block_size_bytes;
	int ramaining_bytes = out_length % block_size_bytes;

	for (int i = 0; i < chunks; ++i) {
		if (test_data) {
			m_health_test.restart();
		}
		clear_response(&resp);
		clear_command(&cmd);
		cmd.e_type = cmd_type;
		cmd.payload_size = 0;
		if (!execute_command(&resp, &cmd, block_size_bytes + 1)) {
			return false;
		}
		uint8_t rng_status = resp.payload[block_size_bytes];
		if (rng_status) {
			m_error_log_oss << "Could not retrieve expected bytes from device, rng status: " << (int)rng_status << ". " << endl;
			return false;
		}
		memcpy(dest, resp.payload, block_size_bytes);
		dest += block_size_bytes;
		if (test_data) {
			m_health_test.test(resp.payload, block_size_bytes);
			if (m_health_test.is_error()) {
				m_error_log_oss << "Stage 1 Health test error : " << (int)m_health_test.get_health_status() << ". " << endl;
				return false;
			}
		}
	}

	if (ramaining_bytes > 0) {
		if (test_data) {
			m_health_test.restart();
		}
		clear_response(&resp);
		clear_command(&cmd);
		cmd.e_type = cmd_type;
		cmd.payload_size = 0;
		if (!execute_command(&resp, &cmd, block_size_bytes + 1)) {
			return false;
		}
		uint8_t rng_status = resp.payload[block_size_bytes];
		if (rng_status) {
			m_error_log_oss << "Could not retrieve expected bytes from device, rng status: " << (int)rng_status << ". " << endl;
			return false;
		}
		memcpy(dest, resp.payload, ramaining_bytes);
		if (test_data) {
			m_health_test.test(resp.payload, ramaining_bytes);
			if (m_health_test.is_error()) {
				m_error_log_oss << "Stage 2 Health test error : " << (int)m_health_test.get_health_status() << ". " << endl;
				return false;
			}
		}
	}

	return true;
}

/**
 * Retrieve random values frequency distribution for both noise sources.
 * That information can be used for evaluating the quality of
 * noise sources.
 *
 * @param[out] freq_tables points to a FrequencyTables structure for storing frequency values
 *
 * @return true for successful operation
 */
bool AlphaRngApi::retrieve_frequency_tables(FrequencyTables *freq_tables) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}

	clear_error_log();
	Command cmd;
	Response resp;

	clear_command(&cmd);
	clear_response(&resp);

	cmd.e_type = CommandType::getFrequencyTables;
	cmd.payload_size = 0;

	if (!execute_command(&resp, &cmd, sizeof(FrequencyTables) + 1)) {
		return false;
	}
	uint8_t rng_status = resp.payload[1024];
	if (rng_status) {
		m_error_log_oss << "Could not retrieve frequency tables, rng status: " << (int)rng_status << ". " << endl;
		return false;
	}

	memcpy(freq_tables->freq_table_1, resp.payload, sizeof(FrequencyTables));

	return true;

}

/**
 * Invoke device internal health test suite
 *
 * @return true if health tests executed successfully
 */
bool AlphaRngApi::run_health_test() {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	clear_error_log();
	Command cmd;
	Response resp;

	clear_command(&cmd);
	clear_response(&resp);

	cmd.e_type = CommandType::healthTest;
	cmd.payload_size = 0;

	if (!execute_command(&resp, &cmd, 1)) {
		return false;
	}
	uint8_t rng_status = resp.payload[0];
	if (rng_status) {
		m_error_log_oss << "Device health test has failed with error code: " << (int)rng_status << ". " << endl;
		return false;
	}
	return true;
}

bool AlphaRngApi::retrieve_device_info(DeviceInfo *device_info) {
	Command cmd;
	Response resp;

	clear_command(&cmd);
	clear_response(&resp);

	cmd.e_type = CommandType::getDeviceInfo;
	cmd.payload_size = 0;

	if (!execute_command(&resp, &cmd, sizeof(DeviceInfo))) {
		return false;
	}
	memcpy(device_info, resp.payload, sizeof(DeviceInfo));
	return true;
}

/**
 * Establish a connection with AlphaRNG device specified by `device_number`
 *
 * @param[in] device_number device number, 0 for the first device
 *
 * @return true if connection was successful
 *
 */
bool AlphaRngApi::connect(int device_number) {
	if (!is_initialized() || is_connected()) {
		return false;
	}

	m_op_retry_count = 0;

	for (int tries = 0; tries < c_max_command_retry_count; ++tries) {
		disconnect();
		if (connect_internal(device_number)) {
			return true;
		}
		sleep_usecs(1000 * 100);
		clear_receiver();
		m_op_retry_count++;
	}
	return false;
}

bool AlphaRngApi::connect_internal(int device_number) {
	clear_error_log();
	get_device_count();
	if (m_device_count == 0) {
		m_error_log_oss << "Device number " <<  device_number << " could not be found" << ". " << endl;
		return false;
	}
	if (!m_device->retrieve_device_path(m_device_name, m_max_device_name_size, device_number)) {
		m_error_log_oss << "Could not identify device name for device number " <<  device_number << ". " << endl;
		return false;
	}
	bool status = m_device->connect(m_device_name);
	if(!status) {
		m_error_log_oss << m_device->get_error_log() << ". " << endl;
		return false;
	}

	clear_receiver();

	// Set connection time out for a slow operation
	if (!m_device->set_connection_timeout(c_slow_timeout_mlsecs)) {
		m_error_log_oss << "Could not set connection timeout value: " << c_slow_timeout_mlsecs << ". " << endl;
		return false;

	}

	if (m_cfg.e_mac_type != MacType::None) {
		// Create a new MAC key as part of the session key
		if (!m_hmac->generate_new_key()) {
			m_error_log_oss << "Could not generate MAC key for new session" << ". " << endl;
			return false;
		}
	}

	// Create a new cipher key as part of the session key
	if (!initialize_aes(m_cfg.e_aes_key_size)) {
		m_error_log_oss << "Could not generate cipher key for new session" << ". " << endl;
		return false;
	}

	if (!upload_session_key()) {
		m_error_log_oss << "Could not upload the session key" << ". " << endl;
		return false;
	}

	// Set connection time out for fast operations
	if (!m_device->set_connection_timeout(c_fast_timeout_mlsecs)) {
		m_error_log_oss << "Could not set connection timeout to: " << c_fast_timeout_mlsecs << ". " << endl;
		return false;
	}

	if (!retrieve_device_info(&m_device_info)) {
		m_error_log_oss << "Could not retrieve device information" << ". " << endl;
		return false;
	}
	return true;
}

/**
 * Retrieve AlphaRNG device identification number or serial number
 *
 * @param[out] id device serial number
 *
 * @return true for successful operation
 */
bool AlphaRngApi::retrieve_device_id(string &id) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	id.clear();
	id.append((char*)m_device_info.identifier, sizeof(m_device_info.identifier));
	return true;
}

/**
 * Retrieve AlphaRNG major version
 *
 * @param[out] major_version points to location for major version number
 *
 * @return true for successful operation
 */
bool AlphaRngApi::retrieve_device_major_version(unsigned char *major_version) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	*major_version = m_device_info.major_version;
	return true;
}
/**
 * Retrieve AlphaRNG minor_version version
 *
 * @param[out] minor_version points to location for minor version number
 *
 * @return true for successful operation
 */
bool AlphaRngApi::retrieve_device_minor_version(unsigned char *minor_version) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	*minor_version = m_device_info.minor_version;
	return true;
}

/**
 * Retrieve AlphaRNG model
 *
 * @param[out]  model device model
 *
 * @return true for successful operation
 */
bool AlphaRngApi::retrieve_device_model(string &model) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}
	model.clear();
	model.append((char*)m_device_info.model, sizeof(m_device_info.model));
	return true;
}

bool AlphaRngApi::clear_receiver() {
	if (!is_initialized()) {
		return false;
	}
	unsigned char clear_buffer[128];
	int bytes_received;
	while (!m_device->receive_data(clear_buffer, sizeof(clear_buffer), &bytes_received));
	return true;
}

bool AlphaRngApi::execute_command (Response *resp, Command *cmd, int resp_payload_size_bytes) {
	if (!is_initialized() || !is_connected()) {
		return false;
	}

	if (cmd->payload_size > sizeof(cmd->payload)) {
		m_error_log_oss << "Command payload size invalid: " << cmd->payload_size << ". " << endl;
		return false;
	}

	for (int tries = 0; tries < c_max_command_retry_count; ++tries) {
		clear_error_log();
		if (execute_command_internal(resp, cmd, resp_payload_size_bytes)) {
			return true;
		}
		m_op_retry_count++;
		sleep_usecs(1000 * 100);
		clear_receiver();
		sleep_usecs(1000 * 100);
	}
	return false;
}

bool AlphaRngApi::create_token(uint64_t *new_token) {
	time_t seconds = time(NULL);
	uint64_t token = seconds;
	uint16_t rnd;
	if (!RAND_bytes((unsigned char*)&rnd, sizeof(rnd))) {
		return false;
	}
	token  = (token << 32) | (m_token_serial_number++ << 16) | rnd;
	*new_token = token;
	return true;
}

bool AlphaRngApi::execute_command_internal (Response *resp, Command *cmd, int resp_payload_size_bytes) {

	// Update the command structure
	cmd->e_mac_type = m_cfg.e_mac_type;
	int bytes_to_hash = (int)(sizeof(cmd->e_type) + sizeof(cmd->token) + sizeof(cmd->payload_size)) + cmd->payload_size;
	if (!create_token(&cmd->token)) {
		m_error_log_oss << "Could not create a command token" << ". " << endl;
		return false;
	}
	if (m_cfg.e_mac_type != MacType::None) {
		if (!m_hmac->hmac((const unsigned char *)&cmd->e_type, bytes_to_hash, cmd->mac)) {
			m_error_log_oss << "Could not compute the hash value for the command using hash type " << (int)m_cfg.e_mac_type << ". " << endl;
			return false;
		}
	}
	int cmd_struct_size_bytes = bytes_to_hash + (int)sizeof(cmd->e_mac_type) + sizeof(cmd->mac);

	if (!create_and_upload_command_packet((uint8_t*)cmd, cmd_struct_size_bytes)) {
		return false;
	}

	if (download_response(resp, get_resp_packet_payload_size(resp_payload_size_bytes))) {
		m_error_log_oss << "Could not execute command: " << (int)cmd->e_type << ". " << endl;
		return false;
	}

	if (cmd->token != resp->token) {
		m_error_log_oss << "Response token doesn't match command token" << ". " << endl;
		return false;
	}
	return true;
}

bool AlphaRngApi::upload_session_key() {

	// Create the session key
	Session sess;
	memset(&sess, 0x00, sizeof(sess));
	sess.e_type = SessionKeyType::aes;
	sess.e_size = m_cfg.e_aes_key_size;
	sess.e_mac_type = m_cfg.e_mac_type;

	if (!create_token(&sess.token)) {
		m_error_log_oss << "Could not create a session token" << ". " << endl;
		return false;
	}

	if (m_cfg.e_aes_key_size != KeySize::None) {
		if (!m_aes_cryptor->get_aad(sess.cipher_aad)) {
			m_error_log_oss << "Could not retrieve cipher AAD" << ". " << endl;
			return false;
		}
		if (!m_aes_cryptor->get_key(sess.key)) {
			m_error_log_oss << "Could not retrieve AES key" << ". " << endl;
			return false;
		}
	}

	int actual_size = (sizeof(sess)) - (sizeof(sess.mac));
	if (m_hmac->get_mac_size() > (int)sizeof(sess.mac)) {
		m_error_log_oss << "The MAC size " << m_hmac->get_mac_size() << "exceeds the expected size: " << sizeof(sess.mac) << ". " << endl;
		return false;
	}
	if (m_hmac->get_mac_size() > (int)sizeof(sess.mac_key)) {
		m_error_log_oss << "MAC key is too large" << ". " << endl;
		return false;
	}
	if (!m_hmac->get_mac_key(sess.mac_key)) {
		m_error_log_oss << "Could not retrieve the MAC key" << ". " << endl;
		return false;
	}
	if (m_cfg.e_mac_type != MacType::None) {
		if (!m_hmac->hmac((const unsigned char *)&sess, actual_size, sess.mac)) {
			m_error_log_oss << "Could not compute the MAC value for session key using type: " << (int)m_cfg.e_mac_type << ". " << endl;
			return false;
		}
	}
	int actual_size_bytes = actual_size + m_hmac->get_mac_size();

	if (!create_and_upload_session_packet((uint8_t*)&sess, actual_size_bytes)) {
		return false;
	}

	Response resp;
	if (download_response(&resp, get_resp_packet_payload_size(1))) {
		return false;
	}

	if (sess.token != resp.token) {
		m_error_log_oss << "Response token doesn't match session token" << ". " << endl;
		return false;
	}

	if (resp.payload_size != 1) {
		m_error_log_oss << "Response payload size invalid: " << (int)resp.payload_size << ". " << endl;
		return false;
	}
	uint8_t rng_status = resp.payload[0];
	if (rng_status != 0) {
		m_error_log_oss << "Received an unexpected RNG status byte: " << (int)rng_status << ". " << endl;
		return false;
	}
	return true;
}

bool AlphaRngApi::create_and_upload_command_packet(uint8_t *p, int object_size_bytes) {
	// Fill in the request structure
	Packet rqst;
	memset(&rqst, 0x00, sizeof(rqst));
	rqst.e_key_size = m_cfg.e_aes_key_size;
	rqst.e_type = get_aes_request_type();
	int cmd_packet_payload_size = get_cmd_packet_payload_size(object_size_bytes);
	rqst.payload_size = (uint16_t)cmd_packet_payload_size;
	Packet tmp;
	memset(&tmp, 0x00, sizeof(tmp));
	if ((int)sizeof(tmp.payload) < cmd_packet_payload_size) {
		m_error_log_oss << "The command packet payload size invalid: " << sizeof(cmd_packet_payload_size) << ". " << endl;
		return false;
	}
	if (m_cfg.e_aes_key_size == KeySize::None) {
		memcpy((uint8_t *)rqst.payload, p, object_size_bytes);
	} else {
		memcpy((uint8_t *)tmp.payload, p, object_size_bytes);

		if (!m_aes_cryptor->initialize_iv()) {
			m_error_log_oss << "Could not generate IV for the AES cipher" << ". " << endl;
			return false;
		}

		if (!m_aes_cryptor->get_iv(rqst.cipher_iv)) {
			m_error_log_oss << "Could not retrieve AES cipher IV" << ". " << endl;
			return false;
		}

		// Encrypt the payload
		int enc_byte_count = 0;
		if (!m_aes_cryptor->encrypt((const unsigned char *)tmp.payload,
				cmd_packet_payload_size, (unsigned char *)rqst.payload,
				&enc_byte_count, (unsigned char *)rqst.cipher_tag)
			|| enc_byte_count != cmd_packet_payload_size) {
			m_error_log_oss << "Could not encrypt the payload with the AES cipher" << ". " << endl;
			return false;
		}
	}
	// Upload the encrypted command
	if (!upload_request(&rqst)) {
		return false;
	}

	return true;
}

bool AlphaRngApi::create_and_upload_session_packet(uint8_t *p, int object_size_bytes) {
	// Fill in the request structure
	Packet rqst;
	memset(&rqst, 0x00, sizeof(rqst));
	rqst.e_type = get_rsa_request_type();

	rqst.payload_size = (uint16_t)m_cfg.e_rsa_key_size;
	Packet tmp;
	memset(&tmp, 0x00, sizeof(tmp));
	RAND_bytes(tmp.payload, sizeof(tmp.payload));
	if ((int)sizeof(tmp.payload) < object_size_bytes) {
		m_error_log_oss << "The actual payload size " << sizeof(tmp.payload) << " won't fit for " << object_size_bytes << ". " << endl;
		return false;
	}
	memcpy((uint8_t *)tmp.payload, p, object_size_bytes);


	if (m_cfg.e_aes_key_size != KeySize::None) {
		if (!m_aes_cryptor->initialize_iv()) {
			m_error_log_oss << "Could not generate AES IV for the session" << ". " << endl;
			return false;
		}

		if (!m_aes_cryptor->get_iv(rqst.cipher_iv)) {
			m_error_log_oss << "Could not retrieve AES cipher IV for session" << ". " << endl;
			return false;
		}
	}

	int encrypted_size_bytes;
	// Encrypt session key with the public key
	if (!m_rsa_cryptor->encrypt_with_public_key(tmp.payload, rqst.payload_size, rqst.payload, &encrypted_size_bytes)) {
		m_error_log_oss << "encrypt_with_public_key() failed to encrypt  " << rqst.payload_size << " bytes" << ". " << endl;
		return false;
	}
	if (encrypted_size_bytes != rqst.payload_size) {
		m_error_log_oss << "encrypt_with_public_key() encrypted " << encrypted_size_bytes << " bytes, expected " << rqst.payload_size << ". " << endl;
		return false;
	}

	// Upload the PK encrypted session key
	if (!upload_request(&rqst)) {
		return false;
	}
	return true;
}

int AlphaRngApi::get_cmd_packet_payload_size(int cmd_struct_size_bytes) const {
	int packet_size = cmd_struct_size_bytes;
	if (m_cfg.e_aes_key_size != KeySize::None) {
		// Add bytes for padding if needed
		int remainder = packet_size % (int)m_cfg.e_aes_key_size;
		if (remainder > 0) {
			packet_size += ((int)m_cfg.e_aes_key_size - remainder);
		}
	}
	return packet_size;
}

int AlphaRngApi::get_resp_packet_payload_size(int actual_payload_size_bytes) const {
	int packet_size =
			  sizeof(Response::e_mac_type)
			+ sizeof(Response::mac)
			+ sizeof(Response::token)
			+ sizeof(Response::payload_size)
			+ actual_payload_size_bytes;
	if (m_cfg.e_aes_key_size != KeySize::None) {
		// Add bytes for padding if needed
		int remainder = packet_size % (int)m_cfg.e_aes_key_size;
		if (remainder > 0) {
			packet_size += ((int)m_cfg.e_aes_key_size - remainder);
		}
	}
	return packet_size;
}

int AlphaRngApi::download_response(Response *resp, int resp_packet_payload_size) {
	Packet packet;
	int packet_receive_size = get_packet_size(resp_packet_payload_size);
	memset(&packet, 0x00, sizeof(packet));
	int actual_bytes_received;
	int resp_code = m_device->receive_data((unsigned char *)&packet, packet_receive_size, &actual_bytes_received);
	if (resp_code) {
		if (resp_code == -7) {
			m_error_log_oss << "Reached timeout when receiving data" << ". " << endl;
			m_error_log_oss << m_device->get_error_log();
		} else {
			m_error_log_oss << m_device->get_error_log() << ". " << endl;
		}
		return resp_code;
	}
	if (packet.e_type != PacketType::aes) {
		m_error_log_oss << "Received packet is not of type AES" << ". " << endl;
		return -1;
	}
	if (packet.e_key_size != m_cfg.e_aes_key_size) {
		m_error_log_oss << "Expected packet type with AES key size: " << (int)m_cfg.e_aes_key_size << ", but was: " << (int)packet.e_type << ". " << endl;
		return -1;
	}

	if (packet.payload_size != (uint16_t)resp_packet_payload_size) {
		m_error_log_oss << "Received packet has an invalid payload size: " << packet.payload_size << ". " << endl;
		return -1;
	}

	// Decrypt the payload
	if (packet.payload_size > sizeof(Response)) {
		m_error_log_oss << "Received packet payload won't fit in the response structure" << ". " << endl;
		return -1;
	}

	if (packet.e_key_size == KeySize::None) {
		memcpy(resp, packet.payload, packet.payload_size);
	} else {
		int dec_byte_count = 0;
		if (!m_aes_cryptor->decrypt((const unsigned char *)packet.payload,
				resp_packet_payload_size, (unsigned char *)resp,
				&dec_byte_count, packet.cipher_tag)
			|| dec_byte_count != resp_packet_payload_size) {
			m_error_log_oss << "Could not decrypt the payload using the AES cipher" << ". " << endl;
			ERR_print_errors_fp(stderr);
			return -1;
		}
	}

	// Validate response
	if (!is_response_valid(resp)) {
		return -1;
	}

	return 0;
}

bool AlphaRngApi::is_response_valid(Response *resp) {
	if (resp->e_mac_type != m_cfg.e_mac_type) {
		m_error_log_oss << "Response contains an invalid hash type: " << (int)resp->e_mac_type << ", expected: " << (int)m_cfg.e_mac_type << ". " << endl;
		return false;
	}
	if (resp->payload_size > (uint16_t)sizeof(resp->payload)) {
		m_error_log_oss << "Response has an invalid payload size: " << resp->payload_size << ". " << endl;
		return false;
	}

	// Verify the hash value
	Response tmp;
	if (m_cfg.e_mac_type != MacType::None) {
		int to_hash_bytes = sizeof(resp->token) + resp->payload_size + sizeof(resp->payload_size);
		if (!m_hmac->hmac((const unsigned char*)&resp->token, to_hash_bytes, tmp.mac)) {
			m_error_log_oss << "Could not compute hash value for the response" << ". " << endl;
			return false;
		}
		if (memcmp(resp->mac, tmp.mac, m_hmac->get_mac_size())) {
			m_error_log_oss << "Response hash value mismatch" << ". " << endl;
			return false;
		}
	}
	return true;
}

bool AlphaRngApi::upload_request(Packet *rqst) {
	int request_size_bytes = sizeof(rqst->e_type) + sizeof(rqst->e_key_size) + sizeof(rqst->cipher_iv)
		+ sizeof(rqst->cipher_tag) + sizeof(rqst->payload_size) + rqst->payload_size;
	int actual_bytes_sent = 0;
	if (m_device->send_data((unsigned char *)rqst, request_size_bytes, &actual_bytes_sent)) {
		m_error_log_oss << "send_data() expected to send  " << request_size_bytes << " bytes, actual bytes sent " << actual_bytes_sent << ". " << endl;
		return false;
	}
	return true;
}

/**
 * Disconnect from AlphaRNG device
 *
 * @return true for successful operation
 *
 */
bool AlphaRngApi::disconnect() {
	if (!is_initialized()) {
		return false;
	}
	clear_error_log();
	m_device_count = 0;
	return m_device->disconnect();
}

/**
 * Scan for connected AlphaRNG devices and return the count
 *
 * @return number of AlphaRNG devices currently plugged in
 */
int AlphaRngApi::get_device_count() {
	if (!is_initialized()) {
		return 0;
	}
	if (m_device_count > 0) {
		return m_device_count;
	}
	m_device->scan_available_devices();
	m_device_count = m_device->get_device_count();
	return m_device_count;
}

/**
 * Given a device number, retrieve the device path associated.
 * It should be called after get_device_count().
 *
 * @param[out] dev_path_name points to location to be filled with null terminated device path
 * @param[in] max_dev_path_name_bytes destination max size in bytes 
 * @param[in] device_number pass 0 for the first device
 *
 * @return true for successful operation
 *
 */
bool AlphaRngApi::retrieve_device_path(char *dev_path_name, int max_dev_path_name_bytes, int device_number) {
	if (!is_initialized()) {
		return false;
	}
	return m_device->retrieve_device_path(dev_path_name, max_dev_path_name_bytes, device_number);
}

void AlphaRngApi::clear_error_log() {
	m_error_log_oss.str("");
	m_error_log_oss.clear();
	if (m_device) {
		m_device->clear_error_log();
	}
}

AlphaRngApi::~AlphaRngApi() {
	if (m_hmac) {
		delete m_hmac;
	}
	if (m_device) {
		delete m_device;
	}
	if (m_rsa_cryptor) {
		delete m_rsa_cryptor;
	}
	if (m_aes_cryptor) {
		delete m_aes_cryptor;
	}
	if (m_device_name) {
		delete [] m_device_name;
	}
	if (m_file_buffer) {
		delete [] m_file_buffer;
	}
	if (m_sha_256) {
		delete m_sha_256;
	}
	if (m_sha_512) {
		delete m_sha_512;
	}
	if (m_sha_ent_extr) {
		delete m_sha_ent_extr;
	}
}

} /* namespace alpharng */
