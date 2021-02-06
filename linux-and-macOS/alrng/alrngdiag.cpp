/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program may only be used in conjunction with TectroLabs devices.

 This program is used for running diagnostics for one or more AlphaRNG devices.


 */

/**
 *    @file alrngdiag.cpp
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief A utility used for running the AlphaRNG device diagnostics
 *
 */

#include <cstdint>
#include <iostream>
#include <iomanip>

#include <AlphaRngApi.h>

using namespace alpharng;
using namespace std;

static bool display_device_info(AlphaRngApi &rng);
static bool run_device_diagnostics(AlphaRngApi &rng);
static bool display_frequency_table_summary(uint16_t frequency_table[]);
static bool inspect_raw_data(unsigned char raw_data_1[], unsigned char raw_data_2[]);
static bool retrieve_entropy_bytes(AlphaRngApi &rng);
static bool retrieve_noise_bytes(AlphaRngApi &rng);
static bool retrieve_test_data(AlphaRngApi &rng);

int main() {

	AlphaRngApi rng;
	FrequencyTables freq_tables;

	cout << "-------------------------------------------------------------------" << endl;
	cout << "------ TectroLabs - alrngdiag - AlphaRNG diagnostics utility ------" << endl;
	cout << "-------------------------------------------------------------------" << endl;
	cout << "Searching for devices -----------------";

	int count = rng.get_device_count();
	if (count > 0) {
		cout << "found " << std::setw(2) << count << " AlphaRNG device(s)" << endl;
	} else {
		cout << "  no AlphaRNG device found" << endl;
		return -1;
	}

	for (int i = 0; i < count; ++i) {

		cout << endl;
		cout << "Opening device -------------------------------------------- ";
		if (!rng.connect(i)) {
			cerr << "err: " << rng.get_last_error() << endl;
			return -1;
		}
		cout << "Success" << endl;


		cout << "Retrieving RNG status ----------------------------------- ";
		unsigned char rng_status;
		if (!rng.retrieve_rng_status(&rng_status)) {
			cerr << "err: " << rng.get_last_error() << endl;
			return -1;
		}
		if (rng_status != 0) {
			cout << (int)rng_status << endl;
			return -1;
		} else {
			cout << "(healthy)" << endl;
		}

		if (!display_device_info(rng)) {
			return -1;
		}

		if (!run_device_diagnostics(rng)) {
			return -1;
		}

		if (!retrieve_entropy_bytes(rng)) {
			return -1;
		}

		if (!retrieve_noise_bytes(rng)) {
			return -1;
		}

		if (!retrieve_test_data(rng)) {
			return -1;
		}

		cout << endl;
		cout << "----------  Inspecting RAW data of the noise sources  -------------" << endl;
		unsigned char noise_source_1[16000];
		if(!rng.get_noise_source_1((unsigned char *)noise_source_1, sizeof(noise_source_1))) {
			cerr << "err: " << rng.get_last_error() << endl;
			return -1;
		}
		unsigned char noise_source_2[16000];
		if(!rng.get_noise_source_2((unsigned char *)noise_source_2, sizeof(noise_source_2))) {
			cerr << "err: " << rng.get_last_error() << endl;
			return -1;
		}

		if (!inspect_raw_data(noise_source_1, noise_source_2)) {
			return -1;
		}


		if (!rng.retrieve_frequency_tables(freq_tables)) {
			cerr << "err: " << rng.get_last_error() << endl;
		}
		cout << endl << "-------- Retrieving frequency table for noise source 1 ------------" << endl;
		if (!display_frequency_table_summary(freq_tables.freq_table_1)) {
			return -1;
		}

		cout << endl << "-------- Retrieving frequency table for noise source 2 ------------" << endl;
		if (!display_frequency_table_summary(freq_tables.freq_table_2)) {
			return -1;
		}

		rng.disconnect();

		cout << "-------------------------------------------------------------------" << endl;
		cout << "----------------- All tests passed successfully -------------------" << endl;

	}
}

/**
 * Calculate the min and max values of the frequency table values. Displays the frequency values.
 *
 * @param[in] frequency_table frequency values
 *
 * @return true for successful operation
 */
