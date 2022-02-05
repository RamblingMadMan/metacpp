/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <string_view>
#include <filesystem>
#include <fstream>

#include "fmt/format.h"

#include "make_meta.hpp"

namespace fs = std::filesystem;

std::string make_function_refl(const ast::function_info &fn){
	std::string full_name = fn.name;
	std::string param_names_arr, param_types_arr;

	constexpr std::string_view operator_prefix = "::operator";

	if(std::string_view(full_name).substr(0, operator_prefix.size()) == operator_prefix){
		return "";
	}

	if(!fn.param_types.empty()){
		for(std::size_t i = 0; i < fn.param_types.size(); i++){
			auto &&param_name = fn.param_names[i];
			auto &&param_type = fn.param_types[i];

			param_names_arr += fmt::format(", \"{}\"", param_name);
			param_types_arr += fmt::format(", reflpp::reflect<{}>()", param_type);
		}

		param_names_arr.erase(0, 2);
		param_types_arr.erase(0, 2);

		param_names_arr = fmt::format(
			"\t"	"\t"	"const char *const param_name_arr[{}] = {{ {} }};\n"
			"\t"	"\t"	"std::string_view param_name(std::size_t idx) const noexcept override{{ return idx >= num_params() ? \"\" : param_name_arr[idx]; }}\n",
			fn.param_names.size(),
			param_names_arr
		);

		param_types_arr = fmt::format(
			"\t"	"\t"	"const reflpp::type_info param_type_arr[{}] = {{ {} }};\n"
			"\t"	"\t"	"reflpp::type_info param_type(std::size_t idx) const noexcept override{{ return idx >= num_params() ? nullptr : param_type_arr[idx]; }}\n",
			fn.param_types.size(),
			param_types_arr
		);
	}
	else{
		param_names_arr = "\t"	"\t"	"std::string_view param_name(std::size_t) const noexcept override{{ return \"\"; }}\n";
		param_types_arr = "\t"	"\t"	"reflpp::type_info param_type(std::size_t) const noexcept override{{ return nullptr; }}\n";
	}

	return fmt::format(
		"template<> REFLCPP_EXPORT_SYMBOL reflpp::function_info reflpp::detail::function_export<({0})>(){{\n"
		"\t"	"struct function_info_impl: detail::function_info_helper{{\n"
		"\t"	"\t"	"std::string_view name() const noexcept override{{ return \"{0}\"; }}\n"
		"\t"	"\t"	"const reflpp::type_info result_type_val = reflpp::reflect<{1}>();\n"
		"\t"	"\t"	"reflpp::type_info result_type() const noexcept override{{ return result_type_val; }}\n"
		"\t"	"\t"	"std::size_t num_params() const noexcept override{{ return {2}; }}\n"
						"{3}"
						"{4}"
		"\t"	"}} static ret;\n"
		"\t"	"return &ret;\n"
		"}}\n",
		full_name, fn.result_type, fn.param_types.size(),
		param_names_arr, param_types_arr
	);
}

std::string make_namespace_refl(const ast::namespace_info &ns, std::string &ctor_calls){
	std::string output;

	for(auto &&fns : ns.functions){
		for(auto &&fn : fns.second){
			output += fmt::format("{}\n", make_function_refl(*fn));
		}
	}

	for(auto &&enm : ns.enums){
		output += fmt::format(
			"template<> REFLCPP_EXPORT_SYMBOL reflpp::type_info reflpp::detail::type_export<{0}>(){{\n"
			"\t"	"static const auto ret = reflpp::detail::reflect_info<{0}>::reflect();\n"
			"\t"	"return ret;\n"
			"}}\n"
			"\n",
			enm.second->name
		);

		ctor_calls += fmt::format(
			"\t"	"reflpp::detail::type_export<{}>();\n",
			enm.second->name
		);
	}

	for(auto &&cls : ns.classes){
		if(cls.second->is_template) continue;

		output += fmt::format(
			"template<> REFLCPP_EXPORT_SYMBOL reflpp::type_info reflpp::detail::type_export<{0}>(){{\n"
			"\t"	"static const auto ret = reflpp::detail::reflect_info<{0}>::reflect();\n"
			"\t"	"return ret;\n"
			"}}\n"
			"\n",
			cls.second->name
		);

		ctor_calls += fmt::format(
			"\t"	"reflpp::detail::type_export<{}>();\n",
			cls.second->name
		);
	}

	for(auto &&inner : ns.namespaces){
		output += make_namespace_refl(*inner.second, ctor_calls);
	}

	return output;
}

void print_usage(const char *argv0, std::FILE *out = stdout){
	fmt::print(out, "Usage: {} [-o <out-dir>] <build-dir> header [other-headers ..]\n", argv0);
}

