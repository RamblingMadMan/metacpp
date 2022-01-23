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

#include <functional>
#include <iostream>
#include <optional>

#include "fmt/format.h"

#include "metacpp/ast.hpp"
#include "metacpp/meta.hpp"

#include "clang.hpp"

namespace fs = std::filesystem;

namespace astpp::detail{
	template<typename Str, typename ... Args>
	void print_parse_error(const fs::path &path, Str &&fmt_str, Args &&... fmt_args){
		auto msg = fmt::vformat(std::forward<Str>(fmt_str), fmt::make_format_args(std::forward<Args>(fmt_args)...));
		fmt::print(stderr, "[ERROR] {}: {}\n", path.string(), msg);
	}

	template<typename Str, typename ... Args>
	void print_parse_warning(const fs::path &path, Str &&fmt_str, Args &&... fmt_args){
		auto msg = fmt::vformat(std::forward<Str>(fmt_str), fmt::make_format_args(std::forward<Args>(fmt_args)...));
		fmt::print(stderr, "[WARNING] {}: {}\n", path.string(), msg);
	}

	template<typename Str, typename ... Args>
	void print_parse_info(const fs::path &path, Str &&fmt_str, Args &&... fmt_args){
		auto msg = fmt::vformat(std::forward<Str>(fmt_str), fmt::make_format_args(std::forward<Args>(fmt_args)...));
		fmt::print(stdout, "[INFO] {}: {}\n", path.string(), msg);
	}

	template<typename T>
	T *store_info(info_map &info, T &&ent){
		auto ptr = std::make_unique<T>(std::move(std::forward<T>(ent)));
		auto ret = ptr.get();
		info.storage.emplace_back(std::move(ptr));
		return ret;
	}

	std::optional<entity> try_parse(const fs::path &path, info_map &infos, clang::cursor c);

	std::vector<std::string> parse_attrib_args(const fs::path &path, clang::token_iterator &it, clang::token_iterator end){
		std::vector<std::string> ret;

		int depth = 1;
		std::string arg_str;
		while(it != end){
			auto tok_str = it->str();

			if(tok_str == "("){
				++depth;
			}
			else if(tok_str == ")"){
				--depth;
				if(depth == 0){
					++it;
					break;
				}
			}
			else if(tok_str == "," && depth == 1){
				ret.emplace_back(std::move(arg_str));
				arg_str = "";
			}
			else{
				arg_str += tok_str;
			}

			++it;
		}

		if(depth != 0){
			print_parse_error(path, "could not find end of attribute argument list");
		}

		return ret;
	}

	std::optional<attribute> parse_attrib(const fs::path &path, clang::token_iterator &it, clang::token_iterator end){
		if(it->kind() != CXToken_Identifier){
			print_parse_error(path, "bad attribute, expected [scope::]name");
			return std::nullopt;
		}

		std::string name = it->str();

		++it;

		if(it == end){
			return std::make_optional<attribute>(std::move(name));
		}

		std::string scope = it->str();

		if(scope == "::"){
			// first string was scope

			++it;

			if(it->kind() != CXToken_Identifier){
				print_parse_error(path, "bad attribute, expected [scope::]name");
				return std::nullopt;
			}

			scope = std::move(name);
			name = it->str();

			++it;
		}
		else{
			scope = "";
		}

		std::vector<std::string> args;

		if(it != end){
			if(it->str() == "("){
				args = parse_attrib_args(path, ++it, end);
			}

			if(it != end){
				if(it->str() == ","){
					++it;
				}
				else{
					print_parse_warning(path, "bad attribute, ignoring all tokens after '{}'", name);
					it = end;
				}
			}
		}

		return std::make_optional<attribute>(std::move(scope), std::move(name), std::move(args));
	}

	std::vector<attribute> parse_attribs(const fs::path &path, info_map &infos, clang::token_iterator begin, clang::token_iterator end){
		if(begin->str() != "["){
			return {};
		}

		++begin;

		if(begin->str() != "["){
			return {};
		}

		++begin;

		clang::token_iterator new_end_it;

		for(auto it = begin; it != end; ++it){
			if(it->str() == "]"){
				if(new_end_it.is_valid()){
					break;
				}
				else{
					new_end_it = it;
				}
			}
			else{
				new_end_it = clang::token_iterator{};
			}
		}

		if(!new_end_it.is_valid()){
			print_parse_error(path, "could not find end of attribute list");
			return {};
		}

		end = new_end_it;

		std::vector<attribute> attribs;

		auto it = begin;

		while(it != end){
			auto attr = parse_attrib(path, it, end);
			if(attr){
				attribs.emplace_back(std::move(*attr));
			}
		}

		return attribs;
	}

