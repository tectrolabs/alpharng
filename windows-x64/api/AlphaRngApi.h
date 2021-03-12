/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the API for interacting with the AlphaRNG device over a secure data communication channel.

 */

/**
 *    @file AlphaRngApi.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
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

#ifdef _WIN64
#include <WinUsbSerialDevice.h>
#else
#include <UsbSerialDevice.h>
#endif


using namespace std;

namespace alpharng {


class AlphaRngApi {
public:

	AlphaRngApi();
	AlphaRngApi(AlphaRngConfig cfg);
	bool is_connected();
	bool connect(int device_number);
	bool disconnect();
	int get_device_count();
	bool retrieve_device_path(char *dev_path_name, int max_dev_path_name_bytes, int device_number);
	string get_last_error() {return m_error_log_oss.str();}
	bool retrieve_rng_status(unsigned char *status);
	bool retrieve_device_id(string &id);
	bool retrieve_device_model(string &model);
	bool retrieve_device_major_version(unsigned char *major_version);
	bool retrieve_device_minor_version(unsigned char *minor_version);
	bool run_health_test();
	bool retrieve_frequency_tables(FrequencyTables &freq_tables);
	bool get_noise_source_1(unsigned char *out, int out_length);
	bool get_noise_source_2(unsigned char *out, int out_length);
	bool get_entropy(unsigned char *out, int out_length);
	bool get_noise(unsigned char *out, int out_length);
	bool get_test_data(unsigned char *out, int out_length);
	bool entropy_to_file(const string &file_path_name, int64_t num_bytes);
	bool noise_source_one_to_file(string &file_path_name, int64_t num_bytes);
	bool noise_source_two_to_file(string &file_path_name, int64_t num_bytes);
	bool noise_to_file(const string &file_path_name, int64_t num_bytes);
	HealthTests get_health_tests() {return m_health_test;}
	int get_operation_retry_count() {return m_op_retry_count;}

	virtual	~AlphaRngApi();
	bool is_initialized() {return m_is_initialized;}


private:
	void initialize(AlphaRngConfig cfg);
	bool initialize_rsa();
	bool initialize_hash(MacType e_mac_type);
	bool initialize_aes(KeySize e_aes_key_size);
	void clear_error_log();
	bool upload_session_key();
	bool upload_request(Packet *rqst);
	int download_response(Response *resp, int resp_packet_payload_size);
	bool is_response_valid(Response *resp);
	int get_resp_packet_payload_size(int actual_payload_size);
	int get_cmd_packet_payload_size(int cmd_struct_size_bytes);
	PacketType get_rsa_request_type();
	PacketType get_aes_request_type();
	int get_packet_size(int resp_packet_payload_size_bytes);
	bool create_and_upload_session_packet(uint8_t *p, int object_size_bytes);
	bool create_and_upload_command_packet(uint8_t *p, int object_size_bytes);
	bool execute_command (Response *resp, Command *cmd, int resp_payload_size_bytes);
	bool clear_receiver();
	bool retrieve_device_info(DeviceInfo *device_info);
	void clear_command(Command *cmd);
	void clear_response(Response *resp);
	bool get_bytes(CommandType cmd_type, unsigned char *out, int out_length, int block_size_bytes, bool test_data);
	bool get_unpacked_bytes(char cmd, unsigned char *out, int out_length, int block_size_bytes, bool test_data);
	bool get_unpacked_bytes_with_retry(char cmd, unsigned char *out, int out_length);
	bool to_file(CommandType cmd_type, const string &file_path_name, int64_t num_bytes);
	bool get_data(CommandType cmd_type, unsigned char *out, int out_length);
	bool execute_command_internal (Response *resp, Command *cmd, int resp_payload_size_bytes);
	bool connect_internal(int device_number);
	bool initialize_rsa_keyfile();
	bool initialize_serial_device();
	bool create_token(uint64_t *token);
	void sleep_usecs(int usec);

private:
	HmacInterface *m_hmac = nullptr;
	DeviceInterface *m_device = nullptr;
	ostringstream m_error_log_oss;
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
	AlphaRngConfig m_cfg;
};

} /* namespace alpharng */

#endif /* ALPHARNG_H_ */
