/**
 Copyright (C) 2014-2025 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This program may only be used in conjunction with TectroLabs devices.

 This program is used for statistical analysis of bit value distribution within a random binary file. It is primarily
 used for testing hardware random number generators.

 */

/**
 *    @file bitgrps.cpp
 *    @date 07/05/2025
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief A utility that reads from a file with binary random bytes and runs statistical analysis of bit value distribution.
 */
#include <AppArguments.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <cstring>
#include <cstdio>
#include <algorithm>

using namespace std;
using namespace alpharng;

enum class CmdOpt : uint8_t {
	none = 0,
	getHelp = 1,
	findStickyBitGroups = 2
};

struct Command {
	CmdOpt opt;
	int opCount;
	string filePathName;
	int minStickyBitsInGroup;
	bool reportDetails;
};


/**
* Valid command line arguments
*/
AppArguments appArgs ({
	{"-sb", ArgDef::noArgument},
	{"-h", ArgDef::noArgument},
	{"-d", ArgDef::noArgument},
	{"-i", ArgDef::requireArgument},
	{"-gs", ArgDef::requireArgument}
});

/**
* Current version of this utility application
*/
static double const version = 1.0;

/**
* Local functions
*/

static bool extractCommand(Command &cmd, const int argc, const char **argv);
static bool validateComand(const Command &cmd);
static void displayHelp();
static bool findStickyBitGroups(const Command &cmd);
static long findStickyGroupsPerBitPosn(const Command &cmd, int bitIdx, const vector<uint8_t> &buffer);

/**
 * Application entry point
 *
 * @param[in] argc number of arguments provided in command line
 * @param[in] argv points to the location with command line parameters
 *
 * @return 0 if command and options extracted successfully
 */
int main(const int argc, const char **argv) {

	Command cmd;
	if (!extractCommand(cmd, argc, argv)) {
		return -1;
	}

	if (!validateComand(cmd)) {
		return -1;
	}

	bool ret = false;

	switch (cmd.opt) {
	case CmdOpt::getHelp:
		displayHelp();
		ret = true;
		break;
	case CmdOpt::findStickyBitGroups:
		ret = findStickyBitGroups(cmd);
		break;

	default:
		cerr << "Invalid command option: " << (int)cmd.opt << endl;
		ret = false;
	}

	return ret == true ? 0 : -1;
}

/**
 * Parse and extract command and options from the command line
 *
 * @param[out] cmd resulting command
 * @param[in] argc number of arguments provided in command line
 * @param[in] argv points to the location with command line parameters
 *
 * @return true if command and options extracted successfully
 */
