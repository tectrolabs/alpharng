/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program may only be used in conjunction with TectroLabs devices.

 This program is used for interacting with the hardware random data generator device AlphaRNG for the purpose of
 downloading entropy or random data.

 */

/**
 *    @file alrng.cpp
 *    @date 11/18/2023
 *    @Author: Andrian Belinski
 *    @version 2.0
 *
 *    @brief A utility used for downloading data from the AlphaRNG device
 */
#include <AlphaRngApi.h>
#include <AppArguments.h>
#include <iomanip>

using namespace std;
using namespace alpharng;

/**
* Valid command line arguments
*/
AppArguments appArgs ({
	{"-1", ArgDef::noArgument},
	{"-2", ArgDef::noArgument},
	{"-r", ArgDef::noArgument},
	{"-e", ArgDef::noArgument},
	{"-x", ArgDef::noArgument},
	{"-X", ArgDef::noArgument},
	{"-t", ArgDef::noArgument},
	{"-o", ArgDef::requireArgument},
	{"-n", ArgDef::requireArgument},
	{"-d", ArgDef::requireArgument},
	{"-l", ArgDef::noArgument},
	{"-s", ArgDef::noArgument},
	{"-h", ArgDef::noArgument},
	{"-m", ArgDef::requireArgument},
	{"-k", ArgDef::requireArgument},
	{"-c", ArgDef::requireArgument},
	{"-p", ArgDef::requireArgument},
	{"-dt", ArgDef::noArgument},
	{"-th", ArgDef::requireArgument}
});

/**
* Current version of this utility application
*/
static double const version = 2.0;

/**
* Local functions used
*/
static bool extract_command(Cmd &cmd, RngConfig &cfg, const int argc, const char **argv);
static bool validate_comand(const Cmd &cmd);
static bool list_connected_devices(const RngConfig cfg);
static void reset_statistics(DeviceStatistics *ds);
static void generate_statistics(DeviceStatistics &ds, const Cmd &cmd);
static void display_help();

/**
 * Application entry point
 * 
 * @param[in] argc number of arguments provided in command line
 * @param[in] argv points to the location with command line parameters
 *
 * @return 0 if command and options extracted successfully
 */
