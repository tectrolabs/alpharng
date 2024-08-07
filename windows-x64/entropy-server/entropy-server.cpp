/**
 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program may only be used in conjunction with TectroLabs devices.

 This program is used for interacting with the hardware random data generator device AlphaRNG for the purpose of
 downloading and distributing true random bytes using named pipes.

 It uses OpenSSL library.

 */

 /**
  *    @file entropy-server.cpp
  *    @date 07/23/2024
  *    @Author: Andrian Belinski
  *    @version 1.6
  *
  *    @brief A pipe service for distributing true random bytes generated by an AlphaRNG device
  */

#include <AlphaRngApi.h>
#include <AppArguments.h>
#include <EntropyServer.h>
#include <new>

using namespace std;
using namespace alpharng;

AppArguments appArgs({
	{"-d", ArgDef::requireArgument},
	{"-e", ArgDef::noArgument},
	{"-h", ArgDef::noArgument},
	{"-m", ArgDef::requireArgument},
	{"-k", ArgDef::requireArgument},
	{"-c", ArgDef::requireArgument},
	{"-p", ArgDef::requireArgument},
	{"-P", ArgDef::requireArgument},
	{"-i", ArgDef::requireArgument},
	{"-E", ArgDef::requireArgument},
	{"-dt", ArgDef::noArgument},
	{"-th", ArgDef::requireArgument},
	{"-le", ArgDef::noArgument},
	{"-ttl", ArgDef::requireArgument}
	});

/**
* Current version of this application
*/
static double const version = 1.6;

static bool extract_command(Cmd& cmd, RngConfig& cfg, const int argc, const char** argv);
static bool validate_comand(const Cmd& cmd);
static void display_help();

/**
 * Application entry point
 * @param[in] argc number of arguments provided in command line
 * @param[in] argv points to the location with command line parameters
 *
 * @return 0 if command and options extracted successfully
 */
int main(const int argc, const char** argv) {
	
	RngConfig cfg;
	Cmd cmd;
	if (!extract_command(cmd, cfg, argc, argv)) {
		return -1;
	}

	if (!validate_comand(cmd)) {
		return -1;
	}

	if (cfg.key_file.size() > 0) {
		RsaCryptor rsa(cfg.key_file, true);
		if (!rsa.is_initialized()) {
			cerr << "Could not load the RSA public key file: " << cfg.key_file << endl;
			return -1;
		}
	}

	if (cmd.cmd_type == CmdOpt::getHelp) {
		display_help();
		return 0;
	}
	
	AlphaRngApi rng (AlphaRngConfig{ cfg.e_mac_type, cfg.e_rsa_key_size, cfg.e_aes_key_size, cfg.key_file });

	if (!rng.set_session_ttl(cmd.ttl_minutes)) {
		cerr << rng.get_last_error() << endl;
		return -1;
	}

	if (cmd.disable_stat_tests == true) {
		rng.disable_stat_tests();
	}
	rng.set_num_failures_threshold(cmd.num_failures_threshold);

	unique_ptr<EntropyServer> server{ new EntropyServer(&rng, &cmd) };

	if (!server->run()) {
		return -1;
	}
	return 0;
}

/**
 * Parse and extract command and options from the command line
 *
 * @param[out] cmd resulting command
 * @param[out] cfg configuration
 * @param[in] argc number of arguments provided in command line
 * @param[in] argv points to the location with command line parameters
 *
 * @return true if command and options extracted successfully
 */
