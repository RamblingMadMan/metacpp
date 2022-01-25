/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef MAKE_META_HPP
#define MAKE_META_HPP 1

#include "metacpp/ast.hpp"

#include "fmt/format.h"

std::string class_full_name(const ast::class_info &cls){
	std::string ret = cls.name;

	std::string tmpl_param_names;
	for(auto &&param : cls.template_params){
		tmpl_param_names += fmt::format(", {}", param.name);
	}

	if(!tmpl_param_names.empty()){
		ret += fmt::format("<{}>", std::string_view(tmpl_param_names.data() + 2, tmpl_param_names.size() - 2));
	}

	return ret;
}

std::string class_tmpl_params(const ast::class_info &cls){
	std::string ret;

	for(auto &&param : cls.template_params){
		ret += fmt::format(", {} {}", param.declarator, param.name);
	}

	if(!ret.empty()){
		ret.erase(0, 2);
	}

	return ret;
}

std::string make_ctor_meta(const ast::class_info &cls, const ast::class_constructor_info &ctor, std::size_t idx){
	const auto full_name = class_full_name(cls);
	const auto tmpl_params = class_tmpl_params(cls);

	std::string param_types_str;

	for(auto &&param_type : ctor.param_types){
		param_types_str += param_type;
		param_types_str += ", ";
	}

	if(!param_types_str.empty()){
		param_types_str.erase(param_types_str.size() - 2);
	}

	return fmt::format(
		"template<{6}> struct metapp::detail::class_ctor_info_data<{0}, {1}>{{\n"
		"\t"	"static constexpr std::size_t num_params = {2};\n"
		"\t"	"static constexpr bool is_move_ctor = {3};\n"
		"\t"	"static constexpr bool is_copy_ctor = {4};\n"
		"\t"	"static constexpr bool is_default_ctor = {5};\n"
		"}};\n",
		full_name,
		idx,
		ctor.param_names.size(),
		ctor.constructor_kind == ast::constructor_kind::move,
		ctor.constructor_kind == ast::constructor_kind::copy,
		ctor.constructor_kind == ast::constructor_kind::default_,
		tmpl_params
	);
}

std::string make_member_meta(const ast::class_info &cls, const ast::class_member_info &m, std::size_t idx){
	const auto full_name = class_full_name(cls);
	const auto tmpl_params = class_tmpl_params(cls);

	return fmt::format(
		"template<{4}> struct metapp::detail::class_member_info_data<{0}, {1}>{{\n"
		"\t"	"using type = {3};\n"
		"\t"	"using ptr_type = {3} {0}::*;\n"
		"\t"	"static constexpr std::string_view name = \"{2}\";\n"
		"\t"	"static constexpr ptr_type ptr = &{0}::{2};\n"
		"}};\n",
		full_name, idx, m.name, m.type, tmpl_params
	);
}

