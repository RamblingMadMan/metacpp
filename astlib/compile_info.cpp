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

std::vector<std::string> compile_info::file_options(const std::filesystem::path &path) const{
	return impl->db.file_options(path);
}