int main(int argc, char *argv[]){
	if(argc < 3){
		print_usage(argv[0], stderr);
		return EXIT_FAILURE;
	}

	fs::path output_dir = fs::path(argv[0]).parent_path();
	fs::path build_dir;
	std::vector<fs::path> headers;

	headers.reserve(argc - 2); // we know argc >= 3

	for(int i = 1; i < argc; i++){
		std::string_view arg = argv[i];

		if(arg == "-o"){
			++i;
			if(i == argc){
				print_usage(argv[0], stderr);
				return EXIT_FAILURE;
			}

			output_dir = fs::path(argv[i]);

			if(!fs::exists(output_dir)){
				if(!fs::create_directory(output_dir)){
					fmt::print(stderr, "could not create directory '{}'\n", output_dir.c_str());
					return EXIT_FAILURE;
				}
			}
			else if(!fs::is_directory(output_dir)){
				fmt::print(stderr, "'{}' is not a directory\n", output_dir.c_str());
				return EXIT_FAILURE;
			}
		}
		else if(build_dir.empty()){
			build_dir = arg;

			if(!fs::exists(build_dir)){
				fmt::print(stderr, "build directory '{}' does not exist\n", build_dir.c_str());
				return EXIT_FAILURE;
			}
			else if(!fs::is_directory(build_dir)){
				fmt::print(stderr, "'{}' is not a build directory\n", build_dir.c_str());
				return EXIT_FAILURE;
			}
		}
		else{
			fs::path header = arg;

			if(!fs::exists(header)){
				fmt::print(stderr, "header '{}' does not exist\n", header.c_str());
				return EXIT_FAILURE;
			}
			else if(!fs::is_regular_file(header)){
				fmt::print(stderr, "'{}' is not a header file\n", header.c_str());
				return EXIT_FAILURE;
			}

			headers.emplace_back(std::move(header));
		}
	}

	if(build_dir.empty()){
		fmt::print(stderr, "no build directory specified\n");
		return EXIT_FAILURE;
	}
	else if(headers.empty()){
		fmt::print(stderr, "no header files passed\n");
		return EXIT_FAILURE;
	}

	auto compile_info = ast::compile_info(build_dir);

	const auto include_dirs = compile_info.all_include_dirs();

	for(const auto &header : headers){
		const auto abs_header = fs::absolute(header).string();

		auto info = ast::parse(header, compile_info);

		auto file_output_dir = output_dir;

		for(auto &&dir : include_dirs){
			const auto abs_dir = fs::absolute(dir).string();
			const auto initial_header = std::string_view(abs_header).substr(0, abs_dir.size());

			if(initial_header == abs_dir){
				const auto abs_header_dir = fs::path(abs_header).parent_path().string();

				const auto rel_header = std::string_view(abs_header_dir).substr(abs_dir.size() + 1);

				file_output_dir = output_dir / rel_header;

				break;
			}
		}

		const auto header_file = header.filename();

		auto out_header_path = file_output_dir / header_file;

		out_header_path.replace_extension(fmt::format(".meta{}", header_file.extension().string()));

		auto out_source_path = file_output_dir / header_file;
		out_source_path += ".refl.cpp";

		std::string ctor_calls;
		auto namespace_refl = make_namespace_refl(info.global, ctor_calls);

		std::string out_source = fmt::format(
			"#define REFLCPP_IMPLEMENTATION\n"
			"#include \"{}\"\n"
			"#include \"metacpp/refl.hpp\"\n"
			"\n"
			"{}"
			"\n"
			"__attribute__((constructor))\n"
			"static void reflpp_load_type_info(){{\n"
					"{}"
			"}}",
			fs::absolute(out_header_path).string(),
			namespace_refl,
			ctor_calls
		);

		std::string out_header = fmt::format(
			"#pragma once\n"
			"\n"
			"#include \"{}\"\n"
			"#include \"metacpp/meta.hpp\"\n"
			"\n"
			"{}",
			fs::absolute(header).string(),
			make_namespace_meta(info.global)
		);

		auto out_header_dir = out_source_path.parent_path();
		if(!fs::exists(out_header_dir) && !fs::create_directories(out_header_dir)){
			fmt::print(stderr, "could not create directory '{}'\n", out_header_dir.c_str());
			return EXIT_FAILURE;
		}

		{
			std::ofstream out_source_file(out_source_path);
			if(!out_source_file ){
				fmt::print(stderr, "could not create output file '{}'\n", out_source_path.c_str());
				return EXIT_FAILURE;
			}

			out_source_file << out_source;
		}

		{
			std::ofstream out_header_file(out_header_path);
			if(!out_header_file ){
				fmt::print(stderr, "could not create output file '{}'\n", out_header_path.c_str());
				return EXIT_FAILURE;
			}

			out_header_file << out_header;
		}
	}

	return EXIT_SUCCESS;
}
