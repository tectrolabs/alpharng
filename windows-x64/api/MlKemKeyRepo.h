/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for establishing a secure session between the host computer and the AlphaRNG PQ device.

  */

/**
 *    @file MlKemKeyRepo.h
 *    @date 11/03/2026
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Used for storing the hard-coded ML-KEM-1024 public key used for establishing a secure connection with the AlphaRNG PQ device.
 */

#ifndef ALPHARNG_API_INC_MLKEMKEYREPO_H_
#define ALPHARNG_API_INC_MLKEMKEYREPO_H_

namespace alpharng {

class MlKemKeyRepo {
public:
	const unsigned char *c_mlkem1024_pub_pem;
	const unsigned int c_mlkem1024_pub_pem_len;
	MlKemKeyRepo();
	MlKemKeyRepo(const MlKemKeyRepo &repo) = delete;
	MlKemKeyRepo & operator=(const MlKemKeyRepo &repo) = delete;
	virtual ~MlKemKeyRepo() {}
};

} /* namespace alpharng */

#endif /* ALPHARNG_API_INC_MLKEMKEYREPO_H_ */
