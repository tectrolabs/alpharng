/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This interface may only be used in conjunction with TectroLabs devices.

 This interface is used for communicating with a hardware random number generator AlphaRNG device.

 */

/**
 *    @file DeviceInterface.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Provides an API for communicating with the AlphaRNG device
 */

#ifndef API_SRC_DEVICE_H_
#define API_SRC_DEVICE_H_

#include <string>

using namespace std;

namespace alpharng {

class DeviceInterface {

public:
	virtual bool is_connected() = 0;
	virtual bool connect(const char *device_path_name) = 0;
	virtual bool disconnect() = 0;
	virtual string get_error_log() = 0;
	virtual void clear_error_log() = 0;
	virtual int send_data(unsigned char *snd, int size_snd, int *bytes_sent) = 0;
	virtual int receive_data(unsigned char *rcv, int size_receive, int *bytes_rceived) = 0;
	virtual int get_device_count() = 0;
	virtual void scan_available_devices() = 0;
	virtual bool retrieve_device_path(char *dev_path_name, int max_dev_path_name_bytes, int device_number) = 0;
	virtual bool set_connection_timeout(int milliseconds) = 0;

	DeviceInterface() {};
	virtual ~DeviceInterface() {};
};

} /* namespace alpharng */

#endif /* API_SRC_DEVICE_H_ */
