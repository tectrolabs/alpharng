/**
 *    @file entropy-client-sample.cpp
 *    @date 03/12/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief A sample program for connecting to the entropy server and retrieving 10 random bytes 
 */

#include <EntropyServerConnector.h>
#include <iostream>

using namespace alpharng;

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

	// Connecting to the first AlphaRNG device found
	if (!pipe.open_named_pipe()) {
		cerr << pipe.get_last_error() << endl;
		cerr << "Is entropy server running?" << endl;
		return -1;
	}

	cout << endl << "Pipe open successfully" << endl << endl;

	cout << "*** Generating " << count << " random bytes ***" << endl;

	// Retrieve random bytes from device
	if (!pipe.get_entropy(rnd_byte_buffer, sizeof(rnd_byte_buffer))) {
		cerr << pipe.get_last_error() << endl;
		return -1;
	}

	for (int i = 0; i < count; i++) {
		cout << "random byte " << i << "-> " << (int)rnd_byte_buffer[i] << endl;
	}

	return 0;
}



