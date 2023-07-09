/**1 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the access to the AlphaRNG device over a CDC USB interface on Linux and macOS platforms.

 */

/**
 *    @file UsbSerialDevice.cpp
 *    @date 7/8/2023
 *    @Author: Andrian Belinski
 *    @version 1.1
 *
 *    @brief Implements the API for communicating with the CDC USB interface
 */

#include <UsbSerialDevice.h>

namespace alpharng {


UsbSerialDevice::UsbSerialDevice() {
	this-> m_device_connected = false;
	this->m_fd = -1;
	m_active_device_count = 0;
	clear_error_log();
}

void UsbSerialDevice::clear_error_log() {
	m_error_log_oss.str("");
	m_error_log_oss.clear();
}

/**
 * Check to see if a connection to the AlphaRNG device is already established.
 *
 * @return true if connection is established
 */
bool UsbSerialDevice::is_connected() {
	return this->m_device_connected;
}

/**
 * Connect to the device specified by `device_path_name`.
 * It will only work if the specified device is available and it is unlocked.
 * Upon successful connection, the specific device will be locked for usage.
 *
 * @param[in] device_path_name points to a null terminated string for device name
 *
 * @return true if connected successfully
 */
bool UsbSerialDevice::connect(const char *device_path_name) {
	if (is_connected()) {
		return false;
	}

	clear_error_log();

	this->m_fd = open(device_path_name, O_RDWR | O_NOCTTY);
	if (this->m_fd == -1) {
		m_error_log_oss << "Could not open serial device: " << device_path_name << ". ";
		return false;;
	}

	// Lock the device
	this->m_lock = flock(this->m_fd, LOCK_EX | LOCK_NB);
	if (this->m_lock != 0) {
		m_error_log_oss << "Could not lock device: " << device_path_name << "." << endl;
		close(m_fd);
		return false;
	}

	purge_comm_data();

	int retVal = tcgetattr(m_fd, &m_opts);
	if (retVal) {
		m_error_log_oss << "Could not retrieve configuration from serial device: " << device_path_name << ". ";
		close(m_fd);
		return false;
	}

	m_opts.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	m_opts.c_iflag &= ~(INLCR | IGNCR | ICRNL | IXON | IXOFF);
	m_opts.c_oflag &= ~(ONLCR | OCRNL);

	if (!set_connection_timeout(c_timeout_mlsecs)) {
		close(m_fd);
		return false;
	}
	this->m_device_connected = true;
	return true;
}

/**
 * Set connection read/write timeout in milliseconds.
 *
 * @param[i]  milliseconds
 *
 * @return true for successful operation
 */
bool UsbSerialDevice::set_connection_timeout(int milliseconds) {
	// Set time out to 100 milliseconds for read serial device operations
	int conn_timeout = milliseconds < 100 ? 1 : milliseconds / 100;
	m_opts.c_cc[VTIME] = conn_timeout;
	m_opts.c_cc[VMIN] = 0;

	int retVal = tcsetattr(m_fd, TCSANOW, &m_opts);
	if (retVal) {
		m_error_log_oss <<  "Could not set configuration for serial device" << ". ";
		return false;
	}
	return true;
}

void UsbSerialDevice::set_error_message(const char *error_message) {
	m_error_log_oss << error_message;
}

/**
 * Disconnect from the device and remove the lock.
 *
 * @return true for successful operation
 */
bool UsbSerialDevice::disconnect() {
	if (!is_connected()) {
		return false;
	}
	flock(this->m_fd, LOCK_UN);
	close(m_fd);
	this->m_device_connected = false;
	clear_error_log();
	return true;
}

/**
 * Send data to the AlphaRNG device
 *
 * @param[in] snd points to location of the bytes to send
 * @param[in] size_snd how many bytes to send
 * @param[out] bytes_sent point to a location to store the actual bytes sent
 *
 * @return 0 for successful operation, otherwise it is the error code
 */
int UsbSerialDevice::send_data(unsigned char *snd, int size_snd, int *bytes_sent) {
	if (!is_connected()) {
		return -1;
	}
	ssize_t ret_val = write(this->m_fd, snd, size_snd);
	if (ret_val != (ssize_t)size_snd) {
		set_error_message("Could not send data to serial device");
		return -1;
	}
	fsync(this->m_fd);

	*bytes_sent = ret_val;
	return 0;
}

string UsbSerialDevice::get_error_log() {
	return m_error_log_oss.str();
}

void UsbSerialDevice::purge_comm_data() {
	if (this->m_fd != -1) {
		tcflush(this->m_fd, TCIOFLUSH);
	}
}

/**
 * Receive data from the AlphaRNG device
 *
 * @param[out] rcv points to location for the bytes to receive
 * @param[in] size_receive expected amount of bytes to receive
 * @param[out] bytes_received point to a location to store the actual bytes received
 *
 * @return 0 for successful operation, -7 for operation timeout or other error value
 *
 */
int UsbSerialDevice::receive_data(unsigned char *rcv, int size_receive, int *bytes_received) {
	int return_status = 0;
	if (!is_connected()) {
		return -1;
	}

	size_t actual_bytes_received = 0;
	while (actual_bytes_received < (size_t)size_receive) {
		ssize_t received_count = read(this->m_fd, rcv + actual_bytes_received, size_receive - actual_bytes_received);
		if (received_count < 0) {
			set_error_message("Could not receive data from serial device");
			return_status = -1;
			break;
		}
		if (received_count == 0) {
			// Operation timed out
			m_error_log_oss << "expected to receive " << size_receive << " bytes, actual received " << actual_bytes_received << ".";
			return_status = -7;
			break;
		}
		actual_bytes_received += received_count;
	  }
	*bytes_received = actual_bytes_received;
	return return_status;
}

UsbSerialDevice::~UsbSerialDevice() {
	if (is_connected()) {
		disconnect();
	}
}


#ifndef __FreeBSD__
/**
 * Scan for available AlphaRNG devices currently connected in Linux or macOS, up to `c_max_devices` devices.
 * This method will populate the internal structures with device path names and numbers.
 */
void UsbSerialDevice::scan_available_devices() {
	m_active_device_count = 0;
#ifdef __linux__
	char command[] = "/bin/ls -1l /dev/serial/by-id 2>&1 | grep -i \"TectroLabs_Alpha_RNG\"";
#else
	char command[] = "/bin/ls -1a /dev/cu.usbmodemALPHARNG* /dev/cu.usbmodemFD* 2>&1";
#endif
	FILE *pf = popen(command,"r");
	if (pf == nullptr) {
		return;
	}

	char line[512];
	while (fgets(line, sizeof(line), pf) && m_active_device_count < c_max_devices) {
#ifdef __linux__
		char *tty = strstr(line, "ttyACM");
		if (tty == nullptr) {
			continue;
		}
#else
		int cmp1  = strncmp(line, "/dev/cu.usbmodemALPHARNG", 24);
		int cmp2  = strncmp(line, "/dev/cu.usbmodemFD", 18);
		if (cmp1 != 0 && cmp2 != 0 ) {
			continue;
		}
		char *tty = line;
#endif
		int size_tty = strlen(tty);
		for (int i = 0; i < size_tty; i++) {
			if(tty[i] < 33 || tty[i] > 125) {
				tty[i] = 0;
			}
		}
#ifdef __linux__
		strcpy(c_device_names[m_active_device_count], "/dev/");
		strcat(c_device_names[m_active_device_count], tty);
#else
		strcpy(c_device_names[m_active_device_count], tty);
#endif
		m_active_device_count++;
	}
	pclose(pf);

}
#endif

#ifdef __FreeBSD__
/**
 * Scan for available AlphaRNG devices currently connected in FreeBSD, up to `c_max_devices` devices.
 * This method will populate the internal structures with device path names and numbers.
 */
void UsbSerialDevice::scan_available_devices() {
	m_active_device_count = 0;
	bool device_candidate = false;
	char command[] = "usbconfig show_ifdrv | grep -E \"TectroLabs Alpha RNG|VCOM\" | grep -vi \"(tectrolabs)\"";
	FILE *pf = popen(command,"r");
	if (pf == nullptr) {
		return;
	}

	char line[512];
	while (fgets(line, sizeof(line), pf) && m_active_device_count < c_max_devices) {
		if (device_candidate == false && strstr(line, "Alpha RNG") != nullptr) {
			device_candidate = true;
			continue;
		}
		if (device_candidate) {
			if (strstr(line, "VCOM") != nullptr && strstr(line, "umodem") != nullptr) {
				// Found VCOM description. Extract 'cuaU' number.
				char *p = strstr(line, "umodem");
				if (p == nullptr) {
					continue;
				}
				char *tkn = strtok(p + strlen("umodem"), ":");
				if (tkn != nullptr) {
					strcpy(c_device_names[m_active_device_count], "/dev/cuaU");
					strcat(c_device_names[m_active_device_count], tkn);
					m_active_device_count++;
					device_candidate = false;
				}
			} else {
				device_candidate = false;
			}
		}
	}
	pclose(pf);
}
#endif

/**
 * Get the number of AlphaRNG devices currently connected.
 * This method should be called after calling scan_available_devices()
 *
 * @return number of connected devices
 */
int UsbSerialDevice::get_device_count() {
	return m_active_device_count;
}

/**
 * Given a device number, retrieve the device path associated.
 * It should be called after get_device_count().
 *
 * @param[out] dev_path_name points to location to be filled with null terminated device path
 * @param[in] max_dev_path_name_bytes destination max size in bytes
 * @param[in] device_number pass 0 for the first device
 *
 * @return true for successful operation
 *
 */
bool UsbSerialDevice::retrieve_device_path(char *dev_path_name, int max_dev_path_name_bytes, int device_number) {
	if (device_number >= m_active_device_count) {
		m_error_log_oss << "Device number: " << device_number << " exceeds the maximum limit: " << m_active_device_count  << "." << endl;
		return false;
	}
	if(max_dev_path_name_bytes < c_max_size_device_name - 1) {
		m_error_log_oss << "Destination size too small: " << max_dev_path_name_bytes  << "." << endl;	
		return false;
	}
	memset(dev_path_name, 0, max_dev_path_name_bytes);
	memcpy(dev_path_name, c_device_names[device_number], c_max_size_device_name);
	return true;
}

} /* namespace alpharng */
