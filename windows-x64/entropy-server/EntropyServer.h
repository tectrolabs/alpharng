/**
 Copyright (C) 2014-2021 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for interacting with the hardware random data generator device AlphaRNG for the purpose of
 downloading and distributing true random bytes using named pipes.

 It uses OpenSSL library.

 */

 /**
  *    @file EntropyServer.h
  *    @date 07/04/2021
  *    @Author: Andrian Belinski
  *    @version 1.2
  *
  *    @brief A pipe service for distributing true random bytes generated by an AlphaRNG device
  */

#ifndef ALRNG_ENTROPY_SERVER_H_
#define ALRNG_ENTROPY_SERVER_H_

#include <windows.h> 
#include <AlphaRngApi.h>

namespace alpharng {

class EntropyServer
{
public:
	EntropyServer(AlphaRngApi* rng, Cmd* cmd);
	bool run();

private:
	void reconnect(DWORD);
	BOOL connect_to_new_clinet(HANDLE, LPOVERLAPPED);
	bool create_pipe_instances();
	bool fill_entropy_for_write(DWORD idx);
	bool retrieve_entropy(DWORD idx);
	bool extract_sha256_entropy(DWORD idx);
	bool extract_sha512_entropy(DWORD idx);
	bool retrieve_noise_source_1(DWORD idx);
	bool retrieve_noise_source_2(DWORD idx);
	bool retrieve_noise(DWORD idx);
	bool retrieve_device_serial_number(DWORD idx);
	bool retrieve_device_model(DWORD idx);
	bool retrieve_device_minor_version(DWORD idx);
	bool retrieve_device_major_version(DWORD idx);
	bool retrieve_server_minor_version(DWORD idx);
	bool retrieve_server_major_version(DWORD idx);

public:
	static const int c_max_pipe_instances = 64;
	static const int c_default_pipe_instances = 10;

private:
	AlphaRngApi *m_rng;
	Cmd *m_cmd;

	static const char c_server_major_version = 1;
	static const char c_server_minor_version = 2;

	static const int c_pipe_timeout = 5000;
	static const int c_write_buff_size_bytes = 100000;
	static const int c_connectiong_state = 0;
	static const int c_reading_state = 1;
	static const int c_writing_state = 2;
	static const int c_cmd_entropy_retrieve_id = 0;
	static const int c_cmd_diag_id = 1;
	static const int c_cmd_dev_ser_num_id = 2;
	static const int c_cmd_dev_model_id = 3;
	static const int c_cmd_dev_minor_version_id = 4;
	static const int c_cmd_dev_major_version_id = 5;
	static const int c_cmd_serv_minor_version_id = 6;
	static const int c_cmd_serv_major_version_id = 7;
	static const int c_cmd_noise_src_one_id = 8;
	static const int c_cmd_noise_src_two_id = 9;
	static const int c_cmd_entropy_sha256_extract_id = 10;
	static const int c_cmd_entropy_sha512_extract_id = 11;
	static const int c_cmd_noise_id = 12;

	typedef struct
	{
		DWORD cmd;
		DWORD cbReqData;
	} READCMD;

	typedef struct
	{
		OVERLAPPED oOverlap;
		HANDLE hPipeInst;
		READCMD chRequest;
		DWORD cbRead;
		unsigned char chReply[c_write_buff_size_bytes];
		DWORD dwState;
		BOOL fPendingIO;
	} PIPEINST;


	PIPEINST m_pipe[c_max_pipe_instances];
	HANDLE m_handle_events[c_max_pipe_instances];
	int m_device_num = 0;
	DWORD m_pipe_instances = c_default_pipe_instances;
	DWORD m_dw_idx;
	DWORD m_dw_wait;
	DWORD m_dw_cb_ret = 0;
	DWORD m_dw_err = 0;
	BOOL m_f_success = false;
	wstring m_pipe_endpoint;
	wstring m_default_pipe_endpoint = {L"\\\\.\\pipe\\AlphaRNG"};

};

} /* namespace alpharng */

#endif /* ALRNG_ENTROPY_SERVER_H_ */