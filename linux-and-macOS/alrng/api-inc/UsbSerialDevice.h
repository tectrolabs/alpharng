/**
 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the access to the AlphaRNG device over a CDC USB interface on Linux and macOS platforms.

*/

/**
 *    @file UsbSerialDevice.h
 *    @date 06/23/2024
 *    @Author: Andrian Belinski
 *    @version 1.3
 *
 *    @brief Implements the API for communicating with the AlphaRNG device
 */
#ifndef ALPHARNG_USBSERIALDEVICE_H_
#define ALPHARNG_USBSERIALDEVICE_H_

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <cstring>
#include <string>
#include <sstream>
#include <sys/types.h>
#include <sys/file.h>
#include <cstdint>

#ifdef __FreeBSD__
#include <sys/param.h>
#endif

#include <DeviceInterface.h>

namespace alpharng {

class UsbSerialDevice : public DeviceInterface {
public:

	bool is_connected() override;
	bool connect(const char *device_path_name) override;
	bool disconnect() override;
	std::string get_error_log() override;
	void clear_error_log() override;
	int send_data(unsigned char *snd, int size_snd, int *bytes_sent) override;
	int receive_data(unsigned char *rcv, int size_receive, int *bytes_received) override;
	int get_device_count() override;
	void scan_available_devices() override;
	bool retrieve_device_path(char *dev_path_name, int max_dev_path_name_bytes, int device_number) override;
	bool set_connection_timeout(int milliseconds) override;

	UsbSerialDevice();
	~UsbSerialDevice() override;

private:
	void set_error_message(const char *error_message);
	void purge_comm_data() const;

private:
	static const int c_max_devices = 25;
	static const int c_max_size_device_name = 128;
	int m_fd {-1};
	int m_lock;
	char c_device_names[c_max_devices][c_max_size_device_name];
	int m_active_device_count {0};
	bool m_device_connected {false};
	std::ostringstream m_error_log_oss;
	struct termios m_opts;
	const int c_timeout_mlsecs = 100;
};

} /* namespace alpharng */

#endif /* ALPHARNG_USBSERIALDEVICE_H_ */