	std::vector<attribute> parse_class_attribs(const fs::path &path, info_map &infos, clang::cursor decl){
		auto toks = decl.tokens();

		auto toks_begin = toks.begin();

		++toks_begin; // skip class/struct

		return parse_attribs(path, infos, toks_begin, toks.end());
	}

	std::optional<class_constructor_info> parse_class_ctor(const fs::path &path, info_map &infos, clang::cursor c){
		if(c.kind() != CXCursor_Constructor){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXPublic){
			return std::nullopt;
		}

		class_constructor_info ret;

		if(clang_CXXConstructor_isMoveConstructor(c)){
			ret.constructor_kind = constructor_kind::move;
		}
		else if(clang_CXXConstructor_isCopyConstructor(c)){
			ret.constructor_kind = constructor_kind::copy;
		}
		else if(clang_CXXConstructor_isDefaultConstructor(c)){
			ret.constructor_kind = constructor_kind::default_;
		}
		else if(clang_CXXConstructor_isConvertingConstructor(c)){
			ret.constructor_kind = constructor_kind::converting;
		}
		else{
			ret.constructor_kind = constructor_kind::generic;
		}

		int num_params = clang_Cursor_getNumArguments(c);

		if(num_params > 0){
			ret.param_names.reserve(num_params);
			ret.param_types.reserve(num_params);

			for(int i = 0; i < num_params; i++){
				auto arg_cursor = clang_Cursor_getArgument(c, i);
				auto arg_type = clang_getCursorType(arg_cursor);

				auto arg_name = clang::detail::convert_str(clang_getCursorSpelling(arg_cursor));
				auto arg_type_name = clang::detail::convert_str(clang_getTypeSpelling(arg_type));

				ret.param_names.emplace_back(std::move(arg_name));
				ret.param_types.emplace_back(std::move(arg_type_name));
			}
		}

		return ret;
	}

	std::optional<class_destructor_info> parse_class_dtor(const fs::path &path, info_map &infos, clang::cursor c){
		if(c.kind() != CXCursor_Destructor){
			return std::nullopt;
		}

		class_destructor_info ret;

		return ret;
	}

	std::optional<class_member_info> parse_class_member(const fs::path &path, info_map &infos, clang::cursor c){
		if(c.kind() != CXCursor_VarDecl){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXPublic){
			return std::nullopt;
		}

		class_member_info ret;

		ret.name = c.spelling();
		ret.type = clang::detail::convert_str(clang_getTypeSpelling(clang_getCursorType(c)));

		return ret;
	}

	std::optional<class_method_info> parse_class_method(const fs::path &path, info_map &infos, clang::cursor c){
		if(c.kind() != CXCursor_CXXMethod){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXPublic){
			return std::nullopt;
		}

		class_method_info ret;

		ret.name = c.spelling();

		ret.is_static = clang_CXXMethod_isStatic(c);
		ret.is_const = clang_CXXMethod_isConst(c);
		ret.is_virtual = clang_CXXMethod_isVirtual(c);
		ret.is_pure_virtual = clang_CXXMethod_isPureVirtual(c);
		ret.is_defaulted = clang_CXXMethod_isDefaulted(c);

		auto fn_type = clang_getCursorType(c);

		ret.is_noexcept =
			clang_getExceptionSpecificationType(fn_type) == CXCursor_ExceptionSpecificationKind_BasicNoexcept;

		auto fn_type_str = clang::detail::convert_str(clang_getTypeSpelling(fn_type));

		{
			auto result_type = clang_getResultType(fn_type);
			auto result_type_str = clang::detail::convert_str(clang_getTypeSpelling(result_type));
			ret.result_type = std::move(result_type_str);
		}

		const int num_params = clang_Cursor_getNumArguments(c);

		if(num_params > 0){
			ret.param_names.reserve(num_params);
			ret.param_types.reserve(num_params);

			for(int i = 0; i < num_params; i++){
				auto param_cursor = clang_Cursor_getArgument(c, i);
				auto param_name = clang::detail::convert_str(clang_getCursorSpelling(param_cursor));
				auto param_type = clang::detail::convert_str(clang_getTypeSpelling(clang_getCursorType(param_cursor)));

				ret.param_names.emplace_back(std::move(param_name));
				ret.param_types.emplace_back(std::move(param_type));
			}
		}

		return ret;
	}