std::string make_method_meta(const ast::class_info &cls, const ast::class_method_info &m, std::size_t idx){
	std::string param_metas_str, param_types_str, param_names_member_str, params_member_str;	

	const auto full_name = class_full_name(cls);
	const auto tmpl_params = class_tmpl_params(cls);

	for(std::size_t i = 0; i < m.param_types.size(); i++){
		auto &&param_type = m.param_types[i];
		auto &&param_name = m.param_names[i];

		param_metas_str += fmt::format(
			"template<{5}> struct metapp::detail::class_method_param_info_data<{0}, {1}, {2}>{{\n"
			"\t"	"static constexpr std::string_view name = \"{3}\";\n"
			"\t"	"using type = {4};\n"
			"}};\n"
			"\n",
			full_name, idx, i, param_name, param_type,
			tmpl_params
		);

		param_types_str += param_type;
		param_types_str += ", ";

		param_names_member_str += ",\n";
		param_names_member_str += fmt::format("\t\t"	"metapp::str(\"{}\")", param_name);

		params_member_str += ",\n";
		params_member_str += fmt::format("\t\t"		"metapp::class_method_param_info<{0}, {1}, {2}>", full_name, idx, i);
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
		"template<{11}> struct metapp::detail::class_method_info_data<{0}, {1}>{{\n"
		"\t"	"using ptr_type = {3}({0}::*)({4}){6};\n"
		"\t"	"using result = {9};\n"
		"#if __cplusplus >= 202002L\n"
		"\t"	"using param_names = metapp::values<{10}>;\n"
		"#endif\n"
		"\t"	"using param_types = metapp::types<{4}>;\n"
		"\t"	"using params = metapp::types<{8}>;\n"
		"\t"	"static constexpr std::string_view name = \"{2}\";\n"
		"\t"	"static constexpr ptr_type ptr = &{0}::{2};\n"
		"}};\n",
		full_name,
		idx,
		m.name,
		m.result_type,
		param_types_str,
		m.param_types.size(),
		m.is_const ? " const" : "",
		param_metas_str,
		params_member_str,
		m.result_type,
		param_names_member_str,
		tmpl_params
	);
}

std::string_view access_to_str(ast::access_kind access){
	switch(access){
		case ast::access_kind::public_: return "public_";
		case ast::access_kind::protected_: return "protected_";
		default: return "private_";
	}
}

std::string make_class_meta(const ast::class_info &cls){
	std::string
		output,
		tmpl_param_names, tmpl_params,
		attribs_member_str,
		bases_member_str,
		ctors_member_str,
		methods_member_str,
		members_member_str;

	std::size_t ctor_idx = 0;

	for(auto &&tmpl_param : cls.template_params){
		tmpl_param_names += fmt::format(", {}", tmpl_param.name);
		tmpl_params += fmt::format(", {} {}", tmpl_param.declarator, tmpl_param.name);
	}

	std::string full_name = cls.name;

	if(!tmpl_param_names.empty()){
		tmpl_param_names.erase(0, 2);
		tmpl_params.erase(0, 2);
		full_name += fmt::format("<{}>", tmpl_param_names);
	}

	std::size_t base_idx = 0;

	for(auto &&base : cls.bases){
		output += fmt::format(
			"template<{0}> struct metapp::detail::class_base_info_data<{1}, {2}>{{\n"
			"\t"	"static constexpr auto access = metapp::access_kind::{3};\n"
			"\t"	"using type = {4};\n"
			"}};\n"
			"\n",
			tmpl_params,
			full_name,
			base_idx,
			access_to_str(base.access),
			base.name
		);

		bases_member_str += fmt::format(
			",\n"
			"\t\t"	"metapp::class_base_info<{0}, {1}>",
			full_name,
			base_idx++
		);
	}

	if(!bases_member_str.empty()){
		bases_member_str.erase(0, 1);
		bases_member_str += "\n\t";
	}

	for(auto &&ctor : cls.ctors){
		output += make_ctor_meta(cls, *ctor, ctor_idx++);
	}

	std::size_t attrib_idx = 0, method_idx = 0, member_idx = 0;

	for(auto &&attrib : cls.attributes){
		attribs_member_str += fmt::format(
			",\n"
			"\t\t"	"metapp::attrib_info<{0}, {1}>",
			full_name,
			attrib_idx
		);

		std::string args_member_str;

		std::size_t attrib_arg_idx = 0;

		for(auto &&arg : attrib.args()){
			args_member_str += fmt::format(
				",\n"
				"\t\t"	"metapp::attrib_arg_info<{0}, {1}, {2}>",
				full_name,
				attrib_idx,
				attrib_arg_idx
			);

			output += fmt::format(
				"template<{4}> struct metapp::detail::attrib_arg_info_data<{0}, {1}, {2}>{{\n"
				"\t"	"static constexpr std::string_view value = R\"({3})\";\n"
				"}};\n"
				"\n",
				full_name,
				attrib_idx,
				attrib_arg_idx++,
				arg,
				tmpl_params
			);
		}

		if(!args_member_str.empty()){
			args_member_str.erase(0, 1);
			args_member_str += "\n\t";
		}

		output += fmt::format(
			"template<{5}> struct metapp::detail::attrib_info_data<{0}, {1}>{{\n"
			"\t"	"static constexpr std::string_view scope = \"{2}\";\n"
			"\t"	"static constexpr std::string_view name = \"{3}\";\n"
			"\t"	"using args = metapp::types<{4}>;\n"
			"}};\n"
			"\n",
			full_name,
			attrib_idx++,
			attrib.scope(),
			attrib.name(),
			args_member_str,
			tmpl_params
		);
	}

	for(auto &&methods : cls.methods){
		for(auto &&m : methods.second){
			methods_member_str += fmt::format(
				",\n"
				"\t\t"	"metapp::class_method_info<{0}, {1}>",
				full_name, method_idx
			);

			output += make_method_meta(cls, *m, method_idx++);
			output += "\n";
		}
	}

	for(auto &&member : cls.members){
		members_member_str += fmt::format(
			",\n"
			"\t\t"	"metapp::class_member_info<{0}, {1}>",
			full_name, member_idx
		);

		output += make_member_meta(cls, member, member_idx++);
		output += "\n";
	}

	if(!attribs_member_str.empty()){
		attribs_member_str.erase(0, 1);
		attribs_member_str += "\n\t";
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
		"template<{6}> struct metapp::detail::class_info_data<{1}>{{\n"
		"\t"	"static constexpr std::string_view name = metapp::type_name<{1}>;\n"
		"\t"	"using attributes = metapp::types<{5}>;\n"
		"\t"	"using bases = metapp::types<{7}>;\n"
		"\t"	"using ctors = metapp::types<{3}>;\n"
		"\t"	"using methods = metapp::types<{2}>;\n"
		"\t"	"using members = metapp::types<{4}>;\n"
		"}};\n",

		output,
		full_name,
		methods_member_str,
		ctors_member_str,
		members_member_str,

		attribs_member_str, // 6
		tmpl_params,
		bases_member_str
	);
}

