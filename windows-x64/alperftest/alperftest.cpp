/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program may only be used in conjunction with TectroLabs devices.

 This program is used for interacting with the hardware random data generator device AlphaRNG for the purpose of
 measuring performance in different transmission modes.

 */

/**
 *    @file alperftest.cpp
 *    @date 06/7/2021
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief A utility used for measuring performance of the AlphaRNG device in different transmission modes.
 */


#include <iostream>
#include <iomanip>
#include <AlphaRngApi.h>

using namespace std;
using namespace alpharng;


static bool display_device_info(AlphaRngApi &rng);
static bool run_device_perf_tests(int device_num);
static bool run_device_perf_test(int device_num, RngConfig &cfg);
static void reset_statistics(DeviceStatistics *ds);
static void generate_statistics(DeviceStatistics &ds, int64_t num_bytes);

/**
 * Application entry point.
 *
 * @return 0 when executed successfully
 */
int main() {

	AlphaRngApi rng_count;

	cout << "-------------------------------------------------------------------------------" << endl;
	cout << "-------- TectroLabs - alperftest - AlphaRNG performance test utility ----------" << endl;
	cout << "-------------------------------------------------------------------------------" << endl;
	cout << "Searching for devices ----------------------------- ";

	int count = rng_count.get_device_count();
	if (count > 0) {
		cout << "found " << std::setw(2) << count << " AlphaRNG device(s)" << endl;
	} else {
		cout << "  no AlphaRNG device found" << endl;
		return -1;
	}

	for (int i = 0; i < count; ++i) {
		AlphaRngApi rng;
		cout << endl;
		cout << "Opening device " << std::setw(2) << i << " ----------------------------------------------------- ";
		if (!rng.connect(i)) {
			cerr << "err: " << rng.get_last_error() << endl;
			return -1;
		}
		cout << "Success" << endl;

		if (!display_device_info(rng)) {
			return -1;
		}
		rng.disconnect();

		if (!run_device_perf_tests(i)) {
			return -1;
		}
		cout << endl;
	}

}


/**
 * Retrieve and display AlphaRNG device information.
 *
 * @param[in] rng RNG API instance
 *
 * @return true for successful operation
 */
static bool display_device_info(AlphaRngApi &rng) {

	string g_id;
	string g_model;
	unsigned char g_major_version;
	unsigned char g_minor_version;

	if (!rng.retrieve_device_id(g_id)) {
		cerr << "Could not retrieve device id" << endl;
		cerr << "err: " << rng.get_last_error() << endl;
		return false;
	}

	if (!rng.retrieve_device_model(g_model)) {
		cerr << "Could not retrieve device model" << endl;
		cerr << "err: " << rng.get_last_error() << endl;
		return false;
	}

	if (!rng.retrieve_device_major_version(&g_major_version)) {
		cerr << "Could not retrieve device major version" << endl;
		cerr << "err: " << rng.get_last_error() << endl;
		return false;
	}

	if (!rng.retrieve_device_minor_version(&g_minor_version)) {
		cerr << "Could not retrieve device minor version" << endl;
		cerr << "err: " << rng.get_last_error() << endl;
		return false;
	}

	cout << "Measuring performance for ";
	cout << "'" << g_model << "', S/N: " << g_id << ", version: " << (int)g_major_version << "." << (int)g_minor_version << endl;


	return true;
}

/**
 * Run performance tests for the device in different modes.
 *
 * @param[in] device_num device number
 *
 * @return true for successful operation
 */
static bool run_device_perf_tests(int device_num) {

	{
		RngConfig cfg {MacType::None, KeySize::None, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::hmacMD5, KeySize::None, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::hmacSha160, KeySize::None, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::hmacSha256, KeySize::None, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::None, KeySize::k128, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::hmacMD5, KeySize::k128, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::hmacSha160, KeySize::k128, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::hmacSha256, KeySize::k128, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::None, KeySize::k256, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::hmacMD5, KeySize::k256, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::hmacSha160, KeySize::k256, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}

	{
		RngConfig cfg {MacType::hmacSha256, KeySize::k256, "", RsaKeySize::rsa2048};
		if (!run_device_perf_test(device_num, cfg)) {
			return false;
		}
	}


	return true;
}

/**
 * Run a performance test for specific configuration and device number.
 *
 * @param[in] device_num device number
 * @param[in] cfg RngConfig reference
 *
 * @return true for successful operation
 */
static bool run_device_perf_test(int device_num, RngConfig &cfg) {
	DeviceStatistics ds;
	AlphaRngApi rng(AlphaRngConfig {cfg.e_mac_type, cfg.e_rsa_key_size, cfg.e_aes_key_size, cfg.key_file});
	bool slow_dwld = false;
	unsigned char rnd_buffer[100000];


	cout << "MAC: ";
	switch(cfg.e_mac_type) {
	case MacType::hmacMD5:
		cout << "HMAC-MD5   ";
		slow_dwld = true;
		break;
	case MacType::hmacSha160:
		cout << "HMAC-SHA160";
		slow_dwld = true;
		break;
	case MacType::hmacSha256:
		cout << "HMAC-SHA256";
		slow_dwld = true;
		break;
	case MacType::None:
		cout << "None       ";
		break;
	default:
		cerr << "Could not set MAC" << endl;
		return false;
	}
	cout << ", cipher: ";

	switch(cfg.e_aes_key_size) {
	case KeySize::k128:
		cout << "AES-128-GCM";
		slow_dwld = true;
		break;
	case KeySize::k256:
		cout << "AES-256-GCM";
		slow_dwld = true;
		break;
	case KeySize::None:
		cout << "None       ";
		break;
	default:
		cerr << "Could not set cipher" << endl;
		return false;
	}

	cout << ", session pk: RSA-2048";
	cout << " ...... ";
	std::cout.flush();


	if (!rng.connect(device_num)) {
		cerr << "Could not reach device: " << rng.get_last_error() << endl;
		return false;
	}

	int dwld_blocks_count = slow_dwld ? 70 : 1000;

	reset_statistics(&ds);
	for (int i = 0; i < dwld_blocks_count; ++i) {
		if (!rng.get_entropy(rnd_buffer, sizeof (rnd_buffer))) {
			cerr << "Error when retrieving entropy bytes: " << rng.get_last_error() << endl;
			return false;
		}
	}
	generate_statistics(ds, dwld_blocks_count * sizeof (rnd_buffer));

	cout << std::setw(5) << ds.download_speed_kbsec << " KB/sec" << endl;

	return true;
}

/**
 * Reset statistics and start the counter.
 *
 * @param[in] ds pointer to DeviceStatistics structure
 *
 */
static void reset_statistics(DeviceStatistics *ds) {
	memset(ds, 0x0, sizeof(DeviceStatistics));
	time(&ds->begin_time);
}

/**
 * Generate performance statistics.
 *
 * @param[out] ds device statistics
 * @param[in] num_bytes number of bytes
 *
 *
 */
static void generate_statistics(DeviceStatistics &ds, int64_t num_bytes) {
	time(&ds.end_time);
	ds.total_time = ds.end_time - ds.begin_time;
	if (ds.total_time == 0) {
		ds.total_time = 1;
	}
	ds.download_speed_kbsec = (int) (num_bytes / (int64_t) 1024 / (int64_t)ds.total_time);
}


