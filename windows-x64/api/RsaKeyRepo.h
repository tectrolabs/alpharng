/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for establishing a secure session between the host computer and the AlphaRNG device.

  */

/**
 *    @file RsaKeyRepo.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Used for storing hard-coded RSA 2048 and 1024 public keys used for establishing a secure connection with the AlphaRNG device.
 */

#ifndef API_INC_RSAKEYREPO_H_
#define API_INC_RSAKEYREPO_H_

namespace alpharng {

class RsaKeyRepo {
public:
	const unsigned char *c_rsapub_2048_pem;
	const unsigned char *c_rsapub_1024_pem;
	const unsigned int c_rsapub_2048_pem_len = 426;
	const unsigned int c_rsapub_1024_pem_len = 251;
	RsaKeyRepo();
	virtual ~RsaKeyRepo();
};

} /* namespace alpharng */

#endif /* API_INC_RSAKEYREPO_H_ */
