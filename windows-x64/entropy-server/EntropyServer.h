/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class is used for interacting with the hardware random data generator device AlphaRNG for the purpose of
 downloading and distributing true random bytes using named pipes.

 It uses OpenSSL library.

 */

 /**
  *    @file EntropyServer.h
  *    @date 03/10/2021
  *    @Author: Andrian Belinski
  *    @version 1.0
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
	bool fill_entropy_for_write(DWORD i);
	bool retrieve_entropy(DWORD i);

public:
	static const int c_max_pipe_instances = 64;
	static const int c_default_pipe_instances = 10;

private:
	AlphaRngApi *m_rng;
	Cmd *m_cmd;

	static const int c_pipe_timeout = 5000;
	static const int c_write_buff_size_bytes = 100000;
	static const int c_connectiong_state = 0;
	static const int c_reading_state = 1;
	static const int c_writing_state = 2;
	static const int c_cmd_entropy_retrieve_id = 0;
	static const int c_cmd_diag_id = 1;

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