int main(const int argc, const char **argv) {

	DeviceStatistics ds;
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

	AlphaRngApi rng{AlphaRngConfig {cfg.e_mac_type, cfg.e_rsa_key_size, cfg.e_aes_key_size, cfg.key_file}};

	if (cmd.cmd_type != CmdOpt::listDevices && cmd.cmd_type != CmdOpt::getHelp && !rng.connect(cmd.device_number)) {
		cerr << rng.get_last_error() << endl;
		return -1;
	}

	reset_statistics(&ds);

	bool status = false;
	if (cmd.disable_stat_tests == true) {
		rng.disable_stat_tests();
	}

	rng.set_num_failures_threshold(cmd.num_failures_threshold);

	switch (cmd.cmd_type) {
	case CmdOpt::getEntropy:
		status = rng.entropy_to_file(cmd.out_file_name, cmd.num_bytes);
		break;
	case CmdOpt::extractSha256Entropy:
		status = rng.extract_sha256_entropy_to_file(cmd.out_file_name, cmd.num_bytes);
		break;
	case CmdOpt::extractSha512Entropy:
		status = rng.extract_sha512_entropy_to_file(cmd.out_file_name, cmd.num_bytes);
		break;
	case CmdOpt::getNoiseSourceOne:
		status = rng.noise_source_one_to_file(cmd.out_file_name, cmd.num_bytes);
		break;
	case CmdOpt::getNoiseSourceTwo:
		status = rng.noise_source_two_to_file(cmd.out_file_name, cmd.num_bytes);
		break;
	case CmdOpt::getNoise:
		status = rng.noise_to_file(cmd.out_file_name, cmd.num_bytes);
		break;
	case CmdOpt::runDiagnostics:
		status = rng.run_health_test();
		if (status) {
			cout << "OK" << endl;
		}
		break;
	case CmdOpt::listDevices:
		cmd.log_statistics = false;
		status = list_connected_devices(cfg);
		break;
	case CmdOpt::getHelp:
		cmd.log_statistics = false;
		status = true;
		display_help();
		break;
	default:
		cerr << "Invalid option: " << (int)cmd.cmd_type << endl;
		return -1;
	}

	if (!status) {
		cerr << "Err: " << rng.get_last_error() << endl;
		return -1;
	}

	generate_statistics(ds, cmd);
	if (cmd.log_statistics) {
		switch (cmd.cmd_type) {
		case CmdOpt::getEntropy:
		case CmdOpt::extractSha256Entropy:
		case CmdOpt::extractSha512Entropy:
		case CmdOpt::getNoiseSourceOne:
		case CmdOpt::getNoiseSourceTwo:
		case CmdOpt::getNoise:
			cout << "Recorded " << cmd.num_bytes << " bytes to " << cmd.out_file_name << " file, download speed: " << ds.download_speed_kbsec << " KB/sec";
			cout << ", retries: " << rng.get_operation_retry_count();
			cout << ", max RCT/APT block events: " << rng.get_health_tests().get_max_rct_failures() << "/" << rng.get_health_tests().get_max_apt_failures() << endl;
			break;
		default:
			break;
		}
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
static bool extract_command(Cmd &cmd, RngConfig &cfg, const int argc, const char **argv) {
	appArgs.load_arguments(argc, argv);
	if (appArgs.is_error()) {
		cerr << appArgs.get_last_error();
		return false;
	}

	cmd.device_number = 0;
	cmd.num_bytes = 0;
	cmd.op_count = 0;
	cmd.out_file_name = "";
	cmd.cmd_type = CmdOpt::none;
	cmd.log_statistics = false;
	cmd.disable_stat_tests = false;
	cmd.num_failures_threshold = HealthTests::s_min_num_failures_threshold;
	cmd.err_log_enabled = false;

	cfg.e_mac_type = MacType::None;
	cfg.e_aes_key_size = KeySize::k256;
	cfg.e_rsa_key_size = RsaKeySize::rsa2048;


	map<string, string> arg_map = appArgs.get_argument_map();
    for (map<string, string>::iterator it = arg_map.begin(); it != arg_map.end(); ++it)	{
    	string option = it->first;
    	string value = it->second;

    	if (option.length() <= 1) {
    		cerr << "Invalid option: " << option << endl;
    		return false;
    	}

    	char c = option.at(1);

    	switch(c) {
		case 'h':
			cmd.cmd_type = CmdOpt::getHelp;
			cmd.op_count++;
			break;
		case 'o':
			cmd.out_file_name = value;
			break;
		case 'e':
			cmd.cmd_type = CmdOpt::getEntropy;
			cmd.op_count++;
			break;
		case 'x':
			cmd.cmd_type = CmdOpt::extractSha256Entropy;
			cmd.op_count++;
			break;
		case 'X':
			cmd.cmd_type = CmdOpt::extractSha512Entropy;
			cmd.op_count++;
			break;
		case '1':
			cmd.cmd_type = CmdOpt::getNoiseSourceOne;
			cmd.op_count++;
			break;
		case '2':
			cmd.cmd_type = CmdOpt::getNoiseSourceTwo;
			cmd.op_count++;
			break;
		case 't':
			if (option.length() == 3 && option.at(2) == 'h') {
				cmd.num_failures_threshold = atoi(value.c_str());
				if (cmd.num_failures_threshold < 6 || cmd.num_failures_threshold > 255) {
					cerr << "unexpected threshold for number of failures, must be between 6 and 255" << endl;
					return false;
				}
			} else {
				cmd.cmd_type = CmdOpt::runDiagnostics;
				cmd.op_count++;
			}
			break;
		case 'r':
			cmd.cmd_type = CmdOpt::getNoise;
			cmd.op_count++;
			break;
		case 'l':
			cmd.cmd_type = CmdOpt::listDevices;
			cmd.op_count++;
			break;
		case 'n':
			cmd.num_bytes = atoll(value.c_str());
			if (cmd.num_bytes == 0LL || cmd.num_bytes > 200000000000LL) {
				cerr << "unexpected number of bytes requested, must be between 1 and  200000000000" << endl;
				return false;
			}
			break;
		case 'k':
			cfg.key_file = value;
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
			} else {
				cmd.device_number = atoi(value.c_str());
			}
			break;
		case 's':
			cmd.log_statistics = true;
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
static bool validate_comand(const Cmd &cmd) {
	if (cmd.op_count > 1) {
		cerr << "Too many 'get' options specified, choose only one" << endl;
		return false;
	}

	if (cmd.op_count == 0) {
		cerr << "No function letter specified. Use -h for help" << endl;
		return false;
	}

	if (cmd.out_file_name.length() == 0 && cmd.cmd_type != CmdOpt::listDevices
			&& cmd.cmd_type != CmdOpt::getHelp && cmd.cmd_type != CmdOpt::runDiagnostics) {
		cerr << "Output file name not specified" << endl;
		return false;
	}

	if (cmd.device_number < 0 || cmd.device_number > 25) {
		cerr << "Invalid device number specified: " << cmd.device_number << endl;
		return false;
	}

	return true;
}

/**
 * Display information about all AlphaRNG connected and available devices.
 * @param[in] cfg RNG configuration data
 *
 * @return true for successful operation
 */
static bool list_connected_devices(const RngConfig cfg) {
	AlphaRngApi rng{AlphaRngConfig {cfg.e_mac_type, cfg.e_rsa_key_size, cfg.e_aes_key_size, cfg.key_file}};
	string id;
	string model;
	unsigned char major_version;
	unsigned char minor_version;

	int device_count = rng.get_device_count();
	if (device_count == 0) {
		cout << "No AlphaRNG connected devices found" << endl;
		return true;
	}

	for (int i = 0; i < device_count; ++i) {
		rng.disconnect();
		cout << "Device " << i << ": ";
		if (!rng.connect(i)) {
			cout << "could not connect" << endl;
			continue;
		}
		rng.retrieve_device_id(id);
		rng.retrieve_device_model(model);
		rng.retrieve_device_major_version(&major_version);
		rng.retrieve_device_minor_version(&minor_version);
		cout << "'" << model << "', S/N: " << id << ", version: " <<
				(int)major_version << "." << (int)minor_version << endl;
	}

	return true;
}

/**
 * Reset statistics, fill it with zeros
 * @param[in] ds pointer to DeviceStatistics structure
 *
 */
static void reset_statistics(DeviceStatistics *ds) {
	memset(ds, 0x0, sizeof(DeviceStatistics));
	time(&ds->begin_time);
}

/**
 * Generate performance statistics
 *
 * @param[out] ds device statistics
 * @param[in] cmd device command
 *
 *
 */
static void generate_statistics(DeviceStatistics &ds, const Cmd &cmd) {
	time(&ds.end_time);
	ds.total_time = ds.end_time - ds.begin_time;
	if (ds.total_time == 0) {
		ds.total_time = 1;
	}
	ds.download_speed_kbsec = (int) (cmd.num_bytes / (int64_t) 1024 / ds.total_time);
}

/**
 * Display usage
 */
void display_help() {
	cout << "*********************************************************************************" << endl;
	cout << "             TectroLabs - alrng - AlphaRNG download utility Ver ";
	cout << std::fixed << std::setw(2) << std::setprecision(1) << version << endl;
	cout << "*********************************************************************************" << endl;
	cout << "NAME" << endl;
	cout << "     alrng  - True Random Number Generator AlphaRNG download utility" << endl;
	cout << "SYNOPSIS" << endl;
	cout << "     alrng <operation mode> -o <file name> -n <number of bytes> [options]" << endl;
	cout << endl;
	cout << "DESCRIPTION" << endl;
	cout << "     alrng establishes a secure data communication channel with AlphaRNG devices" << endl;
	cout << "          connected through USB interface and downloads device data to a file." << endl;
	cout << endl;
	cout << "FUNCTION LETTERS" << endl;
	cout << "     Main operation mode:" << endl;
	cout << endl;
	cout << "     -l" << endl;
	cout << "           list all available (not currently in use) AlphaRNG devices." << endl;
	cout << endl;
	cout << "     -e" << endl;
	cout << "           download entropy bytes extracted from an AlphaRNG device to a file." << endl;
	cout << endl;
	cout << "     -x" << endl;
	cout << "           extract entropy bytes into a file by applying SHA-256 method" << endl;
	cout << "           to concatenated RAW random bytes of both noise sources retrieved from" << endl;
	cout << "           an AlphaRNG device. The SHA input/output extraction ratio used is 2/1 ." << endl;
	cout << endl;
	cout << "     -X" << endl;
	cout << "           extract entropy bytes into a file by applying SHA-512 method" << endl;
	cout << "           to concatenated RAW random bytes of both noise sources retrieved from" << endl;
	cout << "           an AlphaRNG device. The SHA input/output extraction ratio used is 2/1 ." << endl;
	cout << endl;
	cout << "     -r" << endl;
	cout << "           download concatenated raw random bytes of both noise sources" << endl;
	cout << "           from an AlphaRNG device to a file." << endl;
	cout << endl;
	cout << "     -1" << endl;
	cout << "           download raw random bytes from the first noise source." << endl;
	cout << "           of an AlphaRNG device to a file." << endl;
	cout << endl;
	cout << "     -2" << endl;
	cout << "           download raw random bytes from the second noise source." << endl;
	cout << "           of an AlphaRNG device to a file." << endl;
	cout << endl;
	cout << "     -t" << endl;
	cout << "           run AlphaRNG device internal diagnostics." << endl;
	cout << endl;
	cout << "     -h" << endl;
	cout << "           display help." << endl;
	cout << endl;
	cout << "OPTIONS" << endl;
	cout << endl;
	cout << "     -o FILE" << endl;
	cout << "           a FILE name for storing downloaded bytes." << endl;
	cout << endl;
	cout << "     -n NUMBER" << endl;
	cout << "           NUMBER of bytes to download, max value 200000000000" << endl;
	cout << "           Skip this option for unlimited (continuous) download. " << endl;
	cout << endl;
	cout << "     -d NUMBER" << endl;
	cout << "           USB device NUMBER, if more than one. Skip this option if only" << endl;
	cout << "           one AlphaRNG device is connected, use '-l' to list all available devices." << endl;
	cout << endl;
	cout << "     -m MAC" << endl;
	cout << "           MAC type: hmacMD5, hmacSha160, hmacSha256 or none - skip this option for none." << endl;
	cout << endl;
	cout << "     -p KEYTYPE" << endl;
	cout << "           Public KEYTYPE: RSA1024 or RSA2048 - skip this option for RSA2048." << endl;
	cout << "           RSA is used for establishing a secure session with an AlphaRNG device." << endl;
	cout << endl;
	cout << "     -c CIPHER" << endl;
	cout << "           CIPHER type: aes256, aes128 or none - skip this option for aes256." << endl;
	cout << "           aes256 refers to AES-256-GCM implementation. aes128 refers to AES-128-GCM implementation. " << endl;
	cout << "           AES cipher is used for securing the data communication within an AlphaRNG session." << endl;
	cout << endl;
	cout << "     -k FILE" << endl;
	cout << "           FILE pathname with an alternative RSA 2048 public key, supplied by the manufacturer." << endl;
	cout << endl;
	cout << "     -dt" << endl;
	cout << "           Disable APT and RCT statistical tests." << endl;
	cout << endl;
	cout << "     -th NUMBER" << endl;
	cout << "           Set threshold for number of failures per APT and RCT test blocks. Must be between 6 and 255" << endl;
	cout << endl;
	cout << "     -s" << endl;
	cout << "           Log statistics such as file name, amount of bytes downloaded, download speed, e.t.c " << endl;
	cout << endl;
	cout << "EXAMPLES:" << endl;
	cout << "     To list all available AlphaRNG (not currently in use) devices:" << endl;
	cout << "           alrng -l" << endl;
	cout << "     To download 1 MB of entropy bytes to 'rnd.bin' file using a non secure mode" << endl;
	cout << "           alrng  -e -o rnd.bin -n 1000000 -c none" << endl;
	cout << "     To download 1 MB of entropy bytes to 'rnd.bin' file using AES-128-GCM cipher and hmacSha256 digest:" << endl;
	cout << "           alrng  -e -o rnd.bin -n 1000000 -c aes128 -m hmacSha256" << endl;
	cout << "     To download 1 MB of raw (unprocessed) random bytes to 'rnd.bin' file using AES-256-GCM cipher:" << endl;
	cout << "           alrng  -r -o rnd.bin -n 1000000" << endl;
	cout << endl;
}