	std::optional<class_info> parse_class_decl(const fs::path &path, info_map &infos, clang::cursor c){
		if(c.kind() != CXCursor_ClassDecl){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXInvalidAccessSpecifier && access != CX_CXXPublic){
			return std::nullopt;
		}

		auto def_cursor = clang_getCursorDefinition(c);
		if(!clang_Cursor_isNull(def_cursor)){
			c = def_cursor;
		}

		auto class_name = c.spelling();

		if(class_name == "__forced_unwind"){
			return std::nullopt; // skip this unwanted class
		}

		class_info ret;

		auto toks = c.tokens();

		auto toks_begin = toks.begin();

		++toks_begin; // skip class/struct keyword

		ret.attributes = parse_attribs(path, infos, toks_begin, toks.end());

		ret.name = c.spelling();
		ret.is_abstract = clang_CXXRecord_isAbstract(c);

		if(clang_Cursor_isNull(def_cursor)){
			return ret;
		}

		auto store_info = [&infos](auto &info){
			auto ptr = std::make_unique<std::remove_reference_t<decltype(info)>>(std::move(info));
			auto ret = ptr.get();
			infos.storage.emplace_back(std::move(ptr));
			return ret;
		};

		std::size_t ctor_idx = 0, member_idx = 0, method_idx = 0;

		c.visit_children([&](clang::cursor inner, clang::cursor){
			// TODO: check 'ptr' in each branch

			if(auto class_decl = detail::parse_class_decl(path, infos, inner); class_decl){
				auto ptr = store_info(*class_decl);
				ret.classes[ptr->name] = ptr;
			}
			else if(auto ctor = detail::parse_class_ctor(path, infos, inner); ctor){
				auto ptr = store_info(*ctor);
				ret.ctors.emplace_back(ptr);
			}
			else if(auto dtor = detail::parse_class_dtor(path, infos, inner); dtor){
				auto ptr = store_info(*dtor);
				ret.dtor = ptr;
			}
			else if(auto method = detail::parse_class_method(path, infos, inner); method){
				auto ptr = store_info(*method);
				ret.methods[ptr->name].emplace_back(ptr);
			}
			else if(auto member = detail::parse_class_member(path, infos, inner); member){
				auto ptr = store_info(*member);
				ret.members[ptr->name] = ptr;
			}

			// TODO: handle access kinds

			// skip anything else
		});

		return ret;
	}

	std::optional<function_info> parse_function_decl(const fs::path &path, info_map &infos, clang::cursor c){
		if(c.kind() != CXCursor_FunctionDecl){
			return std::nullopt;
		}

		auto def_cursor = clang_getCursorDefinition(c);
		if(!clang_Cursor_isNull(def_cursor)){
			c = def_cursor;
		}

		function_info ret;

		ret.name = c.spelling();

		return ret;
	}

	entity_info *parse_namespace_inner(const fs::path &path, info_map &infos, namespace_info &ns, clang::cursor c){
		auto res = try_parse(path, infos, c);
		if(!res) return nullptr;

		entity_info *ret = nullptr;

		auto store_info = [&infos](auto &info){
			auto ptr = std::make_unique<std::remove_reference_t<decltype(info)>>(std::move(info));
			auto ret = ptr.get();
			infos.storage.emplace_back(std::move(ptr));
			return ret;
		};

		std::visit(
			meta::overload(
				[&](function_info &fn_info){
					auto ptr = store_info(fn_info);
					ret = ptr;
					ns.functions[ptr->name].emplace_back(ptr);
				},
				[&](class_info &cls){
					auto ptr = store_info(cls);
					ret = ptr;
					ns.classes[ptr->name] = ptr;
				},
				[&](namespace_info &child_ns){
					auto ptr = store_info(child_ns);
					ret = ptr;
					ns.namespaces[ptr->name] = ptr;
				},
				[&](type_alias_info &alias){
					auto ptr = store_info(alias);
					ret = ptr;
					ns.aliases[ptr->name] = ptr;
				}
			),
			 *res
		);

		return ret;
	}

