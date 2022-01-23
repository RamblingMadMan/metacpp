#include <string_view>
#include <filesystem>
#include <fstream>

#include "fmt/format.h"

#include "make_meta.hpp"

namespace fs = std::filesystem;

std::string make_class_refl(const ast::class_info &cls){
	return fmt::format(
		"template<> REFLCPP_EXPORT_SYMBOL reflpp::type_info reflpp::detail::type_export<{0}>(){{\n"
		"\t"	"struct class_info_impl: detail::info_helper_base<{0}, detail::class_info_helper>{{\n"
		"\t"	"\t"	"std::size_t num_methods() const noexcept override{{ return {1}; }}\n"
		"\t"	"\t"	"reflpp::class_method_info method(std::size_t) const noexcept override{{ return nullptr; }}\n"
		"\t"	"\t"	"std::size_t num_bases() const noexcept override{{ return {2}; }}\n"
		"\t"	"\t"	"reflpp::class_info base(std::size_t) const noexcept override{{ return nullptr; }}\n"
		"\t"	"\t"	"void *cast_to_base(void*, std::size_t i) const noexcept override{{ return nullptr; }}\n"
		"\t"	"}} static ret;\n"
		"\t"	"return &ret;\n"
		"}}\n",
		cls.name, cls.methods.size(), cls.bases.size()
	);
}

std::string make_namespace_refl(const ast::namespace_info &ns){
	std::string output;

	for(auto &&cls : ns.classes){
		output += make_class_refl(*cls.second);
		output += "\n";
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

			output_dir = argv[i];

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

	for(auto &&header : headers){
		auto info = ast::parse(header, compile_info);

		auto out_header_path = output_dir / header;
		out_header_path.replace_extension(fmt::format(".meta{}", header.extension().string()));

		auto out_source_path = output_dir / header;
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