static bool extractCommand(Command &cmd, const int argc, const char **argv) {
	appArgs.load_arguments(argc, argv);
	if (appArgs.is_error()) {
		cerr << appArgs.get_last_error();
		return false;
	}

	cmd.opt = CmdOpt::none;
	cmd.opCount = 0;
	cmd.minStickyBitsInGroup = 18;
	cmd.filePathName = "";
	cmd.reportDetails = false;

	map<string, string> argMap = appArgs.get_argument_map();

	for (auto const& map : argMap)	{
    	string option = map.first;
    	string value = map.second;

    	if (option.length() <= 1) {
    		cerr << "Invalid option: " << option << endl;
    		return false;
    	}

    	char c = option.at(1);
    	switch(c) {
		case 'h':
			cmd.opt = CmdOpt::getHelp;
			cmd.opCount++;
			break;
		case 's':
			if (option.length() == 3 && option.at(2) == 'b') {
				cmd.opt = CmdOpt::findStickyBitGroups;
				cmd.opCount++;
			}
			break;
		case 'i':
			cmd.filePathName = value;
			break;
		case 'd':
			cmd.reportDetails = true;
			break;
		case 'g':
			if (option.length() == 3 && option.at(2) == 's') {
				cmd.minStickyBitsInGroup = atoi(value.c_str());
				if (cmd.minStickyBitsInGroup < 2 || cmd.minStickyBitsInGroup > 1000) {
					cerr << "unexpected number of bytes requested for block size, must be between 2 and 1000" << endl;
					return false;
				}
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
static bool validateComand(const Command &cmd) {
	if (cmd.opCount > 1) {
		cerr << "Too many operation modes specified, choose only one" << endl;
		return false;
	}

	if (cmd.opCount == 0) {
		cerr << "No operation mode specified. Use -h for help" << endl;
		return false;
	}

	if (cmd.opt != CmdOpt::getHelp) {
		if (cmd.filePathName.length() == 0) {
			cerr << "Input file name not specified" << endl;
			return false;
		}

		ifstream isFile(cmd.filePathName.c_str(), ios::in | ios::binary);
		if(!isFile.good()) {
			cerr << "Could not open file: " << cmd.filePathName << endl;
			return false;
		}
		isFile.close();

	}

	return true;
}

/**
 * Display usage
 */
static void displayHelp() {
	cout << "*********************************************************************************" << endl;
	cout << "             TectroLabs - bitgrps - statistical bit analysis utility ver: ";
	cout << std::fixed << std::setw(2) << std::setprecision(1) << version << endl;
	cout << "*********************************************************************************" << endl;
	cout << "NAME" << endl;
	cout << "     bitgrps - A statistical analysis utility of bit value distribution within a range of random bytes" << endl;
	cout << endl;
	cout << "SYNOPSIS" << endl;
	cout << "     bitgrps <operation mode> -i <random bytes file path> [options]" << endl;
	cout << endl;
	cout << "DESCRIPTION" << endl;
	cout << "     bitgrps performs a statistical analysis of bit value distribution within a range of random bytes " << endl;
	cout << "          used for testing quality of hardware random number generators. It finds groups of " << endl;
	cout << "          consecutive sticky bits (bits with same values) for each bit position (index 0 through 7)." << endl;
	cout << endl;
	cout << "FUNCTION LETTERS" << endl;
	cout << "     Main operation mode:" << endl;
	cout << endl;
	cout << "     -h" << endl;
	cout << "           display help." << endl;
	cout << endl;
	cout << "     -sb" << endl;
	cout << "           Search for sticky bit groups for each bit position (index 0 through 7)." << endl;
	cout << endl;
	cout << "Arguments" << endl;
	cout << endl;
	cout << "     -i FILE" << endl;
	cout << "           a FILE name for reading random bytes retrieved from a HWRNG." << endl;
	cout << endl;
	cout << "OPTIONS" << endl;
	cout << endl;
	cout << "     -gs NUMBER" << endl;
	cout << "           Minimum NUMBER of consecutive sticky bits to find in a group, must be between 2 and 1000" << endl;
	cout << "           Skip this option for using default 18 bit groups." << endl;
	cout << endl;
	cout << "     -d" << endl;
	cout << "           Include detail information about groups." << endl;
	cout << "           Skip this option for hiding details." << endl;
	cout << endl;
	cout << "EXAMPLES:" << endl;
	cout << "     To find groups of sticky bits with at least 10 consecutive sticky 0 or 1 values in rnd.bin file:" << endl;
	cout << "           bitgrps -sb -i rnd.bin -gs 10" << endl;
	cout << endl;
}

/**
 * Run sticky bit analysis - find sticky bit value groups per each bit position.
 *
 * @param cmd command to be used
 *
 * @return true for success
 */
static bool findStickyBitGroups(const Command &cmd) {

	ifstream isFile(cmd.filePathName.c_str(), ios::in | ios::binary);
	if (!isFile.is_open()) {
		cerr << "The file " + cmd.filePathName + " could not be opened" << endl;
		return false;
	}

	isFile.seekg(0, std::ios::end);
	long fileSize = isFile.tellg();
	isFile.seekg(0, ios_base::beg);

	vector<uint8_t> buffer(fileSize);

	isFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);
	if (!isFile.good()) {
		cerr << "The file " + cmd.filePathName + " read error" << endl;
		return false;
	}
	isFile.close();

	cout << endl << endl;
	cout << "Start scanning ... " << endl << endl;

	long totalGroupsFound {0};

	for (int bitIdx = 0; bitIdx < 8; ++bitIdx) {
		totalGroupsFound += findStickyGroupsPerBitPosn(cmd, bitIdx, buffer);
	}

	cout << "....................................................." << endl;

	cout << " Total sticky bit groups found: " << totalGroupsFound << endl << endl;

	return true;
}

/**
 * Find sticky bit value groups per bit position.
 *
 * @param cmd command to be used
 * @param bitIdx bit position
 * @param buffer binary data
 *
 * @return total sticky bit groups found for specific bit position
 */
static long findStickyGroupsPerBitPosn(const Command &cmd, int bitIdx, const vector<uint8_t> &buffer) {
	multimap<long, long, greater<long>> mapGroupsVal1;
	multimap<long, long, greater<long>> mapGroupsVal0;

	long buffSize = buffer.size();

	int minGroupSize = cmd.minStickyBitsInGroup;
	int curGroupSizeBitO {0};
	int curGroupSizeBit1 {0};
	int curGroupBitOStartIdx {0};
	int curGroupBit1StartIdx {0};
	uint8_t bitMask = 1 << bitIdx;

	for (long idx = 0; idx < buffSize; ++idx) {
		uint8_t val = buffer[idx] & bitMask;
		if (val) {
			if (curGroupSizeBitO >= minGroupSize) {
				// Save the group for value 0
				mapGroupsVal0.insert({curGroupSizeBitO, curGroupBitOStartIdx});
			}
			curGroupSizeBitO = 0;
			curGroupSizeBit1++;
			if (curGroupSizeBit1 == 1) {
				curGroupBit1StartIdx = idx;
			}
		} else {
			if (curGroupSizeBit1 >= minGroupSize) {
				// Save the group for value 1
				mapGroupsVal1.insert({curGroupSizeBit1, curGroupBit1StartIdx});
			}
			curGroupSizeBit1 = 0;
			curGroupSizeBitO++;
			if (curGroupSizeBitO == 1) {
				curGroupBitOStartIdx = idx;
			}

		}
	}

	// Process last groups if any
	if (curGroupSizeBit1 >= minGroupSize) {
		mapGroupsVal1.insert({curGroupSizeBit1, curGroupBit1StartIdx});
	}

	if (curGroupSizeBitO >= minGroupSize) {
		mapGroupsVal0.insert({curGroupSizeBitO, curGroupBitOStartIdx});
	}

	if (mapGroupsVal1.size() > 0 || mapGroupsVal0.size() > 0) {
		cout << endl << "-------- Bit position " << bitIdx << " --------" << endl << endl;
		if (cmd.reportDetails) {
			for (const auto& pair : mapGroupsVal0) {
				cout << "\t\t sticky 0 count: " << pair.first << "\t file offset: " << pair.second << endl;
			}
		}
		if (mapGroupsVal0.size() > 0) {
			cout << "\t Total groups found for sticky 0: " << mapGroupsVal0.size() << endl << endl;
		}
		if (cmd.reportDetails) {
			for (const auto& pair : mapGroupsVal1) {
				cout << "\t\t sticky 1 count: " << pair.first << "\t file offset: " << pair.second << endl;
			}
		}
		if (mapGroupsVal1.size() > 0) {
			cout << "\t Total groups found for sticky 1: " << mapGroupsVal1.size() << endl;
		}
	}

	return mapGroupsVal1.size() + mapGroupsVal0.size();
}

