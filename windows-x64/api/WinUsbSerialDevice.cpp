/**
 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the access to the AlphaRNG device over a CDC USB interface on Windows platform.

 */

 /**
  *    @file WinUsbSerialDevice.cpp
  *    @date 04/24/2024
  *    @Author: Andrian Belinski
  *    @version 1.3
  *
  *    @brief Implements the API for communicating with the CDC USB interface
  */

#include <WinUsbSerialDevice.h>

namespace alpharng {


	WinUsbSerialDevice::WinUsbSerialDevice() {
		m_device_connected = false;
		m_active_device_count = 0;
		m_cdc_device_handle = nullptr;
		clear_error_log();
		m_comm_error = 0;
	}

	void WinUsbSerialDevice::clear_error_log() {
		m_error_log_oss.str("");
		m_error_log_oss.clear();
	}

	/**
	 * Check to see if a connection to the AlphaRNG device is already established.
	 *
	 * @return true if connection is established
	 */
	bool WinUsbSerialDevice::is_connected() {
		return m_device_connected;
	}

	/**
	* Connect to AlphaRNG device through COM port name provided
	*
	* @param char *device_path_name - pointer to a COM port name
	* @return true - successful operation
	*
	*/
	bool WinUsbSerialDevice::connect(const char* device_path_name)
	{
		if (device_path_name == nullptr || strlen(device_path_name) > c_max_size_device_name - 1)
		{
			set_error_message("Invalid device name");
			return false;
		}

		string device_path_name_str(device_path_name);
		wstring port_name = wstring(device_path_name_str.begin(), device_path_name_str.end()).c_str();
		return connect(port_name.c_str());
	}

	/**
	* Connect to AlphaRNG device through COM port name provided
	*
	* @param WCHAR *com_port - pointer to a COM port pathname
	* @return true - successful operation
	*
	*/
	bool WinUsbSerialDevice::connect(const WCHAR* com_port) {
		if (is_connected()) {
			return false;
		}

		clear_error_log();

		m_cdc_device_handle = CreateFile(com_port, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_cdc_device_handle == INVALID_HANDLE_VALUE) {
			if (GetLastError() == ERROR_FILE_NOT_FOUND) {
				set_error_message("COM port not found");
			}
			else {
				set_error_message("Could not open COM port");
			}
			return false;
		}

		m_device_connected = true;
		set_connection_timeout(100);
		purge_comm_data();
		return true;
	}

	/**
	 * Set connection read/write timeout in milliseconds.
	 *
	 * @param[i]  milliseconds
	 *
	 * @return true for successful operation
	 */
	bool WinUsbSerialDevice::set_connection_timeout(int milliseconds) {
		if (!is_connected()) {
			return false;
		}

		COMMTIMEOUTS time_out = { 0 };
		time_out.ReadIntervalTimeout = 0;
		time_out.ReadTotalTimeoutConstant = milliseconds;
		time_out.ReadTotalTimeoutMultiplier = 0;
		time_out.WriteTotalTimeoutConstant = milliseconds;
		time_out.WriteTotalTimeoutMultiplier = 0;
		SetCommTimeouts(m_cdc_device_handle, &time_out);

		return true;
	}

	void WinUsbSerialDevice::set_error_message(const char* error_message) {
		m_error_log_oss << error_message;
	}

