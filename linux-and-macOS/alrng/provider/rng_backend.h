/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This file may only be used in conjunction with TectroLabs devices.
 */

/**
 *    @file rng_backend.h
 *
 *    @brief Where the AlphaRNG OpenSSL provider gets its random bytes from.
 *
 *    The provider logic does not know about the device. It asks a backend for bytes. The real backend
 *    (rng_backend_device.cpp) reads an AlphaRNG device. A stand-in backend (rng_backend_sim.cpp) produces a
 *    known byte stream, so the provider can be tested without hardware.
 */
#ifndef ALPHARNG_PROVIDER_RNG_BACKEND_H_
#define ALPHARNG_PROVIDER_RNG_BACKEND_H_

#include <cstddef>

#include "settings.h"

namespace alrng_provider {

class RngBackend {
public:
	virtual ~RngBackend() = default;

	// Prepare the source with the settings from openssl.cnf. Returns false if it cannot be used.
	virtual bool open(const Settings &settings) = 0;

	// Fill 'buffer' with exactly 'size' random bytes. Returns false on failure.
	virtual bool read(unsigned char *buffer, size_t size) = 0;
};

// Defined by exactly one backend source file. The caller owns the returned object.
RngBackend *create_backend();

} // namespace alrng_provider

#endif /* ALPHARNG_PROVIDER_RNG_BACKEND_H_ */