std::string make_enum_meta(const ast::enum_info &enm){
	std::string output;

	std::string values_member_str;

	std::size_t value_idx = 0;

	for(auto &&value : enm.values){
		output += fmt::format(
			"template<> struct metapp::detail::enum_value_info_data<{0}, {1}>{{\n"
			"\t"	"static constexpr std::string_view name = \"{2}\";\n"
			"\t"	"static constexpr std::uint64_t value = {3};\n"
			"}};\n"
			"\n",
			enm.name, value_idx, value.name, value.value
		);

		values_member_str += fmt::format(
			",\n"
			"\t\t"	"metapp::enum_value_info<{0}, {1}>",
			enm.name, value_idx++
		);
	}

	if(!values_member_str.empty()){
		values_member_str.erase(0, 1);
		values_member_str += "\n\t";
	}

	return fmt::format(
		"{0}"
		"template<> struct metapp::detail::enum_info_data<{1}>{{\n"
		"\t"	"using values = metapp::types<{3}>;\n"
		"\t"	"static constexpr std::string_view name = \"{1}\";\n"
		"\t"	"static constexpr bool is_scoped = {2};\n"
		"}};\n",
		output,
		enm.name,
		enm.is_scoped ? "true" : "false",
		values_member_str
	);
}

std::string make_namespace_meta(const ast::namespace_info &ns){
	std::string output;

	for(auto &&cls : ns.classes){
		output += make_class_meta(*cls.second);
		output += "\n";
	}

	for(auto &&enm : ns.enums){
		output += make_enum_meta(*enm.second);
		output += "\n";
	}

	for(auto &&inner : ns.namespaces){
		output += make_namespace_meta(*inner.second);
	}

	return output;
}

#endif // MAKE_META_HPP
