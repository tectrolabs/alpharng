/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the access to the AlphaRNG device over a CDC USB interface on Linux and macOS platforms.

 */

/**
 *    @file UsbSerialDevice.h
 *    @date 01/10/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Implements the API for communicating with the AlphaRNG device
 */

#ifndef USBSERIALDEVICE_H_
#define USBSERIALDEVICE_H_

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/file.h>
#include <cstdint>

#include "DeviceInterface.h"

using namespace std;
namespace alpharng {

class UsbSerialDevice : public DeviceInterface {
public:

	bool is_connected();
	bool connect(const char *device_path_name);
	bool disconnect();
	string get_error_log();
	void clear_error_log();
	int send_data(unsigned char *snd, int size_snd, int *bytes_sent);
	int receive_data(unsigned char *rcv, int size_receive, int *bytes_rceived);
	int get_device_count();
	void scan_available_devices();
	bool retrieve_device_path(char *dev_path_name, int device_number);
	bool set_connection_timeout(int milliseconds);

	UsbSerialDevice();
	virtual ~UsbSerialDevice();

private:
	void set_error_message(const char *error_message);
	void purge_comm_data();

private:
	static const int c_max_devices = 25;
	static const int c_max_size_device_name = 128;
	int m_fd;
	int m_lock;
	char c_device_names[c_max_devices][c_max_size_device_name];
	int m_active_device_count;
	bool m_devicees_connected;
	ostringstream m_error_log_oss;
	struct termios m_opts;
	const int c_timeout_mlsecs = 100;
};

} /* namespace alpharng */

#endif /* USBSERIALDEVICE_H_ */
