/**
 Copyright (C) 2014-2023 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the logic for parsing command line arguments.

*/

/**
 *    @file AppArguments.h
 *    @date 09/16/2023
 *    @Author: Andrian Belinski
 *    @version 1.3
 *
 *    @brief Parse application command line arguments
 */
#ifndef ALPHARNG_API_INC_APPARGUMENTS_H_
#define ALPHARNG_API_INC_APPARGUMENTS_H_

#include <string>
#include <map>
#include <sstream>

namespace alpharng {

enum class ArgDef {noArgument, requireArgument};

class AppArguments {

public:
	explicit AppArguments(const std::map<std::string, ArgDef> &definitions) : m_definition_map(definitions) {}
	void load_arguments(const int argc, const char **argv);
	std::string get_last_error() const {return m_error_log_oss.str();}
	std::map<std::string, std::string> & get_argument_map() {return m_argument_map;}
	std::map<std::string, ArgDef>& get_definition_map() { return m_definition_map; }
	bool is_error() const {return m_is_error;}
	std::string& get_app_name() { return m_app_name; }
	virtual ~AppArguments() = default;

private:
	void clear_error_log();

private:
	std::map<std::string, ArgDef> m_definition_map;
	std::map<std::string, std::string> m_argument_map;
	std::string m_app_name;
	std::ostringstream m_error_log_oss;
	bool m_is_error {false};
};

} /* namespace alpharng */

#endif /* ALPHARNG_API_INC_APPARGUMENTS_H_ */
