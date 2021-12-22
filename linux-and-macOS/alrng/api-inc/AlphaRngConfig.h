/**
 Copyright (C) 2014-2021 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This structure used by the API for interacting with the AlphaRNG device over a secure data communication channel.

 */

/**
 *    @file AlphaRngApi.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief AlphaRNG configuration structure used for establishing a session with the AlphaRNG device
 */

#ifndef API_INC_ALPHARNGCONFIG_H_
#define API_INC_ALPHARNGCONFIG_H_

#include <string>
#include <Structures.h>

namespace alpharng {

struct AlphaRngConfig {
	MacType e_mac_type;
	RsaKeySize e_rsa_key_size;
	KeySize e_aes_key_size;
	string pub_key_file_name;
};


} /* namespace alpharng */

#endif /* API_INC_ALPHARNGCONFIG_H_ */
