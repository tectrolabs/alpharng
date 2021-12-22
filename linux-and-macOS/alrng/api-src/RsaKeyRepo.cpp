/**
 Copyright (C) 2014-2021 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for establishing a secure session between the host computer and the AlphaRNG device.

*/

/**
 *    @file RsaKeyRepo.cpp
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Used for storing hard-coded RSA 2048 and 1024 public keys used for establishing a secure connection with the AlphaRNG device.
 */

#include "pch.h"
#include <RsaKeyRepo.h>

namespace alpharng {

RsaKeyRepo::RsaKeyRepo() {

	c_rsapub_2048_pem = new unsigned char[c_rsapub_2048_pem_len] {
		  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x52,
		  0x53, 0x41, 0x20, 0x50, 0x55, 0x42, 0x4c, 0x49, 0x43, 0x20, 0x4b, 0x45,
		  0x59, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a, 0x4d, 0x49, 0x49, 0x42, 0x43,
		  0x67, 0x4b, 0x43, 0x41, 0x51, 0x45, 0x41, 0x77, 0x57, 0x47, 0x66, 0x45,
		  0x59, 0x6d, 0x76, 0x68, 0x4c, 0x51, 0x66, 0x77, 0x5a, 0x4f, 0x66, 0x61,
		  0x6e, 0x77, 0x52, 0x63, 0x49, 0x74, 0x53, 0x6f, 0x73, 0x58, 0x79, 0x6e,
		  0x59, 0x42, 0x64, 0x42, 0x79, 0x39, 0x4e, 0x51, 0x58, 0x77, 0x6a, 0x30,
		  0x78, 0x77, 0x77, 0x32, 0x62, 0x4a, 0x77, 0x51, 0x74, 0x41, 0x76, 0x0a,
		  0x43, 0x6e, 0x54, 0x30, 0x6e, 0x4f, 0x54, 0x33, 0x64, 0x32, 0x57, 0x4d,
		  0x36, 0x39, 0x41, 0x70, 0x75, 0x35, 0x4f, 0x4b, 0x2f, 0x32, 0x75, 0x43,
		  0x30, 0x79, 0x30, 0x6a, 0x63, 0x38, 0x59, 0x66, 0x51, 0x50, 0x6e, 0x66,
		  0x55, 0x59, 0x73, 0x38, 0x6e, 0x4d, 0x2f, 0x78, 0x61, 0x58, 0x79, 0x77,
		  0x44, 0x4f, 0x72, 0x64, 0x45, 0x6b, 0x42, 0x53, 0x47, 0x4c, 0x39, 0x59,
		  0x4e, 0x6d, 0x7a, 0x67, 0x0a, 0x43, 0x77, 0x32, 0x4e, 0x62, 0x6f, 0x31,
		  0x75, 0x36, 0x2f, 0x43, 0x44, 0x63, 0x48, 0x33, 0x55, 0x74, 0x4c, 0x38,
		  0x69, 0x6f, 0x62, 0x68, 0x53, 0x66, 0x31, 0x73, 0x68, 0x4e, 0x64, 0x68,
		  0x6c, 0x77, 0x58, 0x4d, 0x41, 0x6c, 0x30, 0x4f, 0x48, 0x61, 0x4f, 0x4b,
		  0x59, 0x72, 0x6f, 0x6a, 0x51, 0x51, 0x44, 0x46, 0x2f, 0x45, 0x6c, 0x77,
		  0x2f, 0x55, 0x4e, 0x52, 0x59, 0x4d, 0x7a, 0x57, 0x53, 0x0a, 0x45, 0x4a,
		  0x62, 0x2f, 0x72, 0x6a, 0x59, 0x49, 0x6f, 0x53, 0x32, 0x6f, 0x59, 0x44,
		  0x72, 0x56, 0x55, 0x35, 0x6d, 0x55, 0x56, 0x64, 0x78, 0x72, 0x33, 0x6e,
		  0x59, 0x4e, 0x70, 0x4e, 0x44, 0x69, 0x2b, 0x62, 0x67, 0x67, 0x4a, 0x59,
		  0x61, 0x76, 0x66, 0x6d, 0x65, 0x76, 0x2f, 0x33, 0x4f, 0x34, 0x70, 0x71,
		  0x79, 0x78, 0x77, 0x36, 0x6b, 0x32, 0x4c, 0x4d, 0x4e, 0x5a, 0x61, 0x34,
		  0x77, 0x41, 0x0a, 0x7a, 0x54, 0x69, 0x6b, 0x4b, 0x72, 0x42, 0x38, 0x63,
		  0x38, 0x6f, 0x76, 0x75, 0x70, 0x39, 0x4f, 0x35, 0x4f, 0x44, 0x75, 0x70,
		  0x61, 0x73, 0x66, 0x6f, 0x42, 0x43, 0x5a, 0x45, 0x48, 0x6a, 0x74, 0x35,
		  0x31, 0x77, 0x75, 0x65, 0x74, 0x72, 0x76, 0x58, 0x6d, 0x36, 0x51, 0x58,
		  0x4c, 0x73, 0x48, 0x35, 0x74, 0x65, 0x6e, 0x77, 0x4a, 0x37, 0x51, 0x7a,
		  0x77, 0x4a, 0x5a, 0x6a, 0x4f, 0x42, 0x58, 0x0a, 0x51, 0x59, 0x6c, 0x5a,
		  0x6e, 0x53, 0x36, 0x48, 0x76, 0x57, 0x2f, 0x64, 0x37, 0x50, 0x4c, 0x57,
		  0x4e, 0x79, 0x66, 0x44, 0x32, 0x30, 0x57, 0x5a, 0x67, 0x32, 0x76, 0x61,
		  0x68, 0x63, 0x58, 0x70, 0x73, 0x77, 0x49, 0x44, 0x41, 0x51, 0x41, 0x42,
		  0x0a, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x45, 0x4e, 0x44, 0x20, 0x52, 0x53,
		  0x41, 0x20, 0x50, 0x55, 0x42, 0x4c, 0x49, 0x43, 0x20, 0x4b, 0x45, 0x59,
		  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a
	};

	c_rsapub_1024_pem = new unsigned char [c_rsapub_1024_pem_len] {
		  0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x42, 0x45, 0x47, 0x49, 0x4e, 0x20, 0x52,
		  0x53, 0x41, 0x20, 0x50, 0x55, 0x42, 0x4c, 0x49, 0x43, 0x20, 0x4b, 0x45,
		  0x59, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a, 0x4d, 0x49, 0x47, 0x4a, 0x41,
		  0x6f, 0x47, 0x42, 0x41, 0x4e, 0x6d, 0x53, 0x33, 0x64, 0x61, 0x74, 0x31,
		  0x4d, 0x63, 0x33, 0x36, 0x4e, 0x33, 0x71, 0x73, 0x74, 0x33, 0x4a, 0x67,
		  0x6f, 0x37, 0x45, 0x41, 0x69, 0x61, 0x7a, 0x65, 0x64, 0x39, 0x77, 0x35,
		  0x4c, 0x50, 0x53, 0x43, 0x56, 0x71, 0x44, 0x63, 0x38, 0x48, 0x54, 0x58,
		  0x6b, 0x69, 0x4b, 0x39, 0x45, 0x47, 0x73, 0x63, 0x53, 0x78, 0x4f, 0x0a,
		  0x34, 0x65, 0x32, 0x62, 0x75, 0x77, 0x32, 0x73, 0x34, 0x69, 0x6b, 0x31,
		  0x5a, 0x71, 0x47, 0x43, 0x76, 0x79, 0x6a, 0x59, 0x70, 0x54, 0x57, 0x71,
		  0x2f, 0x79, 0x35, 0x58, 0x5a, 0x41, 0x6b, 0x61, 0x38, 0x34, 0x36, 0x6f,
		  0x4e, 0x34, 0x44, 0x53, 0x41, 0x71, 0x38, 0x6e, 0x70, 0x50, 0x72, 0x43,
		  0x71, 0x55, 0x6a, 0x2b, 0x43, 0x57, 0x7a, 0x47, 0x31, 0x59, 0x39, 0x33,
		  0x68, 0x37, 0x54, 0x57, 0x0a, 0x76, 0x70, 0x2f, 0x51, 0x45, 0x4a, 0x70,
		  0x6f, 0x79, 0x45, 0x6b, 0x6a, 0x7a, 0x6e, 0x68, 0x69, 0x66, 0x72, 0x53,
		  0x33, 0x78, 0x69, 0x32, 0x30, 0x2b, 0x73, 0x31, 0x49, 0x4c, 0x39, 0x61,
		  0x38, 0x35, 0x42, 0x6e, 0x36, 0x41, 0x62, 0x54, 0x39, 0x63, 0x37, 0x43,
		  0x75, 0x46, 0x46, 0x6e, 0x6f, 0x6b, 0x58, 0x5a, 0x2f, 0x41, 0x67, 0x4d,
		  0x42, 0x41, 0x41, 0x45, 0x3d, 0x0a, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x45,
		  0x4e, 0x44, 0x20, 0x52, 0x53, 0x41, 0x20, 0x50, 0x55, 0x42, 0x4c, 0x49,
		  0x43, 0x20, 0x4b, 0x45, 0x59, 0x2d, 0x2d, 0x2d, 0x2d, 0x2d, 0x0a
	};
}

RsaKeyRepo::~RsaKeyRepo() {
	delete [] c_rsapub_2048_pem;
	delete [] c_rsapub_1024_pem;
}

} /* namespace alpharng */
