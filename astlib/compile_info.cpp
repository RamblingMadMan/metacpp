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
