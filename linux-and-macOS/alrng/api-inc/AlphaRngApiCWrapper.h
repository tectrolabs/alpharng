/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements a C wrapper around the C++ API for interacting with the AlphaRNG device over a secure data communication channel.

 */

/**
 *    @file AlphaRngApiCWrapper.h
 *    @date 06/28/2023
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Implements a C API wrapper around the C++ API for securely interacting with the AlphaRNG device.
 */
#ifndef __ALRNGCWRAPPER_H
#define __ALRNGCWRAPPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define security attributes */
enum alrng_rsa_key_type {rsa_2048_key = 256, rsa_1024_key = 128};
enum alrng_mac_type {mac_type_none = 0, hmac_md5 = 16, hmac_sha_160 = 20, hmac_sha_256 = 32};
enum alrng_cipher_type {cipher_type_none = 0, aes_256_gcm = 32, aes_128_gcm = 16};

/* Define a type for referencing the API context */
typedef struct alrng_context alrng_context;

/**
 * Create a context for referencing AlphaRngApi class instance using default security configuration:
 *
 * MAC type:        HMAC-SHA-256
 * public key type: RSA-2048
 * Cipher type:     AES-256-GCM
 * public key file: NONE
 *
 * @return pointer to the new context
 */
alrng_context* alrng_create_default_ctxt();

/**
 * Create a context for referencing AlphaRngApi class instance using specific security configuration
 *
 * @param[in] rsa_key_type RSA public key type used for establishing a secure session with an AlphaRNG device
 * @param[in] mac_type MAC type used for data authentication
 * @param[in] cipher_type AES cipher is used for securing the data communication within an AlphaRNG session
 * @param[in] pub_key_file file pathname with an alternative RSA 2048 public key, supplied by the manufacturer
 *
 * @return pointer to the new context
 */
alrng_context* alrng_create_ctxt(enum alrng_rsa_key_type rsa_key_type, enum alrng_mac_type mac_type, enum alrng_cipher_type cipher_type, const char *pub_key_file);

/**
 * Establish a connection with AlphaRNG device specified by `device_number`
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[in] device_number device number, 0 for the first device
 *
 * @return 0 if connection was successful
 *
 */
int alrng_connect(alrng_context* ctxt, int device_number);

/**
 * Check to see if connection is successfully established with the device
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 *
 * @return 0 if connection has been successfully established
 */
int alrng_is_connected(alrng_context* ctxt);


/**
 * Disconnect from AlphaRNG device
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 *
 * @return 0 for successful operation
 *
 */
int alrng_disconnect(alrng_context* ctxt);

/**
 * Destroy context that references AlphaRngApi class instance.
 * It can be invoked after calling alrng_disconnect(()
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 *
 * @return 0 for successful operation
 */
int alrng_destroy_ctxt(alrng_context* ctxt);

/**
 * Scan for connected AlphaRNG devices and return the count
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 *
 * @return number of AlphaRNG devices currently plugged in or negative number for failed operation
 */
int alrng_get_device_count(alrng_context* ctxt);

/**
 * Given a device number, retrieve the device path associated.
 * It should be called after alrng_get_device_count().
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] dev_path_name points to location to be filled with null terminated device path
 * @param[in] max_dev_path_name_bytes destination max size in bytes
 * @param[in] device_number pass 0 for the first device
 *
 * @return 0 for successful operation
 *
 */
int alrng_retrieve_device_path(alrng_context* ctxt, char *dev_path_name, int max_dev_path_name_bytes, int device_number);

/**
 * Retrieve the message associated with the last error.
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] msg_buffer points to a location for storing a zero terminated error message
 * @param[in] msg_buffer_size the memory allocated to msg_buffer in bytes
 *
 * @return 0 for successful operation
 *
 */
int  alrng_get_last_error(alrng_context* ctxt, char *msg_buffer, int msg_buffer_size);

/**
 * Retrieve AlphaRNG internal health status.
 *
 * It may have one of the following values:
 * 0 - healthy
 * 4 - frequency distribution test failure
 * 2 - APT test error
 * 1 - RCT test error
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] status points to location for storing the RNG status
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_rng_status(alrng_context* ctxt, unsigned char *status);

/**
 * Retrieve AlphaRNG device identification number or serial number
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] id_buffer points to a location to be filled with null terminated serial number as text
 * @param[in] id_buffer_size the memory allocated to id_buffer in bytes (should be 16)
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_device_id(alrng_context* ctxt, char *id_buffer, int id_buffer_size);

/**
 * Retrieve AlphaRNG model
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] model_buffer points to a location to be filled with null terminated model as text
 * @param[in] model_buffer_size the memory allocated to model_buffer in bytes (should be 16)
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_device_model(alrng_context* ctxt, char *model_buffer, int model_buffer_size);

/**
 * Retrieve AlphaRNG major version
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] major_version points to location for major version number
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_device_major_version(alrng_context* ctxt, unsigned char *major_version);

/**
 * Retrieve AlphaRNG minor_version version
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] minor_version points to location for minor version number
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_device_minor_version(alrng_context* ctxt, unsigned char *minor_version);

/**
 * Invoke device internal health test suite
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 *
 * @return 0 for successful operation
 */
int alrng_run_health_test(alrng_context* ctxt);

