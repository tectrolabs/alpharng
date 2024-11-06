/**
 Copyright (C) 2014-2024 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program may only be used in conjunction with TectroLabs devices.

 This program is used for interacting with the hardware random data generator device AlphaRNG for the purpose of
 generating random sequences of unique integer numbers.

 */

/**
 *    @file alseqgen.cpp
 *    @date 11/05/2024
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief A program for generating random sequences of unique integer numbers based on true random bytes produced by an AlphaRNG device.
 */

#include <AlphaRngApi.h>
#include <AppArguments.h>
#include <AlphaRandomRangeSequence.h>
#include <iomanip>
#include <memory>

using namespace std;
using namespace alpharng;

/**
* Valid command line arguments
*/
AppArguments appArgs ({
	{"-g", ArgDef::noArgument},
	{"-d", ArgDef::requireArgument},
	{"-s", ArgDef::requireArgument},
	{"-l", ArgDef::requireArgument},
	{"-n", ArgDef::requireArgument},
	{"-h", ArgDef::noArgument},
	{"-o", ArgDef::requireArgument},
	{"-m", ArgDef::requireArgument},
	{"-k", ArgDef::requireArgument},
	{"-c", ArgDef::requireArgument},
	{"-p", ArgDef::requireArgument}
});

/**
* Current version of this utility application
*/
static double const version = 1.0;

/**
* Local functions used
*/
static bool extract_command(Cmd &cmd, RngConfig &cfg, const int argc, const char **argv);
static bool validate_comand(const Cmd &cmd);
static void display_help();
static bool generate_sequence(AlphaRngApi *rng, int32_t smallest_value, int32_t largest_value, uint32_t sequence_size, const string &file_path_name);

/**
 * Application entry point
 *
 * @param[in] argc number of arguments provided in command line
 * @param[in] argv points to the location with command line parameters
 *
 * @return 0 if command and options extracted successfully
 */
int main(const int argc, const char **argv) {

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

	if (cmd.cmd_type == CmdOpt::generateSequence && !rng.connect(cmd.device_number)) {
		cerr << rng.get_last_error() << endl;
		return -1;
	}

	bool status = false;

	switch (cmd.cmd_type) {
	case CmdOpt::getHelp:
		status = true;
		display_help();
		break;
	case CmdOpt::generateSequence:
			status = generate_sequence(&rng, (int32_t)cmd.smallest_value, (int32_t)cmd.largest_value, (uint32_t)cmd.sequence_size, cmd.out_file_name);
			break;
	default:
		cerr << "Invalid option: " << (int)cmd.cmd_type << endl;
		return -1;
	}

	if (!status) {
		return -1;
	}

	return 0;
}

/**
 * Generate randomized sequence for specific range
 *
 * @param[in] AlphaRngApi *rng a pointer to RNG
 * @param[in] int32_t smallest_value smallest value in sequence
 * @param[in] int32_t largest_value largest value in sequence
 * @param[in] uint32_t sequence_size how many random integer numbers to generate
 * @param[in] string &file_path_name file name for storing generated integers in binary format
 *
 * @return true when executed successfully
 */

