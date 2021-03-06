/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the logic for parsing command line arguments.

*/

/**
 *    @file AppArguments.h
 *    @date 05/06/2020
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Parse application command line arguments
 */
#ifndef API_INC_APPARGUMENTS_H_
#define API_INC_APPARGUMENTS_H_

#include <string>
#include <map>
#include <sstream>

using namespace std;

namespace alpharng {

enum class ArgDef {noArgument, requiredArgument};

class AppArguments {

public:
	AppArguments(map<string, ArgDef> definitions) : m_definition_map(definitions), m_is_error(false) {}
	void load_arguments(int argc, char **argv);
	string get_last_error() {return m_error_log_oss.str();}
	map<string, string> & get_argument_map() {return m_argument_map;}
	bool is_error() {return m_is_error;}
	virtual ~AppArguments();

private:
	void clear_error_log();

private:
	map<string, ArgDef> m_definition_map;
	map<string, string> m_argument_map;
	string m_app_name;
	ostringstream m_error_log_oss;
	bool m_is_error;
};

} /* namespace securerng */

#endif /* API_INC_APPARGUMENTS_H_ */