static bool display_frequency_table_summary(uint16_t frequency_table[]) {
	int min_freq;
	int max_freq;
	int total_samples;
	int freq_range;

	min_freq = frequency_table[0];
	max_freq = frequency_table[0];
	total_samples = 0;

	for (int k = 0; k < 256; k += 8) {
		cout << "(" << std::setw(3) << k << " : " << std::setw(3) << k+7 << ") " <<
		std::setw(3) << frequency_table[k+0] << ", " <<
		std::setw(3) << frequency_table[k+1] << ", " <<
		std::setw(3) << frequency_table[k+2] << ", " <<
		std::setw(3) << frequency_table[k+3] << ", " <<
		std::setw(3) << frequency_table[k+4] << ", " <<
		std::setw(3) << frequency_table[k+5] << ", " <<
		std::setw(3) << frequency_table[k+6] << ", " <<
		std::setw(3) << frequency_table[k+7] << endl;
	}

	for (int k = 0; k < 256; k++) {
		total_samples += frequency_table[k];
		if (frequency_table[k] > max_freq) {
			max_freq = frequency_table[k];
		}
		if (frequency_table[k] < min_freq) {
			min_freq = frequency_table[k];
		}
	}
	cout << "-------------------------------------------------------------------" << endl;
	cout << "Table summary: min " << min_freq << ", max " << max_freq << ", total samples " << total_samples;

	freq_range = max_freq - min_freq;
	if (freq_range > 200.0 || total_samples != 16000) {
		cout << " *FAILED*" << endl;
		return false;
	} else if (freq_range > 100.0) {
		cout << " *WARNING*" << endl;
	} else {
		cout << " (healthy)" << endl;
	}
	return true;
}

/**inspect_raw_data
 * Retrieve and display AlphaRNG device information
 *
 * @param[in] RNG api instance
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

	cout << "Testing ";
	cout << "'" << g_model << "', S/N: " << g_id << ", version: " << (int)g_major_version << "." << (int)g_minor_version << endl;

	return true;
}

/**
 * Generate frequency distribution tables for both noise sources and display counted values.
 *
 * @param[in] raw_data_1 raw (unprocessed) data from noise source one
 * @param[in] raw_data_3 raw (unprocessed) data from noise source two
 *
 * @return true for successful operation
 */
static bool inspect_raw_data(unsigned char raw_data_1[], unsigned char raw_data_2[]) {
	int count1, count2;
	int minFreq1, minFreq2, maxFreq1, maxFreq2;
	uint16_t frequencyTableSource1[256];
	uint16_t frequencyTableSource2[256];
	int freqRange1;
	int freqRange2;

	// Clear the frequency tables
	for (int i = 0; i < 256; ++i) {
		frequencyTableSource1[i] = 0;
		frequencyTableSource2[i] = 0;
	}

	// Fill the frequency tables
	for (int i = 0; i < 16000; ++i) {
		frequencyTableSource1[(uint8_t)raw_data_1[i]]++;
		frequencyTableSource2[(uint8_t)raw_data_2[i]]++;
	}

	// Calculate min and max values for each table
	minFreq1 = frequencyTableSource1[0];
	maxFreq1 = frequencyTableSource1[0];
	minFreq2 = frequencyTableSource2[0];
	maxFreq2 = frequencyTableSource2[0];
	count1 = 0;
	count2 = 0;
	for (int i = 0; i < 256; ++i) {
		count1 += frequencyTableSource1[i];
		count2 += frequencyTableSource2[i];
		if (frequencyTableSource1[i] > maxFreq1) {
			maxFreq1 = frequencyTableSource1[i];
		}
		if (frequencyTableSource1[i] < minFreq1) {
			minFreq1 = frequencyTableSource1[i];
		}
		if (frequencyTableSource2[i] > maxFreq2) {
			maxFreq2 = frequencyTableSource2[i];
		}
		if (frequencyTableSource2[i] < minFreq2) {
			minFreq2 = frequencyTableSource2[i];
		}
	}

	cout << "Frequency table source 1: min " << minFreq1 << ", max " << maxFreq1 << ", samples " << count1;
	freqRange1 = maxFreq1 - minFreq1;
	if (freqRange1 > 200.0 || count1 != 16000) {
		cout << " *FAILED*" << endl;
		return false;
	} else if (freqRange1 > 100.0) {
		cout << " *WARNING*" << endl;
	} else {
		cout << " (healthy)" << endl;
	}

	cout << "Frequency table source 2: min " << minFreq2 << ", max " << maxFreq2 << ", samples " << count2;
	freqRange2 = maxFreq2 - minFreq2;
	if (freqRange2 > 200.0 || count2 != 16000) {
		cout << " *FAILED*" << endl;
		return false;
	} else if (freqRange2 > 100.0) {
		cout << " *WARNING*" << endl;
	} else {
		cout << " (healthy)" << endl;
	}

	return true;
}