	std::optional<type_alias_info> parse_type_alias(const fs::path &path, info_map &infos, clang::cursor c){
		if(c.kind() != CXCursor_TypeAliasDecl && c.kind() != CXCursor_TypedefDecl){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXInvalidAccessSpecifier && access != CX_CXXPublic){
			return std::nullopt;
		}

		type_alias_info ret;

		ret.name = c.spelling();

		auto type = clang_getTypedefDeclUnderlyingType(c);

		ret.aliased = clang::detail::convert_str(clang_getTypeSpelling(type));

		return ret;
	}

	std::optional<namespace_info> parse_namespace(const fs::path &path, info_map &infos, clang::cursor c){
		if(c.kind() != CXCursor_Namespace){
			return std::nullopt;
		}

		namespace_info ret;

		ret.name = c.spelling();

		c.visit_children(
			[&](clang::cursor child, clang::cursor){
				parse_namespace_inner(path, infos, ret, child);
			}
		);

		return ret;
	}

	std::optional<entity> try_parse(const fs::path &path, info_map &infos, clang::cursor c){
		if(auto class_decl = detail::parse_class_decl(path, infos, c); class_decl){
			return std::make_optional<entity>(std::move(*class_decl));
		}
		else if(auto fn_decl = detail::parse_function_decl(path, infos, c); fn_decl){
			return std::make_optional<entity>(std::move(*fn_decl));
		}
		else if(auto ns = detail::parse_namespace(path, infos, c); ns){
			return std::make_optional<entity>(std::move(*ns));
		}
		else if(auto alias = detail::parse_type_alias(path, infos, c); alias){
			return std::make_optional<entity>(std::move(*alias));
		}
		else{
			return std::nullopt;
		}
	}
}

ast::info_map ast::parse(const fs::path &path, const compile_info &info){
	using namespace astpp;

	auto abs_path = fs::absolute(path);

	static clang::index index;

	if(!fs::exists(path)){
		auto msg = fmt::format("File '{}' does not exist", path.c_str());
		throw std::runtime_error(msg);
	}
	else if(!fs::is_regular_file(path)){
		auto msg = fmt::format("'{}' is not a regular file", path.c_str());
		throw std::runtime_error(msg);
	}

	auto retrieve = [&info](const std::string &p) -> std::pair<ast::info_map*, ast::clang::translation_unit*>{
		static std::unordered_map<std::string, std::pair<info_map, clang::translation_unit>> tus;

		auto res = tus.find(p);
		if(res != tus.end()){
			return std::make_pair(&res->second.first, &res->second.second);
		}

		auto emplace_res = tus.try_emplace(
			p,
			std::piecewise_construct,
			std::forward_as_tuple(),
			std::forward_as_tuple(index, p, info.file_options(p))
		);

		if(!emplace_res.second){
			return std::make_pair(nullptr, nullptr);
		}

		return std::make_pair(&res->second.first, &res->second.second);
	};

	auto options = info.file_options(path);

	bool standard_given = false;

	for(auto &&opt : options){
		if(opt.starts_with("-std=")){
			standard_given = true;
		}
	}

	if(!standard_given){
		options.insert(options.begin(), "-std=c++17");
	}

	clang::translation_unit tu(index, path, options);

	const unsigned int num_diag = clang_getNumDiagnostics(tu);

	bool found_err = false;

	for(unsigned int i = 0; i < num_diag; i++){
		CXDiagnostic diagnotic = clang_getDiagnostic(tu, i);

		auto err_str = clang::detail::convert_str(clang_formatDiagnostic(diagnotic, clang_defaultDiagnosticDisplayOptions()));

		if(err_str.find("error:") != std::string::npos){
			found_err = true;
		}

		fmt::print(stderr, "{}\n", err_str);
	}

	if(found_err){
		throw std::runtime_error("AST parsing failed with errors");
	}

	info_map ret;

	std::function<void(clang::cursor, clang::cursor)> visitor = [&](clang::cursor cursor, clang::cursor parent){
		if(cursor.kind() == CXCursor_InclusionDirective){
			auto included_file = clang_getIncludedFile(cursor);
			auto include_str = clang::detail::convert_str(clang_getFileName(included_file));

			//auto included_tu = get_tu(include_str, build_dir);
			return;
		}
		else if(cursor.kind() == CXCursor_UsingDirective){
			return; // skip using directives
		}

		auto entity = ast::detail::parse_namespace_inner(path, ret, ret.global, cursor);
		if(!entity){
			//ast::detail::print_parse_warning(path, "Unrecognized cursor '{}' of kind '{}'", cursor.spelling(), cursor.kind_spelling());
		}
	};

	tu.get_cursor().visit_children(visitor);

	return ret;
}
