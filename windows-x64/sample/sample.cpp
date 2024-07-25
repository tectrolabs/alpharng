/**
 *    @file sample.cpp
 *    @date 1/9/2024
 *    @version 1.0
 *
 *    @brief A C++ example using C++ API for communicating with the AlphaRNG device.
 */

#include <AlphaRngApi.h>
#include <array>

using namespace std;
using namespace alpharng;

/**
 * @return 0 if ran successfully
 */
int main() {

    //A sample code that retrieves entropy bytes from an AlphaRNG device and store those into a file

    // Create a new AlphaRngApi instance using default constructor (HMAC-SHA256, RSA2048, and AES-256-GCM)
    AlphaRngApi rng { };

    // Connecting to the first AlphaRNG device found
    if (!rng.connect(0)) {
        cerr << rng.get_last_error() << endl;
        return -1;
    }

    cout << endl << "AlphaRNG device open successfully" << endl << endl;

    cout << "*** Retrieving 256 bytes of entropy from AlphaRNG device and storing to entropy_bytes.bin file ***"  << endl;

    if (!rng.entropy_to_file("entropy_bytes.bin", 256)) {
        cerr << rng.get_last_error() << endl;
        return -1;
    }

    return 0;
}
