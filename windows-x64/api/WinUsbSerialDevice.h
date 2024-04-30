/**
 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the access to the AlphaRNG device over a CDC USB interface on Windows platform.

 */

 /**
  *    @file WinUsbSerialDevice.h
  *    @date 03/24/2024
  *    @Author: Andrian Belinski
  *    @version 1.2
  *
  *    @brief Implements the API for communicating with the AlphaRNG device
  */

#ifndef WINUSBSERIALDEVICE_H_
#define WINUSBSERIALDEVICE_H_

#include <sstream>
#include <initguid.h>
#include <windows.h>
#include <Setupapi.h>
#include <cfgmgr32.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <combaseapi.h>


#include <DeviceInterface.h>

using namespace std;
namespace alpharng {

class WinUsbSerialDevice :  public DeviceInterface
{
public:
	bool is_connected() override;
	bool connect(const char* device_path_name) override;
	bool disconnect() override;
	string get_error_log() override;
	void clear_error_log() override;
	int send_data(unsigned char* snd, int size_snd, int* bytes_sent) override;
	int receive_data(unsigned char* rcv, int size_receive, int* bytes_rceived) override;
	int get_device_count() override;
	void scan_available_devices() override;
	bool retrieve_device_path(char* dev_path_name, int max_dev_path_name_bytes, int device_number) override;
	bool set_connection_timeout(int milliseconds) override;

	WinUsbSerialDevice();
	virtual ~WinUsbSerialDevice();

private:
	bool connect(const WCHAR* comPort);
	void set_error_message(const char* error_message);
	void purge_comm_data();
	void clear_comm_error();
	void get_connected_ports(int ports[], int max_ports, int* actual_count, WCHAR* hardware_id, WCHAR* serial_id);

private:
	static const int c_max_devices = 25;
	static const int c_max_size_device_name = 128;
	char m_device_names[c_max_devices][c_max_size_device_name];
	int m_ports[c_max_devices];
	bool m_device_connected;
	int m_active_device_count;
	ostringstream m_error_log_oss;
	HANDLE m_cdc_device_handle;
	COMSTAT m_comm_status;
	DWORD m_comm_error;


};

} /* namespace alpharng */

#endif /* WINUSBSERIALDEVICE_H_ */