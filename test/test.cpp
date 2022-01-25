/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cstdlib>
#include <cassert>
#include <string_view>
#include <filesystem>

#include "fmt/format.h"

#include "metacpp/meta.hpp"
#include "metacpp/ast.hpp"
#include "metacpp/refl.hpp"
#include "metacpp/serial.hpp"

#include "test.hpp"
#include "test.meta.hpp"

namespace fs = std::filesystem;

template<typename Str, typename ... Args>
[[noreturn]] void exit_with_error(Str &&fmt_str, Args &&... args){
	auto msg = fmt::vformat(std::forward<Str>(fmt_str), fmt::make_format_args(std::forward<Args>(args)...));
	fmt::print(stderr, "[ERROR] {}\n", msg);
	std::exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]){
	fs::path exe_path = fs::absolute(argv[0]);

	if(argc != 3){
		fmt::print("Usage: {} <build-dir> <test-header-path>", exe_path.filename().string());
		std::exit(EXIT_FAILURE);
	}

	fs::path build_dir = argv[1];
	fs::path header_path = argv[2];

	if(!fs::exists(build_dir)){
		exit_with_error("Build dir '{}' does not exist", build_dir.string());
	}
	else if(!fs::is_directory(build_dir)){
		exit_with_error("'{}' is not a directory", build_dir.string());
	}

	if(!fs::exists(header_path)){
		exit_with_error("Header '{}' does not exist", header_path.string());
	}
	else if(!fs::is_regular_file(header_path)){
		exit_with_error("'{}' is not a regular file", header_path.string());
	}

	auto compile_info = ast::compile_info(build_dir);
	auto info = ast::parse(header_path, compile_info);

	auto test_type = refl::reflect(meta::type_name<TestClassNS>);

	if(!test_type){
		exit_with_error("Failed to reflect '{}'", meta::type_name<TestClassNS>);
	}

	using test_info = meta::class_info<TestClassNS>;
	using test_attribs = meta::attributes<TestClass2Attribs>;

	auto test_cls = dynamic_cast<refl::class_info>(test_type);

	TestClass test_val;
	test_val.m_0 = 69;
	test_val.m_1 = 420.f;

	assert(serial::to_json(test_val) == R"({"TestClass":[{"member":{"name":"m_0","type":"int","value":"69"}},{"member":{"name":"m_1","type":"float","value":"420"}}]})");

	using namespace std::string_view_literals;

	assert(test_cls && "could not cast to refl::class_info");

	assert(test_info::name == test_type->name());
	assert(test_info::methods::size == test_cls->num_methods());

	using attrib_results = meta::query_attribs<TestClass2Attribs, "foo", "bar">;
	using method_results = meta::query_methods<TestClassNS, "test_member", void(std::string_view)>;

	static_assert(method_results::size > 0);
	static_assert(test_attribs::size == 2);
	static_assert(attrib_results::size == 1);

	using test_attrib = meta::get_t<attrib_results>;

	static_assert(test_attrib::args::size == 5);

	// 1, "2", '3', 4.0, 5.f

	static_assert(meta::get_t<test_attrib::args, 0>::value == R"(1)");
	static_assert(meta::get_t<test_attrib::args, 1>::value == R"("2")");
	static_assert(meta::get_t<test_attrib::args, 2>::value == R"('3')");
	static_assert(meta::get_t<test_attrib::args, 3>::value == R"(4.0)");
	static_assert(meta::get_t<test_attrib::args, 4>::value == R"(5.f)");

	static_assert(meta::get_value<TestEnum>("_0") == TestEnum::_0);
	static_assert(meta::get_value<TestEnum>("_1") == TestEnum::_1);
	static_assert(meta::get_value<TestEnum>("_2") == TestEnum::_2);
	static_assert(meta::get_value<TestEnum>("a") == TestEnum::a);
	static_assert(meta::get_value<TestEnum>("b") == TestEnum::b);
	static_assert(meta::get_value<TestEnum>("c") == TestEnum::c);

	return EXIT_SUCCESS;
}