	/**
	 * Disconnect from COM port
	 *
	 * @return true for successful operation
	 */
	bool WinUsbSerialDevice::disconnect() {
		if (!is_connected()) {
			return false;
		}
		if (m_cdc_device_handle != nullptr)
		{
			CloseHandle(m_cdc_device_handle);
		}
		m_cdc_device_handle = nullptr;
		clear_error_log();
		m_device_connected = false;
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
	int WinUsbSerialDevice::send_data(unsigned char* snd, int size_snd, int* bytes_sent) {
		DWORD actualBytesSent;
		int retStatus = -1;
		if (!is_connected()) {
			return retStatus;
		}

		BOOL status = WriteFile(m_cdc_device_handle, (void*)snd, size_snd, &actualBytesSent, 0);
		*bytes_sent = (int)actualBytesSent;
		if (!status || *bytes_sent != size_snd)
		{
			if (status && *bytes_sent != size_snd) {
				retStatus = -7; // Time out
				set_error_message("Got timeout while sending data to device");
			}
			else {
				set_error_message("Could not send data to device");
			}

			clear_comm_error();
			purge_comm_data();
			return retStatus;
		}
		return 0;
	}

	string WinUsbSerialDevice::get_error_log() {
		return m_error_log_oss.str();
	}

	void WinUsbSerialDevice::purge_comm_data() {
		if (!is_connected()) {
			return;
		}
		PurgeComm(m_cdc_device_handle, PURGE_RXCLEAR | PURGE_TXCLEAR);
	}

	/**
	 * Receive data from the AlphaRNG device
	 *
	 * @param[out] rcv points to location for the bytes to receive
	 * @param[in] size_receive expected amount of bytes to receive
	 * @param[out] bytes_rceived point to a location to store the actual bytes received
	 *
	 * @return 0 for successful operation, -7 for operation timeout or other error value
	 *
	 */
	int WinUsbSerialDevice::receive_data(unsigned char* rcv, int size_receive, int* bytes_rceived) {
		DWORD actualBytesReceived;
		int retStatus = -1;
		if (!is_connected()) {
			return retStatus;
		}

		BOOL status = ReadFile(m_cdc_device_handle, (void*)rcv, size_receive, &actualBytesReceived, NULL);
		*bytes_rceived = (int)actualBytesReceived;
		if (!status || *bytes_rceived != size_receive) {
			if (status && *bytes_rceived != size_receive) {
				retStatus = -7; // Time out
				set_error_message("Got timeout while receiving data from the device");
			}
			else {
				set_error_message("Could not receive data from the device");
			}

			clear_comm_error();
			purge_comm_data();
			return retStatus;
		}
		return 0;
	}

	WinUsbSerialDevice::~WinUsbSerialDevice() {
		if (is_connected()) {
			disconnect();
		}
	}

	void WinUsbSerialDevice::clear_comm_error() {
		if (!is_connected()) {
			return;
		}
		ClearCommError(m_cdc_device_handle, &m_comm_error, &m_comm_status);
	}


/**
* Scan registry and discover all the AlphaRNG devices attached to COM ports
*
* @param int ports[] - an array of integers that represent all of the AlphaRNG COM ports found
* @param int max_ports - the maximum number of AlphaRNG devices to discover
* @param  int *actual_count - a pointer to the actual number of AlphaRNG devices found
* @param  WCHAR *hardware_id - a pointer to AlphaRNG device hardware ID
* @param WCHAR* serial_id - a pointer to AlphaRNG device hardware serial ID
*
* @return 0 - successful operation, otherwise the error code
*
*/
void WinUsbSerialDevice::get_connected_ports(int ports[], int max_ports, int* actual_count, WCHAR* hardware_id, WCHAR* serial_id) {

		DWORD dev_idx = 0;
		int found_port_index = 0;
		HDEVINFO h_dev_info;
		SP_DEVINFO_DATA dev_info_data;
		BYTE current_hardware_id[1024] = { 0 };
		DEVPROPTYPE dev_prop_type;
		DWORD dw_size;
		
		h_dev_info = SetupDiGetClassDevs(
			NULL,
			L"USB",
			NULL,
			DIGCF_ALLCLASSES | DIGCF_PRESENT);

		if (h_dev_info == INVALID_HANDLE_VALUE)
			return;


		dev_info_data.cbSize = sizeof(SP_DEVINFO_DATA);

		while (SetupDiEnumDeviceInfo(
			h_dev_info,
			dev_idx,
			&dev_info_data) && found_port_index < max_ports)
		{
			dev_idx++;
			if (SetupDiGetDeviceRegistryProperty(h_dev_info, &dev_info_data, SPDRP_HARDWAREID, &dev_prop_type, (BYTE*)current_hardware_id, sizeof(current_hardware_id), &dw_size))
			{
				HKEY h_device_registry_key;
				h_device_registry_key = SetupDiOpenDevRegKey(h_dev_info, &dev_info_data, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
				if (h_device_registry_key == INVALID_HANDLE_VALUE)
				{
					break;
				}
				else
				{
					wchar_t curPortName[80];
					DWORD dw_port_name_size = sizeof(curPortName);
					DWORD dw_type = 0;

					if ((RegQueryValueEx(h_device_registry_key, L"PortName", NULL, &dw_type, (LPBYTE)curPortName, &dw_port_name_size) == ERROR_SUCCESS) && (dw_type == REG_SZ))
					{
						if (_tcsnicmp(curPortName, _T("COM"), 3) == 0)
						{
							TCHAR* src = (TCHAR*)current_hardware_id;
							if (_tcsnicmp(hardware_id, (TCHAR*)current_hardware_id, _tcsnlen(hardware_id, 80)) == 0) {
								int port_nr = _ttoi(curPortName + 3);
								if (port_nr != 0)
								{
									DEVINST dev_instance_parent_id;
									TCHAR sz_dev_instance_id[MAX_DEVICE_ID_LEN];
									CONFIGRET status = CM_Get_Parent(&dev_instance_parent_id, dev_info_data.DevInst, 0);
									if (status == CR_SUCCESS)
									{
										status = CM_Get_Device_ID(dev_instance_parent_id, sz_dev_instance_id, MAX_DEVICE_ID_LEN, 0);
										if (status == CR_SUCCESS) {
											if (std::wstring(sz_dev_instance_id).find(serial_id) != std::string::npos) {
												ports[found_port_index++] = port_nr;
											}
										}
									}
								}
							}

						}
					}
					RegCloseKey(h_device_registry_key);
				}
			}

		}

		if (h_dev_info)
		{
			SetupDiDestroyDeviceInfoList(h_dev_info);
		}
		*actual_count = found_port_index;
}

	/**
	 * Scan for available AlphaRNG devices currently connected, up to `c_max_devices` devices.
	 * This method will populate the internal structures with device path names and numbers.
	 */
	void WinUsbSerialDevice::scan_available_devices() {
		m_active_device_count = 0;
		get_connected_ports(m_ports, c_max_devices, &m_active_device_count, (WCHAR *)L"USB\\VID_1FC9&PID_8111", (WCHAR*)L"ALPHARNG");
		for (int i = 0; i < m_active_device_count; ++i)
		{
			sprintf_s(m_device_names[i], c_max_size_device_name, "\\\\.\\COM%d", m_ports[i]);
		}
	}

	/**
	 * Get the number of AlphaRNG devices currently connected.
	 * This method should be called after calling scan_available_devices()
	 *
	 * @return number of connected devices
	 */
	int WinUsbSerialDevice::get_device_count() {
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
	bool WinUsbSerialDevice::retrieve_device_path(char* dev_path_name, int max_dev_path_name_bytes, int device_number) {
		if (device_number >= m_active_device_count) {
			return false;
		}
		strcpy_s(dev_path_name, max_dev_path_name_bytes, m_device_names[device_number]);
		return true;
	}

} /* namespace alpharng */
