/**
 *    @file sample.cpp
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief A sample program for connecting to an AlphaRNG device and retrieve 10 random bytes over secure channel
 */

#include <AlphaRngApi.h>

using namespace alpharng;

/**
 * @return 0 if ran successfully
 */
int main() {

	const int count = 10;
	unsigned char rnd_byte_buffer[count];
	AlphaRngApi rng; // Using default constructor to achieve maximum security data connectivity.

	cout << "----------------------------------------------------------------------------" << endl;
	cout << "--- Sample C program for retrieving random bytes from an AlphaRNG device ---" << endl;
	cout << "----------------------------------------------------------------------------" << endl;

	// Connecting to the first AlphaRNG device found
	if (!rng.connect(0)) {
		cerr << rng.get_last_error() << endl;
		return -1;
	}

	cout << endl << "AlphaRNG device open successfully" << endl << endl;

	cout << "*** Generating " << count << " random bytes ***" << endl;

	// Retrieve random bytes from device
	if (!rng.get_entropy(rnd_byte_buffer, sizeof(rnd_byte_buffer))) {
		cerr << rng.get_last_error() << endl;
		return -1;
	}

	for (int i = 0; i < count; i++) {
		cout << "random byte " <<  i << "-> " << (int)rnd_byte_buffer[i] << endl;
	}

	return 0;
}



