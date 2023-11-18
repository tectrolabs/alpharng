/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for interacting with the hardware random data generator device AlphaRNG for the purpose of
 downloading and distributing true random bytes using named pipes.

 It uses OpenSSL library.

 */

 /**
  *    @file EntropyServer.cpp
  *    @date 11/18/2023
  *    @Author: Andrian Belinski
  *    @version 1.4
  *
  *    @brief A pipe service for distributing true random bytes generated by an AlphaRNG device
  */

#include "EntropyServer.h"

namespace alpharng {

	
/**
* Constructor
*/
EntropyServer::EntropyServer(AlphaRngApi* rng, Cmd* cmd) {
	this->m_cmd = cmd;
	this->m_rng = rng;
}

/**
* @param idx - pipe instance index
*/
void EntropyServer::reconnect(DWORD idx)
{
	if (!DisconnectNamedPipe(m_pipe[idx].hPipeInst))
	{
		cerr << "DisconnectNamedPipe failed with " << GetLastError() << "." << endl;
	}

	m_pipe[idx].fPendingIO = connect_to_new_clinet(
		m_pipe[idx].hPipeInst,
		&m_pipe[idx].oOverlap);

	m_pipe[idx].dwState = m_pipe[idx].fPendingIO ?
		c_connectiong_state : // still connecting 
		c_reading_state;     // ready to read 
}

/*
* This function is called to start an overlapped connect operation.
* 
* @param hPipe pipe handle
* @param lpo overlapped flag
* @return TRUE if an operation is pending or FALSE if the
*/

BOOL EntropyServer::connect_to_new_clinet(HANDLE hPipe, LPOVERLAPPED lpo)
{
	BOOL fConnected, fPendingIO = FALSE;
	fConnected = ConnectNamedPipe(hPipe, lpo);
	if (fConnected)
	{
		cerr << "ConnectNamedPipe failed with " << GetLastError() << "." << endl;
		return 0;
	}

	switch (GetLastError())
	{
	case ERROR_IO_PENDING:
		fPendingIO = TRUE;
		break;
	case ERROR_PIPE_CONNECTED:
		if (SetEvent(lpo->hEvent))
			break;
	default:
	{
		cerr << "ConnectNamedPipe failed with " << GetLastError() << "." << endl;
		return 0;
	}
	}
	return fPendingIO;
}

/**
* Start and run the pipe service
*
* @return true when run successfully
*/
bool EntropyServer::run() {

	if (!m_rng->connect(m_cmd->device_number)) {
		cerr << m_rng->get_last_error() << endl;
		return false;
	}

	if (!m_cmd->pipe_name.empty()) {
		m_pipe_endpoint = wstring(m_cmd->pipe_name.begin(), m_cmd->pipe_name.end()).c_str();
	}
	else {
		m_pipe_endpoint = m_default_pipe_endpoint;
	}

	bool pipesStatus = create_pipe_instances();
	if (pipesStatus != true) {
		return pipesStatus;
	}

	string id;
	string model;
	unsigned char major_version;
	unsigned char minor_version;
	m_rng->retrieve_device_id(id);
	m_rng->retrieve_device_model(model);
	m_rng->retrieve_device_major_version(&major_version);
	m_rng->retrieve_device_minor_version(&minor_version);

	cout << "Entropy server started using device '" << model << "' with S/N: " << id << " and Ver: " << (int)major_version << "." << (int)minor_version << endl;

	while (1)
	{
		m_dw_wait = WaitForMultipleObjects(
			m_pipe_instances,    // number of event objects 
			m_handle_events,      // array of event objects 
			FALSE,        // does not wait for all 
			INFINITE);    // wait indefinitely 

		m_dw_idx = m_dw_wait - WAIT_OBJECT_0;
		if (m_dw_idx >(m_pipe_instances - 1))
		{
			cerr << "Index out of range." << endl;
			return false;
		}

		if (m_pipe[m_dw_idx].fPendingIO)
		{
			m_f_success = GetOverlappedResult(
				m_pipe[m_dw_idx].hPipeInst, // handle to pipe 
				&m_pipe[m_dw_idx].oOverlap, // OVERLAPPED structure 
				&m_dw_cb_ret,            // bytes transferred 
				FALSE);            // do not wait 

			switch (m_pipe[m_dw_idx].dwState)
			{
			case c_connectiong_state:
				if (!m_f_success)
				{
					cerr << "Error " << GetLastError() << " ." << endl;
					return false;
				}
				m_pipe[m_dw_idx].dwState = c_reading_state;
				break;

			case c_reading_state:
				if (!m_f_success || m_dw_cb_ret == 0)
				{
					reconnect(m_dw_idx);
					continue;
				}
				m_pipe[m_dw_idx].cbRead = m_dw_cb_ret;
				m_pipe[m_dw_idx].dwState = c_writing_state;
				break;

			case c_writing_state:
				if (!m_f_success || m_dw_cb_ret != m_pipe[m_dw_idx].chRequest.cbReqData)
				{
					reconnect(m_dw_idx);
					continue;
				}
				m_pipe[m_dw_idx].dwState = c_reading_state;
				break;

			default:
			{
				cerr << "Invalid pipe state." << endl;
				return false;
			}
			}
		}

		switch (m_pipe[m_dw_idx].dwState)
		{
		case c_reading_state:
			m_f_success = ReadFile(
				m_pipe[m_dw_idx].hPipeInst,
				&m_pipe[m_dw_idx].chRequest,
				sizeof(READCMD),
				&m_pipe[m_dw_idx].cbRead,
				&m_pipe[m_dw_idx].oOverlap);

			if (m_f_success && m_pipe[m_dw_idx].cbRead == sizeof(READCMD))
			{
				m_pipe[m_dw_idx].fPendingIO = FALSE;
				m_pipe[m_dw_idx].dwState = c_writing_state;
				continue;
			}
			m_dw_err = GetLastError();
			if (!m_f_success && (m_dw_err == ERROR_IO_PENDING))
			{
				m_pipe[m_dw_idx].fPendingIO = TRUE;
				continue;
			}
			reconnect(m_dw_idx);
			break;

		case c_writing_state:

			if (!fill_entropy_for_write(m_dw_idx)) {
				reconnect(m_dw_idx);
				break;
			}
			m_f_success = WriteFile(
				m_pipe[m_dw_idx].hPipeInst,
				m_pipe[m_dw_idx].chReply,
				m_pipe[m_dw_idx].chRequest.cbReqData,
				&m_dw_cb_ret,
				&m_pipe[m_dw_idx].oOverlap);

			if (m_f_success && m_dw_cb_ret == m_pipe[m_dw_idx].chRequest.cbReqData)
			{
				m_pipe[m_dw_idx].fPendingIO = FALSE;
				m_pipe[m_dw_idx].dwState = c_reading_state;
				continue;
			}

			m_dw_err = GetLastError();
			if (!m_f_success && (m_dw_err == ERROR_IO_PENDING))
			{
				m_pipe[m_dw_idx].fPendingIO = TRUE;
				continue;
			}
			reconnect(m_dw_idx);
			break;

		default:
		{
			cerr << "Invalid pipe state." << endl;
			return false;
		}
		}
	}

	return true;
}

/**
* Create a pool of named pipe instances
*
* @return true when run successfully
*/
bool EntropyServer::create_pipe_instances() {
	for (m_dw_idx = 0; m_dw_idx < m_pipe_instances; m_dw_idx++)
	{

		m_handle_events[m_dw_idx] = CreateEvent(
			NULL,
			TRUE,
			TRUE,
			NULL);

		if (m_handle_events[m_dw_idx] == NULL)
		{
			cerr << "CreateEvent failed with " << GetLastError() << " error code." << endl;
			return false;
		}

		m_pipe[m_dw_idx].oOverlap.hEvent = m_handle_events[m_dw_idx];

		m_pipe[m_dw_idx].hPipeInst = CreateNamedPipe(
			m_pipe_endpoint.c_str(),// pipe name 
			PIPE_ACCESS_DUPLEX |    // read/write access 
			FILE_FLAG_OVERLAPPED,   // overlapped mode 
			PIPE_TYPE_BYTE |		// byte-type pipe 
			PIPE_READMODE_BYTE |	// byte-read mode 
			PIPE_WAIT,              // blocking mode 
			m_pipe_instances,       // number of instances 
			c_write_buff_size_bytes,// output buffer size 
			sizeof(READCMD),		// input buffer size 
			c_pipe_timeout,         // client time-out 
			NULL);                  // default security attributes 

		if (m_pipe[m_dw_idx].hPipeInst == INVALID_HANDLE_VALUE)
		{
			cerr << "CreateNamedPipe failed with " << GetLastError() << " error code." << endl;
			return false;
		}

		m_pipe[m_dw_idx].fPendingIO = connect_to_new_clinet(
			m_pipe[m_dw_idx].hPipeInst,
			&m_pipe[m_dw_idx].oOverlap);

		m_pipe[m_dw_idx].dwState = m_pipe[m_dw_idx].fPendingIO ? c_connectiong_state : c_reading_state;
	}
	return true;
}

/**
* Populate the entropy buffer
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::fill_entropy_for_write(DWORD idx) {
	bool devStatus = false;
	unsigned char testCounter = 0;
	if (m_pipe[idx].chRequest.cbReqData == 0 || m_pipe[idx].chRequest.cbReqData > c_write_buff_size_bytes) {
		return devStatus;
	}
	switch (m_pipe[idx].chRequest.cmd) {
	case c_cmd_entropy_retrieve_id:
		devStatus = retrieve_entropy(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_entropy(idx);
			}
		}
		break;
	case c_cmd_entropy_sha256_extract_id:
		devStatus = extract_sha256_entropy(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = extract_sha256_entropy(idx);
			}
		}
		break;
	case c_cmd_entropy_sha512_extract_id:
		devStatus = extract_sha512_entropy(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = extract_sha512_entropy(idx);
			}
		}
		break;
	case c_cmd_noise_id:
		devStatus = retrieve_noise(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_noise(idx);
			}
		}
		break;
	case c_cmd_dev_ser_num_id:
		devStatus = retrieve_device_serial_number(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_device_serial_number(idx);
			}
		}
		break;
	case c_cmd_dev_model_id:
		devStatus = retrieve_device_model(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_device_model(idx);
			}
		}
		break;
	case c_cmd_dev_minor_version_id:
		devStatus = retrieve_device_minor_version(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_device_minor_version(idx);
			}
		}
		break;
	case c_cmd_dev_major_version_id:
		devStatus = retrieve_device_major_version(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_device_major_version(idx);
			}
		}
		break;
	case c_cmd_serv_minor_version_id:
		devStatus = retrieve_server_minor_version(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_server_minor_version(idx);
			}
		}
		break;
	case c_cmd_serv_major_version_id:
		devStatus = retrieve_server_major_version(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_server_major_version(idx);
			}
		}
		break;
	case c_cmd_noise_src_one_id:
		devStatus = retrieve_noise_source_1(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_noise_source_1(idx);
			}
		}
		break;
	case c_cmd_noise_src_two_id:
		devStatus = retrieve_noise_source_2(idx);
		if (devStatus != true) {
			m_rng->disconnect();
			devStatus = m_rng->connect(m_cmd->device_number);
			if (devStatus == true) {
				devStatus = retrieve_noise_source_2(idx);
			}
		}
		break;
	case c_cmd_diag_id:
		testCounter = 0;
		for (DWORD t = 0; t < m_pipe[idx].chRequest.cbReqData; t++) {
			m_pipe[idx].chReply[t] = testCounter++;
		}
		devStatus = true;
		break;
	default:
		cerr << "Invalid command received: " << m_pipe[idx].chRequest.cmd << endl;
		devStatus = false;
	}
	if (!devStatus) {
		log_device_error();
	}
	return devStatus;
}

/**
* Populate the entropy buffer with entropy bytes retrieved directly from the device.
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_entropy(DWORD idx) {
	return m_rng->get_entropy(m_pipe[idx].chReply, m_pipe[idx].chRequest.cbReqData);
}

/**
* Populate the entropy buffer with entropy bytes by applying SHA-256 method to RAW
* random bytes retrieved from the AlphaRNG device.
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::extract_sha256_entropy(DWORD idx) {
	return m_rng->extract_sha256_entropy(m_pipe[idx].chReply, m_pipe[idx].chRequest.cbReqData);
}

/**
* Populate the entropy buffer with entropy bytes by applying SHA-512 method to RAW 
* random bytes retrieved from the AlphaRNG device.
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::extract_sha512_entropy(DWORD idx) {
	return m_rng->extract_sha512_entropy(m_pipe[idx].chReply, m_pipe[idx].chRequest.cbReqData);
}

/**
* Populate the buffer with bytes from the device noise source 1
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_noise_source_1(DWORD idx) {
	return m_rng->get_noise_source_1(m_pipe[idx].chReply, m_pipe[idx].chRequest.cbReqData);
}

/**
* Populate the buffer with concatenated raw random bytes of both noise sources.
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_noise(DWORD idx) {
	return m_rng->get_noise(m_pipe[idx].chReply, m_pipe[idx].chRequest.cbReqData);
}

/**
* Populate the buffer with bytes from the device noise source 2
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_noise_source_2(DWORD idx) {
	return m_rng->get_noise_source_2(m_pipe[idx].chReply, m_pipe[idx].chRequest.cbReqData);
}

/**
* Retrieve serial number (identifier) of connected device
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_device_serial_number(DWORD idx) {
	string id;
	bool status = m_rng->retrieve_device_id(id);
	if (status == true) {
		if (m_pipe[idx].chRequest.cbReqData != id.size()) {
			return false;
		}
		memcpy(m_pipe[idx].chReply, id.c_str(), id.size());
	}
	return status;
}

/**
* Retrieve model of connected device
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_device_model(DWORD idx) {
	string model;
	bool status = m_rng->retrieve_device_model(model);
	if (status == true) {
		if (m_pipe[idx].chRequest.cbReqData != model.size()) {
			return false;
		}
		memcpy(m_pipe[idx].chReply, model.c_str(), model.size());
	}
	return status;
}

/**
* Retrieve device minor version
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_device_minor_version(DWORD idx) {
	unsigned char version;
	bool status = m_rng->retrieve_device_minor_version(&version);
	if (status == true) {
		if (m_pipe[idx].chRequest.cbReqData != sizeof(version)) {
			return false;
		}
		memcpy(m_pipe[idx].chReply, &version, sizeof(version));
	}
	return status;
}

/**
* Retrieve device major version
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_device_major_version(DWORD idx) {
	unsigned char version;
	bool status = m_rng->retrieve_device_major_version(&version);
	if (status == true) {
		if (m_pipe[idx].chRequest.cbReqData != sizeof(version)) {
			return false;
		}
		memcpy(m_pipe[idx].chReply, &version, sizeof(version));
	}
	return status;
}

/**
* Retrieve server minor version
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_server_minor_version(DWORD idx) {
	if (m_pipe[idx].chRequest.cbReqData != sizeof(c_server_minor_version)) {
		return false;
	}
	memcpy(m_pipe[idx].chReply, &c_server_minor_version, sizeof(c_server_minor_version));
	return true;
}

/**
* Retrieve server major version
*
* @param idx - pipe instance index
* @return true when run successfully
*/
bool EntropyServer::retrieve_server_major_version(DWORD idx) {
	if (m_pipe[idx].chRequest.cbReqData != sizeof(c_server_major_version)) {
		return false;
	}
	memcpy(m_pipe[idx].chReply, &c_server_major_version, sizeof(c_server_major_version));
	return true;
}

/**
* Log the latest device error message
*/
void EntropyServer::log_device_error() {
	if (m_cmd->err_log_enabled) {

		auto now = std::chrono::system_clock::now();
		auto time_t_now = std::chrono::system_clock::to_time_t(now);
		char str[30]{ };
		ctime_s(str, sizeof(str), &time_t_now);
		std::string s(str);
		std::replace(s.begin(), s.end(), '\n', '\0');
		cerr << s << ": " << "Device latest error message : " << m_rng->get_last_error() << endl;
	}
}

} /* namespace alpharng */