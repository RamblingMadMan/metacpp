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

std::string make_function_meta(
	const ast::function_info &fn
){
	std::string output;
	std::string param_types_str, params_member_str;

	for(std::size_t i = 0; i < fn.param_types.size(); i++){
		auto &&param_type = fn.param_types[i];
		auto &&param_name = fn.param_names[i];

		params_member_str += fmt::format(
			",\n"
			"\t"	"\t"	"metapp::param_info<metapp::value<ptr>, metapp::value<{}>>",
			i
		);

		param_types_str += fmt::format(", {}", param_type);
	}

	if(!param_types_str.empty()){
		param_types_str.erase(0, 2);

		params_member_str.erase(0, 1);
		params_member_str += "\n\t";
	}


	std::string full_name = fn.name;

	std::string fn_val = fmt::format(
		"static_cast<{}(*)({})>(&{})",
		fn.result_type, param_types_str, full_name
	);

	for(std::size_t i = 0; i < fn.param_types.size(); i++){
		auto &&param_type = fn.param_types[i];
		auto &&param_name = fn.param_names[i];

		const bool param_is_variadic = param_type.rfind("...") == (param_type.size() - 3);

		output += fmt::format(
			"template<> struct metapp::detail::param_info_data<metapp::value<({4})>, {1}>{{\n"
			"\t"	"using type = {2};\n"
			"\t"	"static constexpr std::string_view name = \"{0}\";\n"
			"\t"	"static constexpr bool is_variadic = {3};\n"
			"}};\n"
			"\n",
			param_name,
			i,
			param_is_variadic ? fmt::format("meta::types<{}>", param_type) : param_type,
			param_is_variadic,
			fn_val
		);
	}

	return fmt::format(
		"{0}"
		"template<> struct metapp::detail::function_info_data<({5})>{{\n"
		"\t"	"static constexpr std::string_view name = \"{1}\";\n"
		"\t"	"using type = {2}(*)({3});\n"
		"\t"	"static constexpr type ptr = {1};\n"
		"\t"	"using result = {2};\n"
		"\t"	"using params = metapp::types<{4}>;\n"
		"}};\n",
		output,
		full_name,
		fn.result_type,
		param_types_str,
		params_member_str,
		fn_val
	);
}

std::string make_ctor_meta(
	std::string_view tmpl_params,
	std::string_view full_name,
	const ast::class_constructor_info &ctor,
	std::size_t idx
){
	std::string output, param_types_str, params_member_str;

	std::size_t param_idx = 0;
	for(auto &&param_type : ctor.param_types){
		auto &&param_name = ctor.param_names[param_idx];

		param_types_str += fmt::format("{}, ", param_type);

		params_member_str += fmt::format(
			",\n"
			"\t"	"\t"	"metapp::param_info<metapp::class_ctor_info<{0}, metapp::value<{1}>>, metapp::value<{2}>>",
			full_name, idx, param_idx
		);

		const bool param_is_variadic = param_type.rfind("...") == (param_type.size() - 3);

		output += fmt::format(
			"template<{0}> struct metapp::detail::param_info_data<{1}, {2}>{{\n"
			"\t"	"using type = {3};\n"
			"\t"	"static constexpr std::string_view name = \"{4}\";\n"
			"\t"	"static constexpr bool is_variadic = {5};\n"
			"}};\n"
			"\n",
			tmpl_params,
			fmt::format("metapp::class_ctor_info<{}, metapp::value<{}>>", full_name, idx),
			param_idx,
			param_is_variadic ? fmt::format("metapp::types<{}>", param_type) : param_type,
			param_name,
			param_is_variadic
		);

		++param_idx;
	}

	if(!param_types_str.empty()){
		param_types_str.erase(param_types_str.size() - 2);

		params_member_str.erase(0, 1);
		params_member_str += "\n\t";
	}

	return fmt::format(
		"{8}"
		"template<{6}> struct metapp::detail::class_ctor_info_data<{0}, {1}>{{\n"
		"\t"	"using params = metapp::types<{7}>;\n"
		"\t"	"static constexpr std::size_t num_params = {2};\n"
		"\t"	"static constexpr bool is_move_ctor = {3};\n"
		"\t"	"static constexpr bool is_copy_ctor = {4};\n"
		"\t"	"static constexpr bool is_default_ctor = {5};\n"
		"\t"	"static constexpr bool is_accessable = {9};\n"
		"}};\n",
		full_name,
		idx,
		ctor.param_names.size(),
		ctor.constructor_kind == ast::constructor_kind::move,
		ctor.constructor_kind == ast::constructor_kind::copy,
		ctor.constructor_kind == ast::constructor_kind::default_,
		tmpl_params,
		params_member_str,
		output,
		ctor.is_accessable
	);
}

