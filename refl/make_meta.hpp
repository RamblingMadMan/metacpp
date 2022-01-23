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

#ifndef MAKE_META_HPP
#define MAKE_META_HPP 1

#include "metacpp/ast.hpp"

#include "fmt/format.h"

std::string make_ctor_meta(const ast::class_info &cls, const ast::class_constructor_info &ctor, std::size_t idx){
	std::string param_types_str;

	for(auto &&param_type : ctor.param_types){
		param_types_str += param_type;
		param_types_str += ", ";
	}

	if(!param_types_str.empty()){
		param_types_str.erase(param_types_str.size() - 2);
	}

	return fmt::format(
		"template<> struct metapp::detail::class_ctor_info_data<{0}, {1}>{{\n"
		"\t"	"static constexpr std::size_t num_params = {2};\n"
		"\t"	"static constexpr bool is_move_ctor = {3};\n"
		"\t"	"static constexpr bool is_copy_ctor = {4};\n"
		"\t"	"static constexpr bool is_default_ctor = {5};\n"
		"}};\n",
		cls.name, idx,
		ctor.param_names.size(),
		ctor.constructor_kind == ast::constructor_kind::move,
		ctor.constructor_kind == ast::constructor_kind::copy,
		ctor.constructor_kind == ast::constructor_kind::default_
	);
}

std::string make_member_meta(const ast::class_info &cls, const ast::class_member_info &m, std::size_t idx){
	return fmt::format(
		"template<> struct metapp::detail::class_member_info_data<{0}, {1}>{{\n"
		"\t"	"static constexpr std::string_view name = \"{2}\";\n"
		"\t"	"using type = {3};\n"
		"}};\n",
		cls.name, idx, m.name, m.type
	);
}

std::string make_method_meta(const ast::class_info &cls, const ast::class_method_info &m, std::size_t idx){
	std::string param_metas_str, param_types_str, param_names_member_str, params_member_str;

	for(std::size_t i = 0; i < m.param_types.size(); i++){
		auto &&param_type = m.param_types[i];
		auto &&param_name = m.param_names[i];

		param_metas_str += fmt::format(
			"template<> struct metapp::detail::class_method_param_info_data<{0}, {1}, {2}>{{\n"
			"\t"	"static constexpr std::string_view name = \"{3}\";\n"
			"\t"	"using type = {4};\n"
			"}};\n"
			"\n",
			cls.name, idx, i, param_name, param_type
		);

		param_types_str += param_type;
		param_types_str += ", ";

		param_names_member_str += ",\n";
		param_names_member_str += fmt::format("\t\t"	"metapp::str(\"{}\")", param_name);

		params_member_str += ",\n";
		params_member_str += fmt::format("\t\t"		"metapp::class_method_param_info<{0}, {1}, {2}>", cls.name, idx, i);
	}

	if(!param_types_str.empty()){
		param_types_str.erase(param_types_str.size() - 2);

		param_names_member_str.erase(0, 1);
		param_names_member_str += "\n\t";

		params_member_str.erase(0, 1);
		params_member_str += "\n\t";
	}

	return fmt::format(
		"{7}"
		"template<> struct metapp::detail::class_method_info_data<{0}, {1}>{{\n"
		"\t"	"using ptr_type = {3}({0}::*)({4}){6};\n"
		"\t"	"using result = {9};\n"
		"#if __cplusplus >= 202002L\n"
		"\t"	"using param_names = metapp::values<{10}>;\n"
		"#endif\n"
		"\t"	"using param_types = metapp::types<{4}>;\n"
		"\t"	"using params = metapp::types<{8}>;\n"
		"\t"	"static constexpr std::string_view name = \"{2}\";\n"
		"\t"	"static constexpr std::size_t num_params = {5};\n"
		"\t"	"static constexpr ptr_type ptr = &{0}::{2};\n"
		"}};\n",
		cls.name, idx,
		m.name, m.result_type, param_types_str, m.param_types.size(),
		m.is_const ? " const" : "",
		param_metas_str,
		params_member_str,
		m.result_type,
		param_names_member_str
	);
}

std::string make_class_meta(const ast::class_info &cls){
	std::string output, ctors_member_str, methods_member_str, members_member_str;

	std::size_t ctor_idx = 0;

	for(auto &&ctor : cls.ctors){
		output += make_ctor_meta(cls, *ctor, ctor_idx++);
	}

	std::size_t method_idx = 0, member_idx;

	for(auto &&methods : cls.methods){
		for(auto &&m : methods.second){
			methods_member_str += ",\n";
			methods_member_str += fmt::format(
				"\t\t"	"metapp::class_method_info<{0}, {1}>",
				cls.name, method_idx
			);

			output += make_method_meta(cls, *m, method_idx++);
			output += "\n";
		}
	}

	for(auto &&member : cls.members){
		members_member_str += ",\n";
		members_member_str += fmt::format(
			"\t\t"	"metapp::class_member_info<{0}, {1}>",
			cls.name, member_idx
		);

		output += make_member_meta(cls, *member.second, member_idx++);
		output += "\n";
	}

	if(!methods_member_str.empty()){
		methods_member_str.erase(0, 1);
		methods_member_str += "\n\t";
	}

	if(!members_member_str.empty()){
		members_member_str.erase(0, 1);
		members_member_str += "\n\t";
	}

	if(!ctors_member_str.empty()){
		ctors_member_str.erase(0, 1);
		ctors_member_str += "\n\t";
	}

	return fmt::format(
		"{0}"
		"template<> struct metapp::detail::class_info_data<{1}>{{\n"
		"\t"	"static constexpr std::string_view name = metapp::type_name<{1}>;\n"
		"\t"	"static constexpr std::size_t num_bases = {3};\n"
		"\t"	"static constexpr std::size_t num_ctors = {5};\n"
		"\t"	"static constexpr std::size_t num_methods = {2};\n"
		"\t"	"using ctors = metapp::types<{6}>;\n"
		"\t"	"using methods = metapp::types<{4}>;\n"
		"\t"	"using members = metapp::types<{7}>;\n"
		"}};\n",
		output, cls.name, cls.methods.size(), cls.bases.size(), methods_member_str,
		cls.ctors.size(), ctors_member_str, members_member_str
	);
}

std::string make_namespace_meta(const ast::namespace_info &ns){
	std::string output;

	for(auto &&cls : ns.classes){
		output += make_class_meta(*cls.second);
		output += "\n";
	}

	for(auto &&inner : ns.namespaces){
		output += make_namespace_meta(*inner.second);
	}

	return output;
}

#endif // MAKE_META_HPP