/**
 * Retrieve digitized raw (unprocessed) random bytes from the first noise source of the AlphaRNG device.
 * It is used to evaluate the quality of the first random noise source.
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out points to a byte arrays for storing the resulting bytes
 * @param[in] out_length how many bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_get_noise_source_1(alrng_context* ctxt, unsigned char *out, int out_length);

/**
 * Retrieve digitized raw (unprocessed) random bytes from the second noise source of the AlphaRNG device.
 * It is used to evaluate the quality of the second random noise source.
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out points to a byte arrays for storing the resulting bytes
 * @param[in] out_length how many bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_get_noise_source_2(alrng_context* ctxt, unsigned char *out, int out_length);

/**
 * Retrieve entropy bytes from the AlphaRNG device.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or can feed the entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out points to a byte array for storing the random bytes retrieved
 * @param[in] out_length how many random bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_get_entropy(alrng_context* ctxt, unsigned char *out, int out_length);

/**
 * Extract entropy bytes by applying SHA-256 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for generating high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out points to a byte array for storing the random bytes extracted
 * @param[in] out_length how many entropy bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_extract_sha256_entropy(alrng_context* ctxt, unsigned char *out, int out_length);

/**
 * Extract entropy bytes by applying SHA-512 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for generating high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out points to a byte array for storing the random bytes extracted
 * @param[in] out_length how many entropy bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_extract_sha512_entropy(alrng_context* ctxt, unsigned char *out, int out_length);

/**
 * Retrieve concatenated raw random bytes of both noise sources from the AlphaRNG device.
 * It is used to evaluate the quality of the raw random byte stream produced by both noise sources
 * before any post-processing or conditioning.
 * The byte stream can also be used as an input for alternative post-processing or conditioning algorithms
 * such as SHA and HMAC extractors.
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out points to a byte array for storing the random bytes retrieved
 * @param[in] out_length how many random bytes to retrieve
 *
 * @return 0 for successful operation
 */
int alrng_get_noise(alrng_context* ctxt, unsigned char *out, int out_length);

/**
 * This method is only used for testing the correctness of the data communication with the
 * SecireRNG device. Each byte retrieved, starting with 0, represents an incremented value of
 * the previous byte value.
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out points to a byte array for storing the test data
 * @param[in] out_length how many bytes of test data to receive
 *
 * @return 0 for successful operation
 *
 */
int alrng_get_test_data(alrng_context* ctxt, unsigned char *out, int out_length);

/**
 * Retrieve entropy bytes from the AlphaRNG device and store those into a file.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or can feed the entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out file_path_name pointer to file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_entropy_to_file(alrng_context* ctxt, const char *file_path_name, int64_t num_bytes);

/**
 * Extract entropy bytes into a file by applying SHA-256 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out file_path_name pointer to file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_extract_sha256_entropy_to_file(alrng_context* ctxt, const char *file_path_name, int64_t num_bytes);

/**
 * Extract entropy bytes into a file by applying SHA-512 method to RAW random bytes retrieved from an AlphaRNG device.
 * This is the method for retrieving high quality, non biased, random bytes that can be directly used in applications
 * or feed entropy inputs of DRBG(s).
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out file_path_name pointer to file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_extract_sha512_entropy_to_file(alrng_context* ctxt, const char *file_path_name, int64_t num_bytes);

/**
 * Retrieve digitized raw (unprocessed) random bytes from the first noise source of the AlphaRNG device
 * and store the bytes received into a file.
 * It is used to evaluate the quality of the first random noise source.
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[in] file_path_name file path name for storing the retrieved bytes
 * @param[in] num_bytes how many bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_noise_source_one_to_file(alrng_context* ctxt, const char *file_path_name, const int64_t num_bytes);

/**
 * Retrieve digitized raw (unprocessed) random bytes from the second noise source of the AlphaRNG device
 * and store the bytes received into a file.
 * It is used to evaluate the quality of the second random noise source.
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[in] file_path_name file path name for storing the retrieved bytes
 * @param[in] num_bytes how many bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_noise_source_two_to_file(alrng_context* ctxt, const char *file_path_name, const int64_t num_bytes);

/**
 * Retrieve raw (unprocessed) bytes from the AlphaRNG device and store those into a file.
 * It is used to evaluate the quality of the raw random byte stream produced by both noise sources
 * before any post-processing or conditioning.
 * The byte stream can also be used as an input for alternative post-processing or conditioning algorithms.
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] out file_path_name file path name for storing entropy bytes
 * @param[in] num_bytes how many entropy bytes to retrieve, 0 - for continuous operation
 *
 * @return 0 for successful operation
 */
int alrng_noise_to_file(alrng_context* ctxt, const char *file_path_name, int64_t num_bytes);

/**
 * Retrieve random values frequency distribution for both noise sources.
 * That information can be used for evaluating the quality of
 * noise sources.
 *
 * @param[in] ctxt pointer to context structure, must not be NULL
 * @param[out] freq_table_1 pointer to a memory allocated to store 256 uint16_t values for the first noise source
 * @param[out] freq_table_2 pointer to a memory allocated to store 256 uint16_t values for the second noise source
 *
 * @return 0 for successful operation
 */
int alrng_retrieve_frequency_tables(alrng_context* ctxt, uint16_t *freq_table_1, uint16_t *freq_table_2);


#ifdef __cplusplus
}
#endif
#endif
