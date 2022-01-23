/*
 *	Meta C++ Tool and Library
 *	Copyright (C) 2022  Keith Hammond
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation; either
 *	version 2.1 of the License, or (at your option) any later version.
 *
 *	This library is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	Lesser General Public License for more details.
 */

#include <cstdlib>
#include <cassert>
#include <string_view>
#include <filesystem>

#include "fmt/format.h"

#include "metacpp/meta.hpp"
#include "metacpp/ast.hpp"
#include "metacpp/refl.hpp"

#include "test.hpp"
#include "test.meta.hpp"

namespace fs = std::filesystem;

template<typename Str, typename ... Args>
[[noreturn]] void exit_with_error(Str &&fmt_str, Args &&... args){
	auto msg = fmt::vformat(std::forward<Str>(fmt_str), fmt::make_format_args(std::forward<Args>(args)...));
	fmt::print(stderr, "[ERROR] {}\n", msg);
	std::exit(EXIT_FAILURE);
}

void print_namespace(const int depth, const ast::namespace_info *ns){
	auto pad = fmt::format("{:>{}}", "", (depth+1) * 4);
	auto inner_pad = fmt::format("{:>{}}", "", (depth+2) * 4);

	fmt::print("{:>{}}(Namespace {}\n", "", depth * 4, ns->name.empty() ? "[Global]" : ns->name);

	for(auto &&cls : ns->classes){
		fmt::print("{}(Class '{}'\n", pad, cls.second->name);
		for(auto &&attrib : cls.second->attributes){
			fmt::print("{}(Attribute '{}')\n", inner_pad, attrib.str());
		}
		fmt::print("{})\n", pad);
	}

	for(auto &&fns : ns->functions){
		fmt::print("{}(Function '{}'\n", pad, fns.first);

		int counter = 0;
		for(auto &&fn : fns.second){
			fmt::print("{}(Candidate {})\n", inner_pad, counter++);
		}

		fmt::print("{})\n", pad);
	}

	for(auto &&alias : ns->aliases){
		fmt::print("{}(TypeAlias {} {})\n", pad, alias.second->name, alias.second->aliased);
	}

	for(auto &&inner_ns : ns->namespaces){
		print_namespace(depth+1, inner_ns.second);
	}

	fmt::print("{:>{}})\n", "", depth * 4);
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

	/*
	print_namespace(0, &info.global);

	auto test_type = refl::reflect(meta::type_name<TestClassNS>);

	if(!test_type){
		exit_with_error("Failed to reflect '{}'", meta::type_name<TestClassNS>);
	}

	using test_info = meta::class_info<TestClassNS>;
	auto test_cls = dynamic_cast<refl::class_info>(test_type);

	using namespace std::string_view_literals;

	assert(test_cls && "could not cast to refl::class_info");

	assert(test_info::name == test_type->name());
	assert(test_info::num_methods == test_cls->num_methods());
	//static_assert(test_info::query_method<"test_member">::size > 0);
	*/

	return EXIT_SUCCESS;
}
