/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.

 This file defines data structures used in AlphaRNG API implementation.

 */

/**
 *    @file Structures.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Data structures used in the API implementation.
 */

#ifndef API_INC_STRUCTURES_H_
#define API_INC_STRUCTURES_H_

#include <cstdint>

namespace alpharng {

enum class CommandType : uint16_t {
	getDeviceHealthStatus = 300,
	getDeviceInfo = 301,
	healthTest = 302,
	getFrequencyTables = 303,
	getNoiseSourceOne = 304,
	getNoiseSourceTwo = 305,
	getEntropy = 306,
	getTestData = 307,
	getNoise = 308
};
enum class KeySize : uint8_t {None = 0, k128 = 16, k256 = 32};
enum class MacType : uint8_t {None = 0, hmacMD5 =16, hmacSha160 = 20, hmacSha256 = 32};
enum class PacketType : uint8_t {pkRSA2048 = 1,	pkAltRSA2048 = 2, pkRSA1024 = 20,	aes = 40};
enum class SessionKeyType : uint8_t {aes = 1};
enum class RsaKeySize : uint16_t {rsa2048 = 256, rsa1024 = 128};

#pragma pack (1)
struct DeviceInfo {
	uint8_t major_version;
	uint8_t minor_version;
	uint8_t identifier[15];
	uint8_t model[15];
};
struct Command {
	MacType 	e_mac_type;
	uint8_t 	mac[32];
	CommandType e_type;
	uint64_t	token;
	uint16_t	payload_size;
	uint8_t		payload[256];
};
struct Packet {
	PacketType	e_type;
	KeySize		e_key_size;
	uint8_t		cipher_iv[12];
	uint8_t		cipher_tag[16];
	uint16_t 	payload_size;
	uint8_t 	payload[16096];
};
struct Response {
	MacType 	e_mac_type;
	uint8_t 	mac[32];
	uint64_t	token;
	uint16_t	payload_size;
	uint8_t		payload[16096];
};
struct Session {
	SessionKeyType 	e_type;
	KeySize 		e_size;
	uint8_t 		key[32];
	uint64_t		token;
	uint8_t			cipher_aad[16];
	MacType 		e_mac_type;
	uint8_t			mac_key[32];
	uint8_t 		mac[32];
};
#pragma pack ()

struct FrequencyTables {
	uint16_t freq_table_1[256];
	uint16_t freq_table_2[256];
};

// Repetition Count Test data
struct RctData {
	uint32_t max_repetitions;
	uint32_t cur_repetitions;
	uint8_t last_sample;
	uint8_t status_byte;
	uint8_t signature;
	bool is_initialized;
	uint16_t failure_count;
};

// Adaptive Proportion Test data
struct AptData {
	uint16_t window_size;
	uint16_t cutoff_value;
	uint16_t cur_repetitions;
	uint16_t cur_samples;
	uint8_t status_byte;
	uint8_t signature;
	bool is_initialized;
	uint8_t first_sample;
	uint16_t cycle_failures;
};

} /* namespace alpharng */

#endif /* API_INC_STRUCTURES_H_ */
