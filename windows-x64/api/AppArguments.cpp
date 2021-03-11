/**
 Copyright (C) 2014-2021 TectroLabs, https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 This class may only be used in conjunction with TectroLabs devices.

 This class implements the logic for parsing command line arguments.

*/

/**
 *    @file AppArguments.h
 *    @date 03/06/2021
 *    @Author: Andrian Belinski
 *    @version 1.0
 *
 *    @brief Parse application command line arguments
 */
#include "pch.h"
#include <AppArguments.h>

namespace alpharng {

/**
 * Load and parse command line arguments
 *
 * @param[in] argc number of arguments provided in command line
 * @param[in] argv points to the location with command line parameters
 *
 */
void AppArguments::load_arguments(const int argc, const char **argv) {
	clear_error_log();
	m_argument_map.clear();
	m_app_name = argv[0];
	for (int i = 1; i < argc; ++i) {
		string arg = argv[i];
		if (arg.rfind("-", 0) != 0) {
			m_is_error = true;
			m_error_log_oss << "Not an option: " << arg << ". " << endl;
			return;
		}

		if (m_argument_map.find(arg) != m_argument_map.end()) {
			m_is_error = true;
			m_error_log_oss << "Duplicate option: " << arg << ". " << endl;
			return;
		}

		if (m_definition_map.find(arg) == m_definition_map.end()) {
			m_is_error = true;
			m_error_log_oss << "Unexpected option: " << arg << ". " << endl;
			return;
		} else {
			ArgDef def = m_definition_map[arg];
			if (def == ArgDef::requireArgument) {
				i++;
				if (i < argc) {
					string val = argv[i];
					m_argument_map[arg] = val;
				} else {
					m_is_error = true;
					m_error_log_oss << "No value was specified for option: " << arg  << ". " << endl;
					return;
				}
			} else {
				m_argument_map[arg] = "";
			}
		}
	}
}

void AppArguments::clear_error_log() {
	m_error_log_oss.str("");
	m_error_log_oss.clear();
}

AppArguments::~AppArguments() {
}

} /* namespace alpharng */
