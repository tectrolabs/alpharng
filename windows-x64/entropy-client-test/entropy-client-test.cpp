/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program may only be used in conjunction with TectroLabs devices.

 This program is for testing connectivity to the entropy server and for measuring performance.

 */

 /**
  *    @file entropy-client-test.cpp
  *    @date 03/06/2021
  *    @Author: Andrian Belinski
  *    @version 1.0
  *
  *    @brief A test program for checking the connectivity to the entropy server and for measuring data transfer performance.
  */

#include <EntropyServerConnector.h>
#include <iostream>
#include <iomanip>
#include <time.h>
#include <thread>

using namespace alpharng;

static const int entropy_buffer_size = 100000;
static const int num_pipe_test_threads = 50;
static const int num_test_blocks = 50;
static const int num_test_blocks_high = 1000;
static unsigned char entropy_buffer[entropy_buffer_size];
static atomic<int> test_data_thread_errors = 0;

static bool compute_download_speed(EntropyServerConnector& pipe, int block_count, double& download_speed_mbps);

/*
* Thread testing class
*/
class EntropyServerConnectorTestThread {
public:
	EntropyServerConnectorTestThread() {}
	virtual ~EntropyServerConnectorTestThread() {}
	void operator()() {
		unsigned char byte_buffer[entropy_buffer_size];
		EntropyServerConnector pipe;
		bool status = pipe.open_named_pipe();
		if (status == false) {
			test_data_thread_errors++;
			return;
		}
		status = pipe.get_test_bytes(byte_buffer, entropy_buffer_size);
		if (status == false) {
			test_data_thread_errors++;
			return;
		}
		unsigned char value = 0;
		for (auto i = 0; i < entropy_buffer_size; ++i) {
			if (value++ != byte_buffer[i]) {
				test_data_thread_errors++;
				return;
			}
		}
	}
};


/**
* Main entry
*
* @param int argc - number of parameters
* @param char ** argv - parameters
*
*/
int main(int argc, char** argv) {
	bool status = false;
	EntropyServerConnector pipe;


	cout << "-------- Testing connectivity to the entropy server using named pipes ------" << endl;

	//
	// Step 1
	//
	cout << "Connecting to the entropy server pipe .............................. ";
	status = pipe.open_named_pipe();
	if (status == true) {
		cout << "SUCCESS" << endl;
	}
	else {
		cout << " failed" << endl;
		cout << "is entropy server running?" << endl;
		return -1;
	}

	//
	// Step 2
	//
	cout << "Retrieving 100000 bytes from the entropy server .................... ";
	status = pipe.get_entropy(entropy_buffer, entropy_buffer_size);
	if (status == true) {
		cout << "SUCCESS" << endl;
	}
	else {
		cout << " failed" << endl;
		return -1;
	}

	//
	// Step 3
	//
	cout << "Running pipe communication diagnostics ............................. ";
	status = pipe.get_test_bytes(entropy_buffer, entropy_buffer_size);
	if (status == true) {
		unsigned char testCounter = 0;
		for (auto t = 0; t < entropy_buffer_size; ++t) {
			if (entropy_buffer[t] != testCounter) {
				return -1;
			}
			testCounter++;
		}
		cout << "SUCCESS" << endl;
	}
	else {
		cout << " failed" << endl;
		return -1;
	}

	//
	// Step 3
	//
	cout << "Running pipe communication diagnostics using " << setw(2) << num_pipe_test_threads << " threads ............";
	thread pipe_threads[num_pipe_test_threads];
	for (auto i = 0; i < num_pipe_test_threads; i++) {
		pipe_threads[i] = thread{ EntropyServerConnectorTestThread{} };
	}

	for (auto i = 0; i < num_pipe_test_threads; i++) {
		pipe_threads[i].join();
	}

	if (test_data_thread_errors == 0) {
		cout << " SUCCESS" << endl;
	}
	else {
		cout << " FAILED" << endl;
	}

	//
	// Step 4.1
	//
	cout << "Calculating minimum entropy download speed ......................";
	double download_speed_mbps;
	if (compute_download_speed(pipe, num_test_blocks, download_speed_mbps) == false) {
		return -1;
	}
	if (download_speed_mbps >= 30) {
		cout << "Calculating maximum entropy download speed ......................";
		if (compute_download_speed(pipe, num_test_blocks_high, download_speed_mbps) == false) {
			return -1;
		}
	}
	return 0;
}

/*
* Compoute and display download performance
* 
* @return true if successful
*/
bool compute_download_speed(EntropyServerConnector &pipe, int block_count, double &download_speed_mbps) {
	time_t start = time(NULL);
	bool status = false;
	for (auto l = 0; l < block_count; ++l) {
		status = pipe.get_entropy(entropy_buffer, entropy_buffer_size);
		if (status == false) {
			cout << " failed" << endl;
			return false;
		}
	}
	time_t end_time = time(NULL);

	int64_t totalTime = end_time - start;
	if (totalTime == 0) {
		totalTime = 1;
	}
	download_speed_mbps = (entropy_buffer_size * block_count) / totalTime / 1000.0 / 1000.0 * 8.0;
	cout << setw(6) << setprecision(3) << download_speed_mbps << " Mbps" << endl;
	return true;
}