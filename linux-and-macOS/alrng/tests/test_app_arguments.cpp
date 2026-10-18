/**
 Copyright (C) 2014-2026 TectroLabs L.L.C. https://tectrolabs.com

 THIS SOFTWARE IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
 INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.

 Unit tests for the AppArguments class, which parses the command line options of the utilities.
 */

/**
 *    @file test_app_arguments.cpp
 *
 *    @brief Option parsing and error reporting tests for AppArguments.
 */
#include <AppArguments.h>
#include "test_framework.h"

using namespace alpharng;

static std::map<std::string, ArgDef> definitions() {
	return {
		{"-o", ArgDef::requireArgument},
		{"-n", ArgDef::requireArgument},
		{"-l", ArgDef::noArgument},
		{"-e", ArgDef::noArgument},
	};
}

TEST(args_no_arguments_is_not_an_error) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng"};
	args.load_arguments(1, argv);
	CHECK(!args.is_error());
	CHECK(args.get_argument_map().empty());
	CHECK_EQ(args.get_app_name(), "alrng");
}

TEST(args_option_with_value) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng", "-o", "rnd.bin"};
	args.load_arguments(3, argv);
	REQUIRE(!args.is_error());
	CHECK_EQ(args.get_argument_map().size(), (size_t)1);
	CHECK_EQ(args.get_argument_map()["-o"], "rnd.bin");
}

TEST(args_option_without_value) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng", "-l"};
	args.load_arguments(2, argv);
	REQUIRE(!args.is_error());
	CHECK(args.get_argument_map().find("-l") != args.get_argument_map().end());
	CHECK_EQ(args.get_argument_map()["-l"], "");
}

TEST(args_several_options_in_any_order) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng", "-e", "-n", "1000000", "-o", "rnd.bin"};
	args.load_arguments(6, argv);
	REQUIRE(!args.is_error());
	CHECK_EQ(args.get_argument_map().size(), (size_t)3);
	CHECK_EQ(args.get_argument_map()["-n"], "1000000");
	CHECK_EQ(args.get_argument_map()["-o"], "rnd.bin");
}

TEST(args_a_value_may_start_with_a_dash) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng", "-o", "-weird-name"};
	args.load_arguments(3, argv);
	REQUIRE(!args.is_error());
	CHECK_EQ(args.get_argument_map()["-o"], "-weird-name");
}

TEST(args_unknown_option_is_an_error) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng", "-x"};
	args.load_arguments(2, argv);
	CHECK(args.is_error());
	CHECK(args.get_last_error().find("Unexpected option: -x") != std::string::npos);
}

TEST(args_missing_value_is_an_error) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng", "-o"};
	args.load_arguments(2, argv);
	CHECK(args.is_error());
	CHECK(args.get_last_error().find("No value was specified for option: -o") != std::string::npos);
}

TEST(args_duplicate_option_is_an_error) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng", "-l", "-l"};
	args.load_arguments(3, argv);
	CHECK(args.is_error());
	CHECK(args.get_last_error().find("Duplicate option: -l") != std::string::npos);
}

TEST(args_text_without_a_dash_is_an_error) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng", "rnd.bin"};
	args.load_arguments(2, argv);
	CHECK(args.is_error());
	CHECK(args.get_last_error().find("Not an option: rnd.bin") != std::string::npos);
}

TEST(args_a_value_given_to_a_flag_is_an_error) {
	AppArguments args(definitions());
	const char *argv[] = {"alrng", "-l", "extra"};
	args.load_arguments(3, argv);
	CHECK(args.is_error());
}

TEST(args_error_state_is_cleared_when_new_arguments_are_loaded) {
	AppArguments args(definitions());
	const char *bad[] = {"alrng", "-x"};
	args.load_arguments(2, bad);
	REQUIRE(args.is_error());

	const char *good[] = {"alrng", "-l"};
	args.load_arguments(2, good);
	CHECK(!args.is_error());
}
