/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for interacting with the entropy server for the purpose of downloading true random bytes using named pipes.

 */

 /**
  *    @file EntropyServerConnector.cpp
  *    @date 03/10/2021
  *    @Author: Andrian Belinski
  *    @version 1.0
  *
  *    @brief A pipe service client for downloading true random bytes from the entropy server.
  */

#include "pch.h"
#include "EntropyServerConnector.h"

namespace alpharng {

	/*
	* Constructor
	*/
	EntropyServerConnector::EntropyServerConnector(const string& pipe_endpoint) {
		m_pipe_endpoint = wstring(pipe_endpoint.begin(), pipe_endpoint.end()).c_str();
	}

	/*
	* Open a named pipe to the entropy server
	*
	* @return true if successful
	*/
	bool EntropyServerConnector::open_named_pipe() {
		if (is_connected()) {
			m_error_log_oss << "Pipe is already open. " << endl;
			return false;
		}

		while (1) {
			m_pipe_handle = CreateFile(
				m_pipe_endpoint.c_str(),	// pipe name 
				GENERIC_READ |				// read and write access 
				GENERIC_WRITE,
				0,							// no sharing 
				NULL,						// default security attributes
				OPEN_EXISTING,				// opens existing pipe 
				0,							// default attributes 
				NULL);						// no template file 

			if (m_pipe_handle != INVALID_HANDLE_VALUE) {
				break;
			}

			if (GetLastError() != ERROR_PIPE_BUSY) {
				m_error_log_oss << "Could not create a named pipe connection. " << endl;
				return false;
			}

			if (!WaitNamedPipe(m_pipe_endpoint.c_str(), 20000)) {
				m_error_log_oss << "Received a timeout while establishing a named pipe connection. " << endl;
				return false;
			}
		}

		DWORD mode = PIPE_READMODE_BYTE;
		BOOL is_success = SetNamedPipeHandleState(
			m_pipe_handle,	// pipe handle 
			&mode,			// new pipe mode 
			NULL,
			NULL);
		if (!is_success) {
			m_error_log_oss << "Could not set the pipe handle state. " << endl;
			return false;
		}
		m_is_connected = true;
		return true;
	}

	/*
	* Retrieve entropy bytes from the pipe server
	*
	* @param rcv_buffer buffer for the incoming bytes
	* @param byte_count - number of bytes to receive
	* @return true if successful
	*/
	bool EntropyServerConnector::get_entropy(unsigned char* rcv_buffer, DWORD byte_count) {
		return get_bytes(EntropyServerCommand::getEntropy, rcv_buffer, byte_count);
	}

	/*
	* Retrieve test bytes from the pipe server
	* 
	 * This method is only used for testing the correctness of the data communication with the
	 * entropy server. Each byte retrieved, starting with 0, represents an incremented value of
	 * the previous byte value.
	*
	* @param rcv_buffer buffer for incoming bytes
	* @param byte_count - number of bytes to receive
	* @return true if successful
	*/
	bool EntropyServerConnector::get_test_bytes(unsigned char* rcv_buffer, DWORD byte_count) {
		return get_bytes(EntropyServerCommand::getTestData, rcv_buffer, byte_count);
	}

	/*
	* Retrieve bytes from the pipe server
	* 
	* @param cmd - command to send to the entropy server
	* @param rcv_buffer buffer for incoming bytes
	* @param byte_count - number of bytes to receive
	* @return true if successful
	*/
	bool EntropyServerConnector::get_bytes(EntropyServerCommand cmd, unsigned char* rcv_buffer, DWORD byte_count) {
		clear_error_log();
		if (!is_connected()) {
			m_error_log_oss << "Not connected to the entropy server" << endl;
			return false;
		}

		DWORD byte_count_to_write = sizeof(REQCMD);
		REQCMD req_cmd;
		req_cmd.cmd = (DWORD)cmd;
		req_cmd.num_bytes = byte_count;
		DWORD num_bytes_written;
		BOOL is_success = WriteFile(
			m_pipe_handle,		// pipe handle 
			&req_cmd,           // bytes 
			byte_count_to_write,// number of bytes to write 
			&num_bytes_written, // bytes actually written 
			NULL);              // not overlapped 

		if (!is_success) {
			m_error_log_oss << "Could not write " << byte_count_to_write << " bytes to the entropy server. " << endl;
			return false;
		}
		DWORD num_bytes_read;
		do {
			is_success = ReadFile(
				m_pipe_handle,  // pipe handle 
				rcv_buffer,		// buffer to receive reply 
				byte_count,		// size of buffer 
				&num_bytes_read,// number of bytes read 
				NULL);			// not overlapped 

			if (!is_success && GetLastError() != ERROR_MORE_DATA) {
				break;
			}
		} while (!is_success);  // repeat loop if ERROR_MORE_DATA 

		if (!is_success) {
			m_error_log_oss << "Could not read " << byte_count << " bytes from the entropy server. " << endl;
			return false;
		}
		if (byte_count != num_bytes_read) {
			m_error_log_oss << "Expected to receive " << byte_count << " bytes, actually received " << num_bytes_read <<  " from the entropy server. " << endl;
			return false;
		}

		return true;
	}

	/*
	* Close the named pipe to the entropy server
	*
	*/
	void EntropyServerConnector::close_named_pipe() {
		if (is_connected()) {
			CloseHandle(m_pipe_handle);
			m_is_connected = false;
		}
	}

	void EntropyServerConnector::clear_error_log() {
		m_error_log_oss.str("");
		m_error_log_oss.clear();
	}


} /* namespace alpharng */