static bool generate_sequence(AlphaRngApi *rng, int32_t smallest_value, int32_t largest_value, uint32_t sequence_size, const string &file_path_name) {
	int32_t *buffer = new (std::nothrow) int32_t[sequence_size];
	if (buffer == nullptr) {
		cerr << "Could not allocate memory for data buffer." << endl;
		return false;
	}
	AlphaRandomRangeSequence seq_gen {rng, smallest_value, largest_value};
	bool status = seq_gen.generate_sequence(buffer, sequence_size);
	if (status == false) {
		cerr << seq_gen.get_last_err_msg();
	}

	if (status == true) {
		if (file_path_name.empty()) {
			cout << std::endl;
			cout << "-- Beginning of random sequence --" << endl;
			for (uint32_t i = 0; i < sequence_size; ++i) {
				cout << buffer[i] << endl;
			}
			cout << "-- Ending of random sequence --" << endl;
		} else {
			ofstream os_file(file_path_name.c_str(), ios::out | ios::binary);
			if(!os_file.good()) {
				cerr << "Could not open file: " << file_path_name << ". " << endl;
			} else {
				os_file.write((const char*)buffer, sequence_size * sizeof(int32_t));
				if(!os_file.good()) {
					cerr << "Could not write bytes to file: " << file_path_name << ". " << endl;
				}

			}
			os_file.close();
			if(!os_file.good()) {
				cerr << "Could not close file: " << file_path_name << ". " << endl;
			}
		}
	}
	delete [] buffer;
	return status;
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
	cmd.smallest_value = -10000000000;
	cmd.largest_value = -10000000000;
	cmd.sequence_size = 0;

	cfg.e_mac_type = MacType::None;
	cfg.e_aes_key_size = KeySize::k256;
	cfg.e_rsa_key_size = RsaKeySize::rsa2048;


	map<string, string> arg_map = appArgs.get_argument_map();
	for (auto const& map : arg_map)	{
    	string option = map.first;
    	string value = map.second;

    	if (option.length() <= 1) {
    		cerr << "Invalid option: " << option << endl;
    		return false;
    	}

    	char c = option.at(1);

    	switch(c) {
		case 'g':
			cmd.cmd_type = CmdOpt::generateSequence;
			cmd.op_count++;
			break;
		case 'h':
			cmd.cmd_type = CmdOpt::getHelp;
			cmd.op_count++;
			break;
		case 's':
			cmd.smallest_value = atol(value.c_str());
			break;
		case 'l':
			cmd.largest_value = atol(value.c_str());
			break;
		case 'n':
			cmd.sequence_size = atol(value.c_str());
			break;
		case 'k':
			cfg.key_file = value;
			break;
		case 'o':
			cmd.out_file_name = value;
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
			cmd.device_number = atoi(value.c_str());
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
		cerr << "Too many operation modes specified, choose only one" << endl;
		return false;
	}

	if (cmd.op_count == 0) {
		cerr << "No operation mode specified. Use -h for help" << endl;
		return false;
	}

	if (cmd.cmd_type == CmdOpt::getHelp) {
		return true;
	}

	if (cmd.smallest_value == -10000000000) {
		cerr << "Missing argument that specifies the smallest number in a sequence. Use -h for help." << endl;
		return false;
	}

	if (cmd.largest_value == -10000000000) {
		cerr << "Missing argument that specifies the largest number in a sequence. Use -h for help." << endl;
		return false;
	}

	if (cmd.sequence_size == 0) {
		cerr << "Missing argument that specifies number of random integers to generated. Use -h for help." << endl;
		return false;
	}

	if (cmd.device_number < 0 || cmd.device_number > 25) {
		cerr << "Invalid device number specified: " << cmd.device_number << endl;
		return false;
	}

	return true;
}

/**
 * Display usage
 */
static void display_help() {
	cout << "********************************************************************************************" << endl;
	cout << "       TectroLabs - alseqgen - AlphaRNG random sequence generator, version: ";
	cout << std::fixed << std::setw(2) << std::setprecision(1) << version << endl;
	cout << "********************************************************************************************" << endl;
	cout << "NAME" << endl;
	cout << "     alseqgen  - a utility for generating random sequence of unique integers" << endl;
	cout << "SYNOPSIS" << endl;
	cout << "     alseqgen <operation mode> <arguments> [options]" << endl;
	cout << endl;
	cout << "DESCRIPTION" << endl;
	cout << "     alseqgen generates random sequence of integers within a specific range." << endl;
	cout << endl;
	cout << "FUNCTION LETTERS" << endl;
	cout << "     Main operation mode:" << endl;
	cout << endl;
	cout << "     -g" << endl;
	cout << "           Generate random sequence." << endl;
	cout << endl;
	cout << "     -h" << endl;
	cout << "           display help." << endl;
	cout << "ARGUMENTS" << endl;
	cout << endl;
	cout << "     -s NUMBER" << endl;
	cout << "           Smallest NUMBER in a sequence." << endl;
	cout << "           Must not be smaller than -2147483647. " << endl;
	cout << endl;
	cout << "     -l NUMBER" << endl;
	cout << "           Largest NUMBER in a sequence." << endl;
	cout << "           Must not be larger than 2147483647. " << endl;
	cout << endl;
	cout << "     -n NUMBER" << endl;
	cout << "           NUMBER of random integers to generated in a sequence." << endl;
	cout << "           Must not exceed 4294967295. " << endl;
	cout << endl;
	cout << "OPTIONS" << endl;
	cout << endl;
	cout << "     -o FILE" << endl;
	cout << "           a FILE name for storing generated numbers using signed 32-bit binary format." << endl;
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
	cout << "EXAMPLES:" << endl;
	cout << "     Generating a sequence of 6 integers within [1..49] range" << endl;
	cout << "           alseqgen -g -s 1 -l 49 -n 6" << endl;
	cout << "     Extracting one number out of [1..10000] range" << endl;
	cout << "           alseqgen -g -s 1 -l 10000 -n 1" << endl;
	cout << "     Generating sequence of 100 integers within [-10000..10000] range" << endl;
	cout << "           alseqgen -g -s -10000 -l 10000 -n 100" << endl;
	cout << endl;
}