std::string make_member_meta(
	std::string_view tmpl_params,
	std::string_view full_name,
	const ast::class_member_info &m,
	std::size_t idx
){
	std::string ptr_str;

	if(m.is_accessable){
		ptr_str = fmt::format("\t"	"static constexpr ptr_type ptr = &{}::{};\n", full_name, m.name);
	}
	else{
		ptr_str = fmt::format("\t"	"static constexpr metapp::inaccessible<ptr_type> ptr = {{}};\n");
	}

	return fmt::format(
		"template<{4}> struct metapp::detail::class_member_info_data<{0}, {1}>{{\n"
		"\t"	"using class_ = {0};\n"
		"\t"	"using type = {3};\n"
		"\t"	"using ptr_type = type ({0}::*);\n"
		"\t"	"static constexpr std::string_view name = \"{2}\";\n"
				"{5}"
		"}};\n",
		full_name, idx, m.name, m.type, tmpl_params, ptr_str
	);
}

std::string make_method_meta(
	std::string_view tmpl_params,
	std::string_view full_name,
	const ast::class_method_info &m,
	std::size_t idx
){
	std::string param_metas_str, param_types_str, param_names_member_str, params_member_str;

	for(std::size_t i = 0; i < m.param_types.size(); i++){
		std::string param_type = m.param_types[i];
		auto &&param_name = m.param_names[i];

		const bool param_is_variadic = param_type.rfind("...") == (param_type.size() - 3);

		param_metas_str += fmt::format(
			"template<{5}> struct metapp::detail::class_method_param_info_data<{0}, {1}, {2}>{{\n"
			"\t"	"static constexpr std::string_view name = \"{3}\";\n"
			"\t"	"static constexpr bool is_variadic = {6};\n"
			"\t"	"using type = {4};\n"
			"}};\n"
			"\n",
			full_name, idx, i, param_name,
			param_is_variadic ? fmt::format("metapp::types<{}>", param_type) : param_type,
			tmpl_params,
			param_is_variadic
		);

		param_types_str += param_type;
		param_types_str += ", ";

		param_names_member_str += ",\n";
		param_names_member_str += fmt::format("\t\t"	"metapp::str(\"{}\")", param_name);

		params_member_str += ",\n";
		params_member_str += fmt::format("\t\t"		"metapp::class_method_param_info<{0}, metapp::value<{1}>, metapp::value<{2}>>", full_name, idx, i);
	}

	if(!param_types_str.empty()){
		param_types_str.erase(param_types_str.size() - 2);

		param_names_member_str.erase(0, 1);
		param_names_member_str += "\n\t";

		params_member_str.erase(0, 1);
		params_member_str += "\n\t";
	}

	std::string ptr_str;

	if(m.is_accessable){
		ptr_str = fmt::format("\t"	"static constexpr ptr_type ptr = &{}::{};\n", full_name, m.name);
	}
	else{
		ptr_str = fmt::format("\t"	"static constexpr metapp::inaccessible<ptr_type> ptr = {{}};\n");
	}

	return fmt::format(
		"{7}"
		"template<{10}> struct metapp::detail::class_method_info_data<{0}, {1}>{{\n"
		"\t"	"using ptr_type = {3}({12}*)({4}){6};\n"
		"\t"	"using result = {9};\n"
		"\t"	"using param_types = metapp::types<{4}>;\n"
		"\t"	"using params = metapp::types<{8}>;\n"
		"\t"	"static constexpr std::string_view name = \"{2}\";\n"
				"{11}"
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
		tmpl_params,
		ptr_str,
		m.is_static ? "" : fmt::format("{}::", full_name)
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

	for(auto &&tmpl_param : cls.template_params){
		tmpl_param_names += fmt::format(", {}", tmpl_param.name);
		if(tmpl_param.is_variadic){
			tmpl_param_names += "...";
		}

		tmpl_params += fmt::format(
			", {}{} {}",
			tmpl_param.declarator, (tmpl_param.is_variadic ? "..." : ""),
			tmpl_param.name
		);
	}

	if(!tmpl_param_names.empty()){
		tmpl_param_names.erase(0, 2);
		tmpl_params.erase(0, 2);
	}

	std::string full_name = cls.name;

	if(cls.is_specialization){
		std::string spec_args;
		for(auto &&arg : cls.template_args){
			spec_args += fmt::format("{}, ", arg);
		}
		if(!spec_args.empty()){
			spec_args.erase(spec_args.end() - 2);
		}
		full_name += fmt::format("<{}>", spec_args);
	}
	else if(!tmpl_param_names.empty()){
		full_name += fmt::format("<{}>", tmpl_param_names);
	}

	std::size_t base_idx = 0;

	for(auto &&base : cls.bases){
		std::string type_alias;

		if(base.is_variadic){
			type_alias = fmt::format("metapp::types<{}...>", base.name);
		}
		else{
			type_alias = base.name;
		}

		output += fmt::format(
			"template<{0}> struct metapp::detail::class_base_info_data<{1}, {2}>{{\n"
			"\t"	"static constexpr auto access = metapp::access_kind::{3};\n"
			"\t"	"static constexpr bool is_variadic = {5};\n"
			"\t"	"using type = {4};\n"
			"}};\n"
			"\n",
			tmpl_params,
			full_name,
			base_idx,
			access_to_str(base.access),
			type_alias,
			(base.is_variadic ? "true" : "false")
		);

		bases_member_str += fmt::format(
			",\n"
			"\t\t"	"metapp::class_base_info<{0}, metapp::value<{1}>>",
			full_name,
			base_idx++
		);
	}

	if(!bases_member_str.empty()){
		bases_member_str.erase(0, 1);
		bases_member_str += "\n\t";
	}

	std::size_t ctor_idx = 0;

	for(auto &&ctor : cls.ctors){
		output += make_ctor_meta(tmpl_params, full_name, *ctor, ctor_idx);

		ctors_member_str += fmt::format(
			",\n"
			"\t"	"\t"	"metapp::class_ctor_info<{0}, metapp::value<{1}>>",
			full_name,
			ctor_idx++
		);
	}

	std::size_t attrib_idx = 0, method_idx = 0, member_idx = 0;

	for(auto &&attrib : cls.attributes){
		attribs_member_str += fmt::format(
			",\n"
			"\t\t"	"metapp::attrib_info<{0}, metapp::value<{1}>>",
			full_name,
			attrib_idx
		);

		std::string args_member_str;

		std::size_t attrib_arg_idx = 0;

		for(auto &&arg : attrib.args()){
			args_member_str += fmt::format(
				",\n"
				"\t\t"	"metapp::attrib_arg_info<{0}, metapp::value<{1}>, metapp::value<{2}>>",
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
				"\t\t"	"metapp::class_method_info<{0}, metapp::value<{1}>>",
				full_name, method_idx
			);

			output += make_method_meta(tmpl_params, full_name, *m, method_idx++);
			output += "\n";
		}
	}

	for(auto &&member : cls.members){
		members_member_str += fmt::format(
			",\n"
			"\t\t"	"metapp::class_member_info<{0}, metapp::value<{1}>>",
			full_name, member_idx
		);

		output += make_member_meta(tmpl_params, full_name, member, member_idx++);
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

		attribs_member_str, // 5
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
			"\t\t"	"metapp::enum_value_info<{0}, metapp::value<{1}>>",
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
		"\t"	"static constexpr std::string_view name = metapp::type_name<{1}>;\n"
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

	for(auto &&fns : ns.functions){
		for(auto &&fn : fns.second){
			output += make_function_meta(*fn);
			output += "\n";
		}
	}

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
