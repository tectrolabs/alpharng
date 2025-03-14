/**
 Copyright (C) 2014-2025 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the API for interacting with the AlphaRNG device over a secure data communication channel.

 */

/**
 *    @file AlphaRngApi.h
 *    @date 03/09/2025
 *    @Author: Andrian Belinski
 *    @version 1.8
 *
 *    @brief Implements the API for securely interacting with the AlphaRNG device.
 */

#ifndef ALPHARNG_H_
#define ALPHARNG_H_

#include <string>
#include <sstream>
#include <fstream>
#include <chrono>
#include <thread>

#include <RsaCryptor.h>
#include <AesCryptor.h>
#include <RsaKeyRepo.h>
#include <DeviceInterface.h>
#include <HmacInterface.h>
#include <HmacSha256.h>
#include <HmacSha1.h>
#include <HmacMD5.h>
#include <AlphaRngConfig.h>
#include <HealthTests.h>
#include <Structures.h>
#include <Sha256.h>
#include <Sha512.h>
#include <ShaInterface.h>
#include <ShaEntropyExtractor.h>

#ifdef _WIN64
#include <WinUsbSerialDevice.h>
#else
#include <UsbSerialDevice.h>
#endif

namespace alpharng {

class ShaEntropyExtractor;

class AlphaRngApi {
public:

	AlphaRngApi();
	explicit AlphaRngApi(const AlphaRngConfig &cfg);
	bool is_connected();
	bool connect(int device_number);
	bool disconnect();
	int get_device_count();
	bool retrieve_device_path(char *dev_path_name, int max_dev_path_name_bytes, int device_number);
	std::string get_last_error() const {return m_error_log_oss.str();}
	bool retrieve_rng_status(unsigned char *status);
	bool retrieve_device_id(std::string &id);
	bool retrieve_device_model(std::string &model);
	bool retrieve_device_major_version(unsigned char *major_version);
	bool retrieve_device_minor_version(unsigned char *minor_version);
	bool run_health_test();
	bool retrieve_frequency_tables(FrequencyTables *freq_tables);
	bool get_noise_source_1(unsigned char *out, int out_length);
	bool get_noise_source_2(unsigned char *out, int out_length);
	bool get_entropy(unsigned char *out, int out_length);
	bool extract_sha256_entropy(unsigned char *out, int out_length);
	bool extract_sha512_entropy(unsigned char *out, int out_length);
	bool get_noise(unsigned char *out, int out_length);
	bool get_test_data(unsigned char *out, int out_length);
	bool entropy_to_file(const std::string &file_path_name, int64_t num_bytes);
	bool extract_sha256_entropy_to_file(const std::string &file_path_name, int64_t num_bytes);
	bool extract_sha512_entropy_to_file(const std::string &file_path_name, int64_t num_bytes);
	bool noise_source_one_to_file(const std::string &file_path_name, const int64_t num_bytes);
	bool noise_source_two_to_file(const std::string &file_path_name, const int64_t num_bytes);
	bool noise_to_file(const std::string &file_path_name, int64_t num_bytes);
	void disable_stat_tests();
	void enable_stat_tests();
	void set_num_failures_threshold(uint8_t num_failures_threshold);
	bool set_session_ttl(time_t time_to_live_minutes);

	HealthTests &get_health_tests() {return m_health_test;}
	int get_operation_retry_count() const {return m_op_retry_count;}
	int get_session_count() const {return m_session_count;}
	AlphaRngConfig& get_configuration() { return m_cfg; }