static bool extract_command(Cmd& cmd, RngConfig& cfg, const int argc, const char** argv) {
	appArgs.load_arguments(argc, argv);
	if (appArgs.is_error()) {
		cerr << appArgs.get_last_error();
		return false;
	}

	cmd.device_number = 0;
	cmd.op_count = 0;
	cmd.cmd_type = CmdOpt::none;
	cmd.pipe_instances = EntropyServer::c_default_pipe_instances;
	cmd.disable_stat_tests = false;
	cmd.num_failures_threshold = HealthTests::s_min_num_failures_threshold;
	cmd.err_log_enabled = false;
	cmd.ttl_minutes = 0;

	cfg.e_mac_type = MacType::None;
	cfg.e_aes_key_size = KeySize::k256;
	cfg.e_rsa_key_size = RsaKeySize::rsa2048;

	map<string, string> arg_map = appArgs.get_argument_map();
	for (map<string, string>::iterator it = arg_map.begin(); it != arg_map.end(); ++it) {
		string option = it->first;
		string value = it->second;

		if (option.length() <= 1) {
			cerr << "Invalid option: " << option << endl;
			return false;
		}

		char c = option.at(1);

		switch (c) {
		case 'E':
			cmd.pipe_name = value;
			break;
		case 'h':
			cmd.cmd_type = CmdOpt::getHelp;
			cmd.op_count++;
			break;
		case 'e':
			cmd.cmd_type = CmdOpt::getEntropy;
			cmd.op_count++;
			break;
		case 'l':
			cmd.err_log_enabled = true;
			break;
		case 'k':
			cfg.key_file = value;
			break;
		case 'P':
			cmd.pipe_name = value;
			break;
		case 'm':
			if (value.compare("hmacSha160") == 0) {
				cfg.e_mac_type = MacType::hmacSha160;
				break;
			}
			if (value.compare("hmacMD5") == 0) {
				cfg.e_mac_type = MacType::hmacMD5;
				break;
			}
			if (value.compare("hmacSha256") == 0) {
				cfg.e_mac_type = MacType::hmacSha256;
				break;
			}
			if (value.compare("none") == 0) {
				cfg.e_mac_type = MacType::None;
				break;
			}
			cerr << "unexpected mac option specified, must be hmacMD5, hmacSha160, hmacSha256 or none" << endl;
			return false;
			break;
		case 'c':
			if (value.compare("aes256") == 0) {
				cfg.e_aes_key_size = KeySize::k256;
				break;
			}
			if (value.compare("aes128") == 0) {
				cfg.e_aes_key_size = KeySize::k128;
				break;
			}
			if (value.compare("none") == 0) {
				cfg.e_aes_key_size = KeySize::None;
				break;
			}
			cerr << "unexpected cipher option specified, must be aes256, aes128 or none" << endl;
			return false;
			break;
		case 'p':
			if (value.compare("RSA1024") == 0) {
				cfg.e_rsa_key_size = RsaKeySize::rsa1024;
				break;
			}
			if (value.compare("RSA2048") == 0) {
				cfg.e_rsa_key_size = RsaKeySize::rsa2048;
				break;
			}
			cerr << "unexpected RSA option specified, must be RSA1024, RSA2048 or RSA4096" << endl;
			return false;
			break;
		case 'd':
			if (option.length() == 3 && option.at(2) == 't') {
				cmd.disable_stat_tests = true;
			}
			else {
				cmd.device_number = atoi(value.c_str());
			}
			break;
		case 'i':
			cmd.pipe_instances = atoi(value.c_str());
			break;
		case 't':
			if (option.length() == 4 && option.at(2) == 't' && option.at(3) == 'l') {
				int val = atoi(value.c_str());
				if (val < 1) {
					cerr << "unexpected ttl " << val << " value, must be a positive number in minutes" << endl;
					return false;
				}
				cmd.ttl_minutes = val;
				break;
			}
			else if (option.length() == 3 && option.at(2) == 'h') {
				int val = atoi(value.c_str());
				if (val < 6 || val > 255) {
					cerr << "unexpected threshold for number of failures, must be between 6 and 255" << endl;
					return false;
				}
				cmd.num_failures_threshold = val;
				break;
			}
			else {
				cerr << "unexpected option " << option << endl;
				return false;
			}
			break;
		default:
			cerr << "Unexpected option: " << c << endl;
			return false;
		}
	}
	return true;
}

/**
 * Validate command
 *
 * @param[in] cmd command to be validated
 *
 * @return true if command is valid
 */
static bool validate_comand(const Cmd& cmd) {
	if (cmd.op_count > 1) {
		cerr << "Too many 'get' options specified, choose only one" << endl;
		return false;
	}

	if (cmd.op_count == 0) {
		cerr << "No function letter specified. Use -h for help" << endl;
		return false;
	}

	if (cmd.device_number < 0 || cmd.device_number > 25) {
		cerr << "Invalid device number specified: " << cmd.device_number << endl;
		return false;
	}

	if (cmd.pipe_instances < 0 || cmd.pipe_instances > EntropyServer::c_max_pipe_instances) {
		cerr << "Invalid amount of pipe instances specified: " << cmd.pipe_instances << endl;
		return false;
	}

	return true;
}

