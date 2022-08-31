/**
 *    @file entropy-client-sample.cpp
 *    @date 08/28/2022
 *    @Author: Andrian Belinski
 *    @version 1.1
 *
 *    @brief A sample program for connecting to the entropy server and retrieving 10 random bytes 
 */

#include <EntropyServerConnector.h>
#include <iostream>

using namespace entropy::server::api;

/**
 * @return 0 if ran successfully
 */
int main() {

	const int count = 10;
	unsigned char rnd_byte_buffer[count];
	EntropyServerConnector pipe;

	cout << "------------------------------------------------------------------------------" << endl;
	cout << "--- Sample C++ program for retrieving random bytes from the entropy server ---" << endl;
	cout << "------------------------------------------------------------------------------" << endl;

	// Connecting to the entropy server
	if (!pipe.open_named_pipe()) {
		cerr << pipe.get_last_error() << endl;
		cerr << "Is entropy server running?" << endl;
		return -1;
	}

	cout << endl << "Pipe open successfully" << endl;


	// Retrieve server major version
	int server_major;
	if (!pipe.get_server_major_version(server_major)) {
		cout << "Expected a newer version of Entropy Server ... " << endl;
		return -1;
	}
	cout << "Server major version: " << server_major << endl;

	// Retrieve server minor version
	int server_minor;
	if (!pipe.get_server_minor_version(server_minor)) {
		cerr << pipe.get_last_error() << endl;
		cerr << "Could not retrieve server minor version" << endl;
		return -1;
	}
	cout << "Server minor version: " << server_minor << endl << endl;


	// Retrieve device serial number
	string identifier;
	if (!pipe.get_device_serial_number(identifier)) {
		cerr << pipe.get_last_error() << endl;
		cerr << "Could not retrieve device serial number" << endl;
		return -1;
	}
	cout << "Device identifier: " << identifier << endl;

	// Retrieve device model
	string model;
	if (!pipe.get_device_model(model)) {
		cerr << pipe.get_last_error() << endl;
		cerr << "Could not retrieve device model" << endl;
		return -1;
	}
	cout << "Device model: " << model << endl;

	// Retrieve device major version
	int major;
	if (!pipe.get_device_major_version(major)) {
		cerr << pipe.get_last_error() << endl;
		cerr << "Could not retrieve device major version" << endl;
		return -1;
	}
	cout << "Device major version: " << major << endl;

	// Retrieve device minor version
	int minor;
	if (!pipe.get_device_minor_version(minor)) {
		cerr << pipe.get_last_error() << endl;
		cerr << "Could not retrieve device minor version" << endl;
		return -1;
	}
	cout << "Device minor version: " << minor << endl << endl;

	cout << "*** Generating " << count << " random bytes ***" << endl;

	// Retrieve entropy bytes from device
	if (!pipe.get_entropy(rnd_byte_buffer, sizeof(rnd_byte_buffer))) {
		cerr << pipe.get_last_error() << endl;
		return -1;
	}

	cout << "entropy bytes: ";
	for (int i = 0; i < sizeof(rnd_byte_buffer); i++) {
		cout << (int)rnd_byte_buffer[i] << " ";
	}
	cout << endl;

	// Extract entropy bytes using SHA-256 extractor
	if (!pipe.extract_sha256_entropy(rnd_byte_buffer, sizeof(rnd_byte_buffer))) {
		cerr << pipe.get_last_error() << endl;
		return -1;
	}

	cout << "extracted entropy bytes using SHA-256: ";
	for (int i = 0; i < sizeof(rnd_byte_buffer); i++) {
		cout << (int)rnd_byte_buffer[i] << " ";
	}
	cout << endl;

	// Extract entropy bytes using SHA-512 extractor
	if (!pipe.extract_sha512_entropy(rnd_byte_buffer, sizeof(rnd_byte_buffer))) {
		cerr << pipe.get_last_error() << endl;
		return -1;
	}

	cout << "extracted entropy bytes using SHA-512: ";
	for (int i = 0; i < sizeof(rnd_byte_buffer); i++) {
		cout << (int)rnd_byte_buffer[i] << " ";
	}
	cout << endl;


	return 0;
}