/**
 * Invoke device built-in diagnostics routines.
 *
 * @param[in] RNG api instance
 *
 * @return true for successful operation
 */
static bool run_device_diagnostics(AlphaRngApi &rng) {
	cout << "---------- Running device internal diagnostics  ----------  ";
	cout.flush();
	for (int i = 0; i < 10; ++i) {
		if (!rng.run_health_test()) {
			cerr << "err: " << rng.get_last_error() << endl;
			return false;
		}
	}
	cout << "Success" << endl;
	return true;
}

/**
 *  Retrieve entropy bytes. It will also run APT and RCT health tests.
 *
 *  @return true for successful operation
 *
 */
static bool retrieve_entropy_bytes(AlphaRngApi &rng) {
	cout << "--------------- Retrieving Entropy Bytes  ----------------  ";
	cout.flush();
	for (int i = 0; i < 20; ++i) {
		uint8_t entropy_buffer[100000];
		if (!rng.get_entropy(entropy_buffer, sizeof(entropy_buffer))) {
			cerr << "err: " << rng.get_last_error() << endl;
			return false;
		}
	}
	cout << "Success" << endl;
	return true;
}

/**
 *  Retrieve bytes from the noise sources. It will also run APT and RCT health tests.
 *
 *  @return true for successful operation
 *
 */
static bool retrieve_noise_bytes(AlphaRngApi &rng) {
	uint8_t noise_surce_buffer[100000];
	cout << "--------- Retrieving bytes from noise source 1  ----------  ";
	cout.flush();
	for (int i = 0; i < 10; ++i) {
		if (!rng.get_noise_source_1(noise_surce_buffer, sizeof(noise_surce_buffer))) {
			cerr << "err: " << rng.get_last_error() << endl;
			return false;
		}
	}
	cout << "Success" << endl;

	cout << "--------- Retrieving bytes from noise source 2  ----------  ";
	cout.flush();
	for (int i = 0; i < 10; ++i) {
		if (!rng.get_noise_source_2(noise_surce_buffer, sizeof(noise_surce_buffer))) {
			cerr << "err: " << rng.get_last_error() << endl;
			return false;
		}
	}
	cout << "Success" << endl;

	cout << "----- Retrieving combined bytes from noise sources -------  ";
	cout.flush();
	for (int i = 0; i < 10; ++i) {
		if (!rng.get_noise(noise_surce_buffer, sizeof(noise_surce_buffer))) {
			cerr << "err: " << rng.get_last_error() << endl;
			return false;
		}
	}

	cout << "Success" << endl;
	return true;
}

/**
 * This function is used for testing the correctness of the data communication with the
 * SecireRNG device. Each byte retrieved, starting with 0, represents an incremented value of
 * the previous byte value.
 *
 * @return true for successful operation
 *
 */
static bool retrieve_test_data(AlphaRngApi &rng) {
	cout << "------------------ Retrieving Test Data ------------------  ";
	cout.flush();
	for (int i = 0; i < 5; ++i) {
		uint8_t test_data[256*10];
		if (!rng.get_test_data(test_data, sizeof(test_data))) {
			cerr << "err: " << rng.get_last_error() << endl;
			return false;
		}
		for (int j = 0; j < (int)sizeof(test_data); ++j) {
			if (test_data[j] != (uint8_t)j) {
				cerr << "get_test_data() failed, unexpected byte: " << (int)test_data[j] << endl;
				return false;
			}
		}
	}
	cout << "Success" << endl;
	return true;
}
