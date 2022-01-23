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

std::vector<std::string> compile_info::file_options(const std::filesystem::path &path) const{
	return impl->db.file_options(path);
}
