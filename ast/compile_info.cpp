/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "clang.hpp"

namespace fs = std::filesystem;

using namespace astpp;

struct ast::compile_info::data{
	data(const std::filesystem::path &build_dir)
		: db(build_dir){}

	clang::compilation_database db;
};

compile_info::compile_info(const fs::path &build_dir)
	: impl(std::make_unique<data>(build_dir))
{}

compile_info::~compile_info(){}

std::vector<std::string> compile_info::all_options(const std::vector<std::string_view> &add_args) const{
	return impl->db.all_options(add_args);
}

std::vector<std::string> compile_info::file_options(const std::filesystem::path &path, const std::vector<std::string_view> &add_args) const{
	return impl->db.file_options(path, add_args);
}

std::vector<std::filesystem::path> compile_info::all_include_dirs() const{
	std::vector<std::filesystem::path> ret;

	const auto options =  impl->db.all_options();

	for(std::string_view opt : options){
		if(opt.substr(0, 2) == "-I"){
			ret.emplace_back(opt.substr(2));
		}
	}

	std::sort(ret.begin(), ret.end());
	ret.erase(std::unique(ret.begin(), ret.end()), ret.end());

	return ret;
}

std::vector<std::filesystem::path> compile_info::file_include_dirs(const std::filesystem::path &path) const{
	std::vector<std::filesystem::path> ret;

	const auto options =  impl->db.file_options(path);

	for(std::string_view opt : options){
		if(opt.substr(0, 2) == "-I"){
			ret.emplace_back(opt.substr(2));
		}
	}

	return ret;
}
