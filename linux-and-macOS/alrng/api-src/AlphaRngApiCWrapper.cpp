/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements a C API wrapper around the C++ API for interacting with the AlphaRNG device over a secure data communication channel.

 Most of C wrapper functions will return:
	0 when invoked successfully;
	-1 for invalid parameters;
	-2 for other errors (invoke alrng_get_last_error() to retrieve the error message)
 */

/**
 *    @file AlphaRngApiCWrapper.cpp
 *    @date 9/16/2023
 *    @Author: Andrian Belinski
 *    @version 1.2
 *
 *    @brief Implements a C wrapper around the C++ API for securely interacting with the AlphaRNG device.
 */
#include <AlphaRngApi.h>
#include <AlphaRngApiCWrapper.h>

using namespace alpharng;

extern "C" {

/**
 * Create a context for referencing AlphaRngApi class instance using default security configuration:
 *
 * MAC type:        HMAC-SHA-256
 * public key type: RSA-2048
 * Cipher type:     AES-256-GCM
 * public key file: NONE
 *
 * @return pointer to the new context or NULL if failed
 */
alrng_context* alrng_create_default_ctxt() {
	return (alrng_context*) new (std::nothrow) AlphaRngApi();
}

/**
 * Create a context for referencing AlphaRngApi class instance using specific security configuration
 *
 * @param[in] rsa_key_type RSA public key type used for establishing a secure session with an AlphaRNG device
 * @param[in] mac_type MAC type used for data authentication
 * @param[in] cipher_type AES cipher is used for securing the data communication within an AlphaRNG session
 * @param[in] pub_key_file file pathname with an alternative RSA 2048 public key, supplied by the manufacturer
 *
 * @return pointer to the new context or NULL if failed
 */
alrng_context* alrng_create_ctxt(enum alrng_rsa_key_type rsa_key_type, enum alrng_mac_type mac_type, enum alrng_cipher_type cipher_type, const char *pub_key_file) {
	MacType e_mac_type;
	KeySize e_aes_key_size;
	std::string key_file;
	RsaKeySize e_rsa_key_size;

	switch(rsa_key_type) {
	case rsa_1024_key:
		e_rsa_key_size = RsaKeySize::rsa1024;
		break;
	case rsa_2048_key:
	default:
		e_rsa_key_size = RsaKeySize::rsa2048;
		break;
	}

	switch(mac_type) {
	case mac_type_none:
		e_mac_type = MacType::None;
		break;
	case hmac_md5:
		e_mac_type = MacType::hmacMD5;
		break;
	case hmac_sha_160:
		e_mac_type = MacType::hmacSha160;
		break;
	case hmac_sha_256:
	default:
		e_mac_type = MacType::hmacSha256;
		break;
	}

	switch(cipher_type) {
	case cipher_type_none:
		e_aes_key_size = KeySize::None;
		break;
	case aes_128_gcm:
		e_aes_key_size = KeySize::k128;
		break;
	case aes_256_gcm:
	default:
		e_aes_key_size = KeySize::k256;
		break;
	}

	if (nullptr == pub_key_file) {
		key_file = "";
	} else {
		key_file = pub_key_file;
	}

	return (alrng_context*) new (std::nothrow) AlphaRngApi(AlphaRngConfig {e_mac_type, e_rsa_key_size, e_aes_key_size, key_file});
}

/**
 * Establish a connection with AlphaRNG device specified by `device_number`
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[in] device_number device number, 0 for the first device
 *
 * @return 0 if connection was successful
 *
 */
int alrng_connect(alrng_context *ctxt, int device_number) {
	if (nullptr == ctxt) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	bool status = api->connect(device_number);
	return status ? 0 : -2;
}

/**
 * Check to see if connection is successfully established with the device
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 *
 * @return 0 if connection has been successfully established
 */
int alrng_is_connected(alrng_context* ctxt) {
	if (nullptr == ctxt) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	bool status = api->is_connected();
	return status ? 0 : -2;
}

/**
 * Disconnect from AlphaRNG device
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 *
 * @return 0 for successful operation
 *
 */
int alrng_disconnect(alrng_context *ctxt) {
	if (nullptr == ctxt) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	bool status = api->disconnect();
	return status ? 0 : -2;
}

/**
 * Close any active connection and destroy context that references AlphaRngApi class instance.
 *
 * @return 0 for successful operation
 */

int alrng_destroy_ctxt(alrng_context *ctxt) {
	if (nullptr == ctxt) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	// Close any existing connection to a device
	api->disconnect();
	delete api;
	return 0;
}

/**
 * Scan for connected AlphaRNG devices and return the count
 *
 * @return number of AlphaRNG devices currently plugged in or a negative number for failed operation
 */
int alrng_get_device_count(alrng_context* ctxt) {
	if (nullptr == ctxt) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	return api->get_device_count();
}

/**
 * Given a device number, retrieve the device path associated.
 * It should be called after alrng_get_device_count().
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] dev_path_name points to location to be filled with null terminated device path
 * @param[in] max_dev_path_name_bytes destination max size in bytes
 * @param[in] device_number pass 0 for the first device
 *
 * @return 0 for successful operation
 *
 */
int alrng_retrieve_device_path(alrng_context* ctxt, char *dev_path_name, int max_dev_path_name_bytes, int device_number) {
	if (nullptr == ctxt) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	bool status = api->retrieve_device_path(dev_path_name, max_dev_path_name_bytes, device_number);
	return status ? 0 : -2;
}

/**
 * Retrieve the message associated with the last error.
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] msg_buffer points to a location for storing zero terminated error message
 * @param[in] msg_buffer_size the memory allocated to msg_buffer in bytes
 *
 * @return 0 for successful operation
 *
 */
int alrng_get_last_error(alrng_context* ctxt, char *msg_buffer, int msg_buffer_size) {
	if (nullptr == ctxt || nullptr == msg_buffer || msg_buffer_size <= 2) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	std::string msg = api->get_last_error();
	int size = (int)msg.size();
	if (size >= msg_buffer_size) {
		size = msg_buffer_size -1;
	}
	memcpy(msg_buffer, msg.c_str(), size);
	msg_buffer[size] = '\0';
	return 0;
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
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] status points to location for storing the RNG status
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_rng_status(alrng_context* ctxt, unsigned char *status) {
	if (nullptr == ctxt) {
		return -1;
	}

	auto api = (AlphaRngApi*) ctxt;
	bool ret = api->retrieve_rng_status(status);
	return ret ? 0 : -2;
}

/**
 * Retrieve AlphaRNG device identification number or serial number
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] id_buffer points to a location to be filled with null terminated serial number as text
 * @param[in] id_buffer_size the memory allocated to id_buffer in bytes (should be 16)
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_device_id(alrng_context* ctxt, char *id_buffer, int id_buffer_size) {
	if (nullptr == ctxt || nullptr == id_buffer || id_buffer_size < 16) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	std::string id;
	bool status = api->retrieve_device_id(id);
	if (false == status) {
		return -2;
	}
	int size = (int)id.size();
	if (size >= 16) {
		return -1;
	}
	memcpy(id_buffer, id.c_str(), size);
	id[size] = '\0';
	return 0;
}

/**
 * Retrieve AlphaRNG model
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] model_buffer points to a location to be filled with null terminated model as text
 * @param[in] model_buffer_size the memory allocated to model_buffer in bytes (should be 16)
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_device_model(alrng_context* ctxt, char *model_buffer, int model_buffer_size) {
	if (nullptr == ctxt || nullptr == model_buffer || model_buffer_size < 16) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	std::string model;
	bool status = api->retrieve_device_model(model);
	if (false == status) {
		return -2;
	}
	int size = (int)model.size();
	if (size >= 16) {
		return -1;
	}
	memcpy(model_buffer, model.c_str(), size);
	model[size] = '\0';
	return 0;
}

/**
 * Retrieve AlphaRNG major version
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] major_version points to location for major version number
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_device_major_version(alrng_context* ctxt, unsigned char *major_version) {
	if (nullptr == ctxt || nullptr == major_version) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	bool status = api->retrieve_device_major_version(major_version);
	return status ? 0 : -2;
}

/**
 * Retrieve AlphaRNG minor_version version
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] minor_version points to location for minor version number
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_device_minor_version(alrng_context* ctxt, unsigned char *minor_version) {
	if (nullptr == ctxt || nullptr == minor_version) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	bool status = api->retrieve_device_minor_version(minor_version);
	return status ? 0 : -2;
}

/**
 * Invoke device internal health test suite
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 *
 * @return 0 for successful operation
 */
int alrng_run_health_test(alrng_context* ctxt) {
	if (nullptr == ctxt) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	bool status = api->run_health_test();
	return status ? 0 : -2;
}

/**
 * Retrieve digitized raw (unprocessed) random bytes from the first noise source of the AlphaRNG device.
 * It is used to evaluate the quality of the first random noise source.
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out points to a byte arrays for storing the resulting bytes
 * @param[in] out_length how many bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_get_noise_source_1(alrng_context* ctxt, unsigned char *out, int out_length) {
	if (nullptr == ctxt || nullptr == out || out_length < 1 ) {
		return -1;
	}

	auto api = (AlphaRngApi*) ctxt;
	bool status = api->get_noise_source_1(out, out_length);
	return status ? 0 : -2;
}

/**
 * Retrieve digitized raw (unprocessed) random bytes from the second noise source of the AlphaRNG device.
 * It is used to evaluate the quality of the second random noise source.
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out points to a byte arrays for storing the resulting bytes
 * @param[in] out_length how many bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_get_noise_source_2(alrng_context* ctxt, unsigned char *out, int out_length) {
	if (nullptr == ctxt || nullptr == out || out_length < 1 ) {
		return -1;
	}

	auto api = (AlphaRngApi*) ctxt;
	bool status = api->get_noise_source_2(out, out_length);
	return status ? 0 : -2;
}

/**
 * Retrieve entropy bytes from the AlphaRNG device.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or can feed the entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out points to a byte array for storing the random bytes retrieved
 * @param[in] out_length how many random bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_get_entropy(alrng_context* ctxt, unsigned char *out, int out_length) {
	if (nullptr == ctxt || nullptr == out || out_length < 1 ) {
		return -1;
	}

	auto api = (AlphaRngApi*) ctxt;
	bool status = api->get_entropy(out, out_length);
	return status ? 0 : -2;
}

/**
 * Extract entropy bytes by applying SHA-256 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for generating high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out points to a byte array for storing the random bytes extracted
 * @param[in] out_length how many entropy bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_extract_sha256_entropy(alrng_context* ctxt, unsigned char *out, int out_length) {
	if (nullptr == ctxt || nullptr == out || out_length < 1 ) {
		return -1;
	}

	auto api = (AlphaRngApi*) ctxt;
	bool status = api->extract_sha256_entropy(out, out_length);
	return status ? 0 : -2;
}

/**
 * Extract entropy bytes by applying SHA-512 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for generating high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out points to a byte array for storing the random bytes extracted
 * @param[in] out_length how many entropy bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_extract_sha512_entropy(alrng_context* ctxt, unsigned char *out, int out_length) {
	if (nullptr == ctxt || nullptr == out || out_length < 1 ) {
		return -1;
	}

	auto api = (AlphaRngApi*) ctxt;
	bool status = api->extract_sha512_entropy(out, out_length);
	return status ? 0 : -2;
}

/**
 * Retrieve concatenated raw random bytes of both noise sources from the AlphaRNG device.
 * It is used to evaluate the quality of the raw random byte stream produced by both noise sources
 * before any post-processing or conditioning.
 * The byte stream can also be used as an input for alternative post-processing or conditioning algorithms
 * such as SHA and HMAC extractors.
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out points to a byte array for storing the random bytes retrieved
 * @param[in] out_length how many random bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_get_noise(alrng_context* ctxt, unsigned char *out, int out_length) {
	if (nullptr == ctxt || nullptr == out || out_length < 1 ) {
		return -1;
	}

	auto api = (AlphaRngApi*) ctxt;
	bool status = api->get_noise(out, out_length);
	return status ? 0 : -2;
}

/**
 * This method is only used for testing the correctness of the data communication with the
 * SecireRNG device. Each byte retrieved, starting with 0, represents an incremented value of
 * the previous byte value.
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out points to a byte array for storing the test data
 * @param[in] out_length how many bytes of test data to receive
 *
 * @return 0 for successful operation
 *
 */
int alrng_get_test_data(alrng_context* ctxt, unsigned char *out, int out_length) {
	if (nullptr == ctxt || nullptr == out || out_length < 1 ) {
		return -1;
	}

	auto api = (AlphaRngApi*) ctxt;
	bool status = api->get_test_data(out, out_length);
	return status ? 0 : -2;
}

/**
 * Retrieve entropy bytes from the AlphaRNG device and store those into a file.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or can feed the entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out file_path_name pointer to file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_entropy_to_file(alrng_context* ctxt, const char *file_path_name, int64_t num_bytes) {
	if (nullptr == ctxt || nullptr == file_path_name || num_bytes < 0 ) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	std::string file_path_name_str(file_path_name);
	bool status = api->entropy_to_file(file_path_name_str, num_bytes);
	return status ? 0 : -2;
}

/**
 * Extract entropy bytes into a file by applying SHA-256 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out file_path_name pointer to file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_extract_sha256_entropy_to_file(alrng_context* ctxt, const char *file_path_name, int64_t num_bytes) {
	if (nullptr == ctxt || nullptr == file_path_name || num_bytes < 0 ) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	std::string file_path_name_str(file_path_name);
	bool status = api->extract_sha256_entropy_to_file(file_path_name_str, num_bytes);
	return status ? 0 : -2;
}

/**
 * Extract entropy bytes into a file by applying SHA-512 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out file_path_name pointer to file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_extract_sha512_entropy_to_file(alrng_context* ctxt, const char *file_path_name, int64_t num_bytes) {
	if (nullptr == ctxt || nullptr == file_path_name || num_bytes < 0 ) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	std::string file_path_name_str(file_path_name);
	bool status = api->extract_sha512_entropy_to_file(file_path_name_str, num_bytes);
	return status ? 0 : -2;
}

/**
 * Retrieve digitized raw (unprocessed) random bytes from the first noise source of the AlphaRNG device
 * and store the bytes received into a file.
 * It is used to evaluate the quality of the first random noise source.
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[in] file_path_name file path name for storing the retrieved bytes
 * @param[in] num_bytes how many bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_noise_source_one_to_file(alrng_context* ctxt, const char *file_path_name, const int64_t num_bytes) {
	if (nullptr == ctxt || nullptr == file_path_name || num_bytes < 0 ) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	std::string file_path_name_str(file_path_name);
	bool status = api->noise_source_one_to_file(file_path_name_str, num_bytes);
	return status ? 0 : -2;
}

/**
 * Retrieve digitized raw (unprocessed) random bytes from the second noise source of the AlphaRNG device
 * and store the bytes received into a file.
 * It is used to evaluate the quality of the second random noise source.
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[in] file_path_name file path name for storing the retrieved bytes
 * @param[in] num_bytes how many bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_noise_source_two_to_file(alrng_context* ctxt, const char *file_path_name, const int64_t num_bytes) {
	if (nullptr == ctxt || nullptr == file_path_name || num_bytes < 0 ) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	std::string file_path_name_str(file_path_name);
	bool status = api->noise_source_two_to_file(file_path_name_str, num_bytes);
	return status ? 0 : -2;
}

/**
 * Retrieve raw (unprocessed) bytes from the AlphaRNG device and store those into a file.
 * It is used to evaluate the quality of the raw random byte stream produced by both noise sources
 * before any post-processing or conditioning.
 * The byte stream can also be used as an input for alternative post-processing or conditioning algorithms.
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] out file_path_name file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_noise_to_file(alrng_context* ctxt, const char *file_path_name, int64_t num_bytes) {
	if (nullptr == ctxt || nullptr == file_path_name || num_bytes < 0 ) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	std::string file_path_name_str(file_path_name);
	bool status = api->noise_to_file(file_path_name_str, num_bytes);
	return status ? 0 : -2;
}

/**
 * Retrieve random values frequency distribution for both noise sources.
 * That information can be used for evaluating the quality of
 * noise sources.
 *
 * @param[in] ctxt pointer to context structure, must not be nullptr
 * @param[out] freq_table_1 pointer to a memory allocated to store 256 uint16_t values for the first noise source
 * @param[out] freq_table_2 pointer to a memory allocated to store 256 uint16_t values for the second noise source
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_frequency_tables(alrng_context* ctxt, uint16_t *freq_table_1, uint16_t *freq_table_2) {
	if (nullptr == ctxt || nullptr == freq_table_1 || nullptr == freq_table_2 ) {
		return -1;
	}
	auto api = (AlphaRngApi*) ctxt;
	FrequencyTables freq_tables;
	bool status = api->retrieve_frequency_tables(&freq_tables);
	if (false == status) {
		return -2;
	}
	memcpy(freq_table_1, freq_tables.freq_table_1, sizeof(freq_tables.freq_table_1));
	memcpy(freq_table_2, freq_tables.freq_table_2, sizeof(freq_tables.freq_table_2));
	return 0;
}

}

