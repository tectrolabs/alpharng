/**
 Copyright (C) 2014-2022 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the logic for parsing command line arguments.

*/

/**
 *    @file AppArguments.h
 *    @date 08/27/2022
 *    @Author: Andrian Belinski
 *    @version 1.2
 *
 *    @brief Parse application command line arguments
 */
#ifndef ALPHARNG_API_INC_APPARGUMENTS_H_
#define ALPHARNG_API_INC_APPARGUMENTS_H_

#include <string>
#include <map>
#include <sstream>

using namespace std;

namespace alpharng {

enum class ArgDef {noArgument, requireArgument};

class AppArguments {

public:
	explicit AppArguments(const map<string, ArgDef> &definitions) : m_definition_map(definitions), m_is_error(false) {}
	void load_arguments(const int argc, const char **argv);
	const string get_last_error() const {return m_error_log_oss.str();}
	map<string, string> & get_argument_map() {return m_argument_map;}
	map<string, ArgDef>& get_definition_map() { return m_definition_map; }
	bool is_error() const {return m_is_error;}
	string& get_app_name() { return m_app_name; }
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

} /* namespace alpharng */

#endif /* ALPHARNG_API_INC_APPARGUMENTS_H_ */