	virtual	~AlphaRngApi();
	bool is_initialized() const {return m_is_initialized;}


private:
	void initialize(const AlphaRngConfig &cfg);
	bool initialize_rsa();
	bool initialize_sha();
	bool initialize_hmac(MacType e_mac_type);
	bool initialize_aes(KeySize e_aes_key_size);
	bool initialize_entropy_extractor(ShaInterface *sha_api);
	void clear_error_log();
	bool upload_session_key();
	bool create_new_session();
	bool upload_request(Packet *rqst);
	int download_response(Response *resp, int resp_packet_payload_size);
	bool is_response_valid(Response *resp);
	int get_resp_packet_payload_size(int actual_payload_size_bytes) const;
	int get_cmd_packet_payload_size(int cmd_struct_size_bytes) const;
	bool extract_entropy(ShaInterface *sha_api, unsigned char *out, int out_length);
	PacketType get_rsa_request_type() const;
	static PacketType get_aes_request_type() {return PacketType::aes;};
	bool create_and_upload_session_packet(uint8_t *p, int object_size_bytes);
	bool create_and_upload_command_packet(uint8_t *p, int object_size_bytes);
	bool execute_command (Response *resp, Command *cmd, int resp_payload_size_bytes);
	bool clear_receiver();
	bool retrieve_device_info(DeviceInfo *device_info);
	static void clear_command(Command *cmd) {memset(cmd, 0x7f, sizeof(Command));}
	static void clear_response(Response *resp) {memset(resp, 0x5c, sizeof(Response));}
	bool get_bytes(CommandType cmd_type, unsigned char *out, int out_length, int block_size_bytes, bool test_data);
	bool get_unpacked_bytes_with_retry(char cmd, unsigned char *out, int out_length, int block_size_bytes, bool test_data);
	bool get_unpacked_bytes(char cmd, unsigned char *out, int out_length, int block_size_bytes, bool test_data);
	bool get_payload_bytes_with_retry(char cmd, unsigned char *out, int out_length);
	bool to_file(CommandType cmd_type, const std::string &file_path_name, int64_t num_bytes);
	bool get_data(CommandType cmd_type, unsigned char *out, int out_length);
	bool execute_command_internal (Response *resp, Command *cmd, int resp_payload_size_bytes);
	bool connect_internal(int device_number);
	bool initialize_rsa_keyfile();
	bool initialize_serial_device();
	bool create_token(uint64_t *new_token);
	static void sleep_usecs(int usec) {std::this_thread::sleep_for(std::chrono::microseconds(usec));}
	static int get_packet_size(int resp_packet_payload_size_bytes) {
		return sizeof(Packet::e_type) + sizeof(Packet::e_key_size) + sizeof(Packet::cipher_iv)
		+ sizeof(Packet::cipher_tag) + sizeof(Packet::payload_size)	+ resp_packet_payload_size_bytes;}

private:
	HmacInterface *m_hmac = nullptr;
	DeviceInterface *m_device = nullptr;
	std::ostringstream m_error_log_oss;
	RsaCryptor *m_rsa_cryptor = nullptr;
	AesCryptor *m_aes_cryptor = nullptr;
	RsaKeyRepo m_rsa_key_repo;
	bool m_is_initialized = false;
	int m_device_count;
	const int c_max_command_retry_count = 3;
	uint16_t m_token_serial_number;
	char *m_device_name = nullptr;
	const int m_max_device_name_size = 128;
	DeviceInfo m_device_info;
	const int c_slow_timeout_mlsecs = 4000;
	const int c_fast_timeout_mlsecs = 300;
	const int c_rnd_data_block_size_bytes = 16000;
	const int c_test_data_block_size_bytes = 256;
	const int c_file_output_buff_size_bytes = 100000;
	const int64_t c_max_file_ouput_bytes = 200000000000LL;
	unsigned char *m_file_buffer = nullptr;
	HealthTests m_health_test;
	int m_op_retry_count;
	int m_session_count = 0;
	AlphaRngConfig m_cfg;
	ShaInterface *m_sha_256 = nullptr;
	ShaInterface *m_sha_512 = nullptr;
	ShaEntropyExtractor *m_sha_ent_extr = nullptr;
	time_t m_expire_time_secs = 0;
	time_t m_time_to_live_mins = 0;

};

} /* namespace alpharng */

#endif /* ALPHARNG_H_ */