/**
 * Display usage
 */
void display_help() {
	cout << "*********************************************************************************" << endl;
	cout << "                       AlphaRNG entropy-server Ver ";
	cout << std::fixed << std::setw(2) << std::setprecision(1) << version << endl;
	cout << "*********************************************************************************" << endl;
	cout << "NAME" << endl;
	cout << "     entropy-server - An application server for distributing random bytes" << endl;
	cout << "                      downloaded from AlphaRNG device" << endl;
	cout << "SYNOPSIS" << endl;
	cout << "     entropy-server <operation mode> [options]" << endl;
	cout << endl;
	cout << "DESCRIPTION" << endl;
	cout << "     entropy-server downloads random bytes from Hardware (True) " << endl;
	cout << "     Random Number Generator AlphaRNG device and distributes them to" << endl;
	cout << "     consumer applications using a named pipe." << endl;
	cout << endl;
	cout << "FUNCTION LETTERS" << endl;
	cout << "     Main operation mode:" << endl;
	cout << endl;
	cout << "     -e" << endl;
	cout << "           start the entropy server for retrieving/extracting and distributing" << endl;
	cout << "           entropy bytes from an AlhaRNG device using a named pipe." << endl;
	cout << endl;
	cout << "OPTIONS" << endl;
	cout << endl;
	cout << "     -d NUMBER" << endl;
	cout << "           USB device NUMBER, if more than one. Skip this option if only" << endl;
	cout << "           one AlphaRNG device is connected." << endl;
	cout << endl;
	cout << "     -m MAC" << endl;
	cout << "           MAC type: hmacMD5, hmacSha160, hmacSha256 or none - skip this option for none." << endl;
	cout << endl;
	cout << "     -p KEYTYPE" << endl;
	cout << "           Public KEYTYPE: RSA1024 or RSA2048 - skip this option for RSA2048." << endl;
	cout << endl;
	cout << "     -c CIPHER" << endl;
	cout << "           CIPHER type: aes256, aes128 or none - skip this option for aes256." << endl;
	cout << endl;
	cout << "     -k FILE" << endl;
	cout << "           FILE pathname with an alternative RSA 2048 public key, supplied by the manufacturer." << endl;
	cout << endl;
	cout << "     -E ENDPOINT" << endl;
	cout << "           ENDPOINT: a custom named pipe endpoint (if different from the default endpoint)." << endl;
	cout << endl;
	cout << "     -i NUMBER" << endl;
	cout << "          How many pipe instances to create (default: " << EntropyServer::c_default_pipe_instances << ")" << endl;
	cout << "          It also defines how many concurrent requests the server can handle" << endl;
	cout << "          Valid values are integers from 1 to " << EntropyServer::c_max_pipe_instances << endl;
	cout << endl;
	cout << "     -dt" << endl;
	cout << "           Disable APT and RCT statistical tests." << endl;
	cout << endl;
	cout << "     -th NUMBER" << endl;
	cout << "           Set threshold for number of failures per APT and RCT test blocks. Must be between 6 and 255" << endl;
	cout << endl;
	cout << "     -le" << endl;
	cout << "           Log all errors on standard error stream. Use this option with caution as it may result" << endl;
	cout << "           in flooding the standard error stream with many error messages." << endl;
	cout << endl;
	cout << "     -ttl MINUTES" << endl;
	cout << "           Set session time to live in minutes. A new session will be created every specified " << endl;
	cout << "           amount of minutes within a connection. MINUTES must be a positive number." << endl;
	cout << "           Skip this option if session should never expire for a connection." << endl;
	cout << endl;
	cout << "EXAMPLES:" << endl;
	cout << "     To start the server using AlphaRNG device with default security settings:" << endl;
	cout << "           entropy-server -e " << endl;
	cout << "    To start the server using first AlphaRNG device and custom pipe endpoint name:" << endl;
	cout << "          entropy-server -e -E \\\\.\\pipe\\my_custom_pipename" << endl;
	cout << endl;
}


