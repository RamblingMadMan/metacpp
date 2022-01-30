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

std::string make_method_refl(const ast::class_method_info &method, std::size_t idx){
	std::string names_arr, types_arr;

	if(!method.param_names.empty()){
		for(std::size_t i = 0; i < method.param_names.size(); i++){
			auto &&name = method.param_names[i];
			auto &&type = method.param_types[i];
			names_arr += fmt::format(", \"{}\"", name);
			types_arr += fmt::format(", refl::reflect<{}>()", type);
		}

		names_arr.erase(0, 2);
		types_arr.erase(0, 2);
	}

	return fmt::format(
		"\t"	"struct method_info_impl{0}: reflpp::detail::class_method_helper{{\n"
		"\t"	"\t"	"reflpp::type_info result_type() const noexcept override{{ static auto ret = reflpp::reflect<{1}>(); return ret; }}\n"
		"\t"	"\t"	"std::size_t num_params() const noexcept override{{ return {2}; }}\n"
		"\t"	"\t"	"std::string_view param_name(std::size_t idx) const noexcept override{{\n"
		"\t"	"\t"	"\t"	"constexpr std::string_view arr[] = {{ {3} }};\n"
		"\t"	"\t"	"\t"	"return idx >= num_params() ? \"\" : arr[idx];\n"
		"\t"	"\t"	"}}\n"
		"\t"	"\t"	"reflpp::type_info param_type(std::size_t idx) const noexcept override{{\n"
		"\t"	"\t"	"\t"	"static const reflpp::type_info arr[] = {{ {4} }};\n"
		"\t"	"\t"	"\t"	"return idx >= num_params() ? nullptr : arr[idx];\n"
		"\t"	"\t"	"}}\n"
		"\t"	"}} static param{0};\n",
		idx, method.result_type, method.param_names.size(),
		names_arr, types_arr
	);
}

std::string make_class_refl(const ast::class_info &cls){
	if(!cls.template_params.empty()) return "";

	std::string full_name = cls.name;

	if(!cls.template_args.empty()){
		std::string tmpl_args;

		for(auto &&tmpl_arg : cls.template_args){
			tmpl_args += fmt::format(", {}", tmpl_arg);
		}

		tmpl_args.erase(0, 2);

		full_name += fmt::format("<{}>", tmpl_args);
	}

	std::string methods_output;
	std::string bases_arr, methods_arr;

	if(!cls.bases.empty()){
		for(auto &&base : cls.bases){
			bases_arr += fmt::format(", reflpp::reflect<{}>()", base.name);
		}

		bases_arr.erase(0, 2);
	}

	if(!cls.methods.empty()){
		std::size_t method_idx = 0;
		for(auto &&methods : cls.methods){
			for(auto &&method : methods.second){
				methods_arr += fmt::format(", &param{}", method_idx);
				methods_output += make_method_refl(*method, method_idx++);
				methods_output += "\n";
			}
		}

		methods_arr.erase(0, 2);
	}

	return fmt::format(
		"template<> REFLCPP_EXPORT_SYMBOL reflpp::type_info reflpp::detail::type_export<{0}>(){{\n"
		"{5}"
		"\t"	"struct class_info_impl: detail::info_helper_base<{0}, detail::class_info_helper>{{\n"
		"\t"	"\t"	"std::size_t num_methods() const noexcept override{{ return {1}; }}\n"
		"\t"	"\t"	"reflpp::class_method_info method(std::size_t idx) const noexcept override{{\n"
		"\t"	"\t"	"\t"	"static const reflpp::class_method_info arr[] = {{ {4} }};\n"
		"\t"	"\t"	"\t"	"return idx >= num_methods() ? nullptr : arr[idx];\n"
		"\t"	"\t"	"}}\n"
		"\t"	"\t"	"std::size_t num_bases() const noexcept override{{ return {2}; }}\n"
		"\t"	"\t"	"reflpp::class_info base(std::size_t idx) const noexcept override{{\n"
		"\t"	"\t"	"\t"	"reflpp::class_info arr[] = {{ {3} }};\n"
		"\t"	"\t"	"\t"	"return idx >= num_bases() ? nullptr : arr[idx];\n"
		"\t"	"\t"	"}}\n"
		"\t"	"\t"	"void *cast_to_base(void*, std::size_t i) const noexcept override{{ return nullptr; }}\n"
		"\t"	"}} static ret;\n"
		"\t"	"return &ret;\n"
		"}}\n",
		full_name, cls.methods.size(), cls.bases.size(), bases_arr, methods_arr, methods_output
	);
}

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
	}

	return fmt::format(
		"template<> REFLCPP_EXPORT_SYMBOL reflpp::function_info reflpp::detail::function_export<({0})>(){{\n"
		"\t"	"struct function_info_impl: detail::function_info_helper{{\n"
		"\t"	"\t"	"std::string_view name() const noexcept override{{ return \"{0}\"; }}\n"
		"\t"	"\t"	"reflpp::type_info result_type() const noexcept override{{ return reflpp::reflect<{1}>(); }}\n"
		"\t"	"\t"	"std::size_t num_params() const noexcept override{{ return {2}; }}\n"
		"\t"	"\t"	"std::string_view param_name(std::size_t idx) const noexcept override{{\n"
		"\t"	"\t"	"\t"	"constexpr std::string_view arr[] = {{ {3} }};\n"
		"\t"	"\t"	"\t"	"return idx >= num_params() ? \"\" : arr[idx];\n"
		"\t"	"\t"	"}}\n"
		"\t"	"\t"	"reflpp::type_info param_type(std::size_t idx) const noexcept override{{\n"
		"\t"	"\t"	"\t"	"static const reflpp::type_info arr[] = {{ {4} }};\n"
		"\t"	"\t"	"\t"	"return idx >= num_params() ? nullptr : arr[idx];\n"
		"\t"	"\t"	"}}\n"
		"\t"	"}} static ret;\n"
		"\t"	"return &ret;\n"
		"}}\n",
		full_name, fn.result_type, fn.param_types.size(),
		param_names_arr, param_types_arr
	);
}

std::string make_namespace_refl(const ast::namespace_info &ns){
	std::string output;

	for(auto &&fns : ns.functions){
		for(auto &&fn : fns.second){
			output += fmt::format("{}\n", make_function_refl(*fn));
		}
	}

	for(auto &&cls : ns.classes){
		output += fmt::format("{}\n", make_class_refl(*cls.second));
	}

	for(auto &&inner : ns.namespaces){
		output += make_namespace_refl(*inner.second);
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

		std::string out_source = fmt::format(
			"#define REFLCPP_IMPLEMENTATION\n"
			"#include \"{}\"\n"
			"#include \"metacpp/refl.hpp\"\n"
			"\n"
			"{}",
			fs::absolute(header).string(),
			make_namespace_refl(info.global)
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
		if(!fs::exists(out_header_dir) && !fs::create_directory(out_header_dir)){
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
