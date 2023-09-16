/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for establishing a secure session between the host computer and the AlphaRNG device.

  */

/**
 *    @file RsaKeyRepo.h
 *    @date 09/16/2023
 *    @Author: Andrian Belinski
 *    @version 1.5
 *
 *    @brief Used for storing hard-coded RSA 2048 and 1024 public keys used for establishing a secure connection with the AlphaRNG device.
 */

#ifndef ALPHARNG_API_INC_RSAKEYREPO_H_
#define ALPHARNG_API_INC_RSAKEYREPO_H_

#include <new>

namespace alpharng {

class RsaKeyRepo {
public:
	const unsigned int c_rsapub_2048_pem_len = {426};
	const unsigned int c_rsapub_1024_pem_len = {251};
	const unsigned char *c_rsapub_2048_pem;
	const unsigned char *c_rsapub_1024_pem;
	RsaKeyRepo();
	RsaKeyRepo(const RsaKeyRepo &repo) = delete;
	RsaKeyRepo & operator=(const RsaKeyRepo &repo) = delete;
	virtual ~RsaKeyRepo();
};

} /* namespace alpharng */

#endif /* ALPHARNG_API_INC_RSAKEYREPO_H_ */
