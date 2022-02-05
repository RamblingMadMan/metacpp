/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <functional>
#include <iostream>
#include <optional>
#include <set>

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
	std::decay_t<T> *store_info(info_map &info, T &&ent){
		auto ptr = std::make_unique<std::decay_t<T>>(std::move(std::forward<T>(ent)));
		auto ret = ptr.get();
		info.storage.emplace_back(std::move(ptr));
		return ret;
	}

	std::optional<entity> try_parse(const fs::path &path, info_map &infos, clang::cursor c, namespace_info *ns);

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
					if(!arg_str.empty()){
						ret.emplace_back(std::move(arg_str));
					}
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

	std::optional<class_constructor_info> parse_class_ctor(const fs::path &path, info_map &infos, clang::cursor c, class_info *cls){
		if(c.kind() != CXCursor_Constructor){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXPublic){
			return std::nullopt;
		}

		class_constructor_info ret;

		ret.ns = cls->ns;

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

		std::function<std::string(std::string_view)> replace_self_refs = [](std::string_view type_str){
			return std::string(type_str);
		};

		if(!cls->template_params.empty()){
			const std::string cls_name = cls->name.substr(2); // after global namespace ::
			const std::string cls_name_opened = fmt::format("{}<", cls_name);

			std::string full_name = "::" + cls_name_opened;

			for(auto &&template_param : cls->template_params){
				full_name += fmt::format("{}, ", template_param.name);
			}

			full_name.erase(full_name.size() - 2);
			full_name.insert(full_name.end(), '>');

			replace_self_refs = [cls_name, cls_name_opened, full_name](std::string_view type_str){
				auto ret = std::string(type_str);
				std::size_t name_pos = 0;

				while(1){
					name_pos = ret.find(cls_name, name_pos);
					if(name_pos == std::string::npos) break;

					auto opened_pos = ret.find(cls_name_opened);
					if(opened_pos != name_pos){
						ret.replace(name_pos, cls_name.size(), full_name);
						name_pos += full_name.size();
					}
					else{
						name_pos += cls_name_opened.size();
					}
				}

				return ret;
			};
		}

		int num_params = clang_Cursor_getNumArguments(c);

		if(num_params > 0){
			ret.param_names.reserve(num_params);
			ret.param_types.reserve(num_params);

			for(int i = 0; i < num_params; i++){
				clang::cursor arg_cursor = clang_Cursor_getArgument(c, i);
				clang::type arg_type = clang_getCursorType(arg_cursor);

				auto arg_type_name = arg_type.spelling();

				ret.param_names.emplace_back(arg_cursor.spelling());
				ret.param_types.emplace_back(replace_self_refs(arg_type_name));
			}
		}

		return ret;
	}

	std::optional<class_destructor_info> parse_class_dtor(const fs::path &path, info_map &infos, clang::cursor c, namespace_info *ns){
		if(c.kind() != CXCursor_Destructor){
			return std::nullopt;
		}

		class_destructor_info ret;

		ret.ns = ns;

		return ret;
	}

	std::string resolve_namespaces(clang::cursor c){
		std::string ret;

		while(1){
			clang::cursor parent = clang_getCursorSemanticParent(c);
			if(clang_isInvalid(parent.kind()) || clang_isTranslationUnit(parent.kind())){
				break;
			}
			else if(parent.kind() == CXCursor_ClassDecl || parent.kind() == CXCursor_Namespace){
				auto ns_str = fmt::format("::{}", parent.spelling());
				ret.insert(ret.begin(), ns_str.begin(), ns_str.end());
			}

			c = parent;
		}

		return ret;
	}

	std::optional<class_member_info> parse_class_member(const fs::path &path, info_map &infos, clang::cursor c, class_info *cls){
		if(c.kind() != CXCursor_FieldDecl){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXPublic){
			return std::nullopt;
		}

		class_member_info ret;

		auto member_type = c.type();
		auto member_type_str = member_type.spelling();

		clang::cursor type_decl = clang_getTypeDeclaration(member_type);
		if(!clang_isInvalid(type_decl.kind())){
			auto namespaces = resolve_namespaces(type_decl);
			member_type_str = fmt::format("{}::{}", namespaces, type_decl.spelling());

			auto num_tmpl_args = clang_Type_getNumTemplateArguments(member_type);

			if(num_tmpl_args > 0){
				member_type_str += "<";

				for(int i = 0; i < num_tmpl_args; i++){
					clang::type arg_type = clang_Type_getTemplateArgumentAsType(member_type, i);
					member_type_str += fmt::format("{}, ", arg_type.spelling());
				}

				member_type_str.erase(member_type_str.size() - 2);
				member_type_str += ">";
			}
		}

		ret.ns = cls->ns;
		ret.name = c.spelling();
		ret.type = member_type_str;

		return ret;
	}

	std::optional<class_method_info> parse_class_method(const fs::path &path, info_map &infos, clang::cursor c, class_info *cls){
		if(c.kind() != CXCursor_CXXMethod){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXPublic){
			return std::nullopt;
		}

		class_method_info ret;

		ret.ns = cls->ns;
		ret.name = c.spelling();
		ret.is_static = clang_CXXMethod_isStatic(c);
		ret.is_const = clang_CXXMethod_isConst(c);
		ret.is_virtual = clang_CXXMethod_isVirtual(c);
		ret.is_pure_virtual = clang_CXXMethod_isPureVirtual(c);
		ret.is_defaulted = clang_CXXMethod_isDefaulted(c);

		std::function<std::string(std::string_view)> replace_self_refs = [](std::string_view type_str){
			return std::string(type_str);
		};

		if(!cls->template_params.empty()){
			const std::string cls_name = cls->name.substr(2); // after global namespace ::
			const std::string cls_name_opened = fmt::format("{}<", cls_name);

			std::string full_name = "::" + cls_name_opened;

			for(auto &&template_param : cls->template_params){
				if(template_param.is_variadic){
					full_name += fmt::format("{}..., ", template_param.name);
				}
				else{
					full_name += fmt::format("{}, ", template_param.name);
				}
			}

			full_name.erase(full_name.size() - 2);
			full_name.insert(full_name.end(), '>');

			replace_self_refs = [cls_name, cls_name_opened, full_name](std::string_view type_str){
				auto ret = std::string(type_str);
				std::size_t name_pos = 0;

				while(1){
					name_pos = ret.find(cls_name, name_pos);
					if(name_pos == std::string::npos) break;

					auto opened_pos = ret.find(cls_name_opened);
					if(opened_pos != name_pos){
						ret.replace(name_pos, cls_name.size(), full_name);
						name_pos += full_name.size();
					}
					else{
						name_pos += cls_name_opened.size();
					}
				}

				return ret;
			};
		}

		ret.name = replace_self_refs(ret.name);

		auto fn_type = c.type();

		ret.is_noexcept =
			clang_getExceptionSpecificationType(fn_type) == CXCursor_ExceptionSpecificationKind_BasicNoexcept;

		auto fn_type_str = fn_type.spelling();

		{
			auto result_type = clang::type(clang_getResultType(fn_type));
			auto result_type_str = result_type.spelling();

			if(result_type.kind() == CXType_Typedef){
				result_type_str = fmt::format("typename {}", result_type_str);
			}

			ret.result_type = replace_self_refs(result_type_str);
		}

		const int num_params = clang_Cursor_getNumArguments(c);

		if(num_params > 0){
			ret.param_names.reserve(num_params);
			ret.param_types.reserve(num_params);

			for(int i = 0; i < num_params; i++){
				clang::cursor param_cursor = clang_Cursor_getArgument(c, i);

				auto param_name = param_cursor.spelling();
				auto param_type = param_cursor.type();
				auto param_type_str = param_type.spelling();

				if(param_type.kind() == CXType_Typedef){
					param_type_str = fmt::format("typename {}", param_type_str);
				}

				ret.param_names.emplace_back(std::move(param_name));
				ret.param_types.emplace_back(replace_self_refs(param_type_str));
			}
		}

		return ret;
	}

	std::optional<class_info> parse_class_decl(const fs::path &path, info_map &infos, clang::cursor c, namespace_info *ns);

	std::optional<class_base_info> parse_class_base(const fs::path &path, info_map &infos, clang::cursor c, class_info *cls){
		if(c.kind() != CXCursor_CXXBaseSpecifier){
			return std::nullopt;
		}

		clang::type base_type = c.type();
		auto base_type_str = base_type.spelling();

		clang::cursor base_decl = clang_getTypeDeclaration(base_type);
		if(!clang_isInvalid(base_decl.kind())){
			base_type_str = fmt::format("{}::{}", resolve_namespaces(base_decl), base_decl.spelling());
		}

		class_base_info base;
		base.name = std::move(base_type_str);

		auto toks = c.tokens();
		auto toks_begin = std::make_reverse_iterator(toks.end());

		auto num_tmpl_args = clang_Type_getNumTemplateArguments(base_type);

		std::unordered_map<std::string_view, const template_param_info*> param_map;
		for(auto &&param : cls->template_params){
			param_map[param.name] = &param;
		}

		if(num_tmpl_args > 0){
			base.name += "<";

			for(int i = 0; i < num_tmpl_args; i++){
				clang::type arg_type = clang_Type_getTemplateArgumentAsType(base_type, i);

				base.name += fmt::format("{}, ", arg_type.spelling());

				auto res = param_map.find(arg_type.spelling());
				if(res != param_map.end()){
					if(res->second->is_variadic){
						base.is_variadic = true;
						break;
					}
				}
			}

			base.name.erase(base.name.size() - 2);
			base.name += ">";
		}

		switch(clang_getCXXAccessSpecifier(c)){
			case CX_CXXPublic:{
				base.access = access_kind::public_;
				break;
			}

			case CX_CXXProtected:{
				base.access = access_kind::protected_;
				break;
			}

			default:{
				base.access = access_kind::private_;
				break;
			}
		}

		clang::cursor base_decl_c = clang_getTypeDeclaration(base_type);

		if(base_decl_c.is_null()){
			print_parse_error(path, "Failed to get base class declaration for '{}'", c.spelling());
		}
		else if(base_decl_c.kind() == CXCursor_ClassTemplate){
			auto base_decl_opt = detail::parse_class_decl(path, infos, base_decl_c, &infos.global);
		}

		return base;
	}

	std::optional<class_info> parse_class_decl(
		const fs::path &path, info_map &infos,
		clang::cursor c, namespace_info *ns
	){
		const bool is_template =
			c.kind() == CXCursor_ClassTemplate ||
			c.kind() == CXCursor_ClassTemplatePartialSpecialization;

		if(!is_template && c.kind() != CXCursor_ClassDecl){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXInvalidAccessSpecifier && access != CX_CXXPublic){
			return std::nullopt;
		}

		auto class_name = c.spelling();
		auto class_type = c.type();
		auto class_type_str = class_type.spelling();

		class_info ret;

		{
			auto toks = c.tokens();
			auto toks_begin = toks.begin();

			++toks_begin; // skip class/struct keyword

			ret.attributes = parse_attribs(path, infos, toks_begin, toks.end());
		}

		ret.ns = ns;
		ret.name = ns->name + "::" + class_name;
		ret.is_abstract = clang_CXXRecord_isAbstract(c);
		ret.is_template = is_template;
		ret.is_specialization = c.kind() == CXCursor_ClassTemplatePartialSpecialization;

		std::size_t ctor_idx = 0, member_idx = 0, method_idx = 0;

		bool in_template = is_template;

		c.visit_children([&](clang::cursor inner, clang::cursor){
			if(in_template){
				switch(inner.kind()){
					case CXCursor_TemplateTypeParameter:{
						template_param_info param;

						auto inner_toks = inner.tokens();

						auto toks_begin = inner_toks.begin();
						auto toks_end = inner_toks.end();

						auto toks_it = toks_begin;

						param.declarator = toks_it->str();

						++toks_it;

						if(toks_it != toks_end){
							param.is_variadic = toks_it->str() == "...";
						}
						else{
							param.is_variadic = false;
						}

						param.name = inner.spelling();

						ret.template_params.emplace_back(std::move(param));

						return;
					}

					case CXCursor_TemplateTemplateParameter:{
						template_param_info param;

						print_parse_error(path, "template template parameter '{}' for '{}'; not currently supported", inner.spelling(), ret.name);

						break;
					}

					default:{
						in_template = false;
						break;
					}
				}
			}

			if(auto base_opt = parse_class_base(path, infos, inner, &ret); base_opt){
				ret.bases.emplace_back(std::move(*base_opt));
			}
			else if(auto class_decl = parse_class_decl(path, infos, inner, ns); class_decl){
				auto ptr = store_info(infos, std::move(*class_decl));
				ret.classes[ptr->name] = ptr;
			}
			else if(auto ctor = parse_class_ctor(path, infos, inner, &ret); ctor){
				auto ptr = store_info(infos, std::move(*ctor));
				ret.ctors.emplace_back(ptr);
			}
			else if(auto dtor = parse_class_dtor(path, infos, inner, ns); dtor){
				auto ptr = store_info(infos, std::move(*dtor));
				ret.dtor = ptr;
			}
			else if(auto method = parse_class_method(path, infos, inner, &ret); method){
				auto ptr = store_info(infos, std::move(*method));
				ret.methods[ptr->name].emplace_back(ptr);
			}
			else if(auto member = parse_class_member(path, infos, inner, &ret); member){
				ret.members.emplace_back(std::move(*member));
			}

			// TODO: handle access kinds

			// skip anything else
		});

		if(ret.is_specialization){
			auto num_spec_params = clang_Type_getNumTemplateArguments(class_type);
			for(int i = 0; i < num_spec_params; i++){
				clang::type spec_param_type = clang_Type_getTemplateArgumentAsType(class_type, i);
				clang::cursor spec_param_decl = clang_getTypeDeclaration(spec_param_type);
				const auto namespaces = resolve_namespaces(spec_param_decl);
				ret.template_args.emplace_back(fmt::format("{}::{}", namespaces, spec_param_type.spelling()));
			}

			std::size_t tmpl_param_idx = 0;
			for(const auto &template_param : ret.template_params){
				const auto param_alias = fmt::format("type-parameter-0-{}", tmpl_param_idx++);
				for(std::string &template_arg : ret.template_args){
					auto alias_pos = template_arg.find(param_alias);
					while(alias_pos != std::string::npos){
						template_arg.replace(alias_pos, param_alias.size(), template_param.name);
						alias_pos = template_arg.find(param_alias);
					}
				}
			}

			std::string specialization_params;

			for(auto &&template_arg : ret.template_args){
				specialization_params += fmt::format("{}, ", template_arg);
			}

			if(!specialization_params.empty()){
				specialization_params.erase(specialization_params.size() - 2);
			}
		}

		return ret;
	}

	std::optional<enum_info> parse_enum_decl(const fs::path &path, info_map &infos, clang::cursor c, namespace_info *ns){
		if(c.kind() != CXCursor_EnumDecl){
			return std::nullopt;
		}

		enum_info ret;

		ret.name = ns->name + "::" + c.spelling();
		ret.is_scoped = clang_EnumDecl_isScoped(c);

		c.visit_children([&](clang::cursor value_c, clang::cursor){
			if(value_c.kind() != CXCursor_EnumConstantDecl){
				return;
			}

			enum_value_info value;
			value.name = value_c.spelling();
			value.value = clang_getEnumConstantDeclUnsignedValue(value_c);
			ret.values.emplace_back(std::move(value));
		});

		return ret;
	}

	std::optional<function_info> parse_function_decl(const fs::path &path, info_map &infos, clang::cursor c, namespace_info *ns){
		if(c.kind() != CXCursor_FunctionDecl){
			return std::nullopt;
		}

		auto def_cursor = clang_getCursorDefinition(c);
		if(!clang_Cursor_isNull(def_cursor)){
			c = def_cursor;
		}

		function_info ret;

		ret.name = ns->name + "::" + c.spelling();

		auto fn_type = c.type();
		auto fn_type_str = fn_type.spelling();

		{
			auto result_type = clang_getResultType(fn_type);
			auto result_type_str = clang::detail::convert_str(clang_getTypeSpelling(result_type));
			ret.result_type = std::move(result_type_str);
		}

		int num_params = clang_Cursor_getNumArguments(c);

		if(num_params > 0){
			ret.param_names.reserve(num_params);
			ret.param_types.reserve(num_params);

			for(int i = 0; i < num_params; i++){
				clang::cursor arg_cursor = clang_Cursor_getArgument(c, i);
				auto arg_type = arg_cursor.type();

				auto arg_name = arg_cursor.spelling();
				auto arg_type_name = arg_type.spelling();

				ret.param_names.emplace_back(std::move(arg_name));
				ret.param_types.emplace_back(std::move(arg_type_name));
			}
		}

		return ret;
	}

	entity_info *parse_namespace_inner(const fs::path &path, info_map &infos, clang::cursor c, namespace_info *ns){
		auto res = try_parse(path, infos, c, ns);
		if(!res) return nullptr;

		entity_info *ret = nullptr;

		std::visit(
			meta::overload(
				[&](function_info &fn_info){
					auto ptr = store_info(infos, std::move(fn_info));
					ret = ptr;
					ns->functions[ptr->name].emplace_back(ptr);
				},
				[&](class_info &cls){
					auto ptr = store_info(infos, std::move(cls));
					ret = ptr;
					ns->classes[ptr->name] = ptr;
				},
				[&](enum_info &enm){
					auto ptr = store_info(infos, std::move(enm));
					ret = ptr;
					ns->enums[ptr->name] = ptr;
				},
				[&](namespace_info &child_ns){
					auto ptr = store_info(infos, std::move(child_ns));
					ret = ptr;
					infos.namespaces[ptr->name] = ptr;
					ns->namespaces[ptr->name] = ptr;
				},
				[&](type_alias_info &alias){
					auto ptr = store_info(infos, std::move(alias));
					ret = ptr;
					ns->aliases[ptr->name] = ptr;
				}
			),
			*res
		);

		return ret;
	}

	std::optional<type_alias_info> parse_type_alias(const fs::path &path, info_map &infos, clang::cursor c, namespace_info *ns){
		if(c.kind() != CXCursor_TypeAliasDecl && c.kind() != CXCursor_TypedefDecl){
			return std::nullopt;
		}

		auto access = clang_getCXXAccessSpecifier(c);
		if(access != CX_CXXInvalidAccessSpecifier && access != CX_CXXPublic){
			return std::nullopt;
		}

		type_alias_info ret;

		ret.name = ns->name + "::" + c.spelling();

		auto type = clang_getTypedefDeclUnderlyingType(c);

		ret.aliased = clang::detail::convert_str(clang_getTypeSpelling(type));

		return ret;
	}

	std::optional<namespace_info> parse_namespace(const fs::path &path, info_map &infos, clang::cursor c, namespace_info *ns){
		if(c.kind() != CXCursor_Namespace){
			return std::nullopt;
		}

		auto inner_name = ns->name + "::" + c.spelling();
		auto inner_res = infos.namespaces.find(inner_name);

		if(inner_res != infos.namespaces.end()){
			auto inner = inner_res->second;

			c.visit_children(
				[&](clang::cursor child, clang::cursor){
					parse_namespace_inner(path, infos, child, inner);
				}
			);

			return std::nullopt;
		}
		else{
			namespace_info ret;

			ret.ns = ns;
			ret.name = std::move(inner_name);

			c.visit_children(
				[&](clang::cursor child, clang::cursor){
					parse_namespace_inner(path, infos, child, &ret);
				}
			);

			return ret;
		}
	}

	std::optional<entity> try_parse(const fs::path &path, info_map &infos, clang::cursor c, namespace_info *ns){
		if(!clang_Location_isFromMainFile(clang_getCursorLocation(c))){
			return std::nullopt;
		}
		else if(auto fn_decl = detail::parse_function_decl(path, infos, c, ns); fn_decl){
			return std::make_optional<entity>(std::move(*fn_decl));
		}
		else if(auto class_decl = detail::parse_class_decl(path, infos, c, ns); class_decl){
			return std::make_optional<entity>(std::move(*class_decl));
		}
		else if(auto enum_decl = detail::parse_enum_decl(path, infos, c, ns); enum_decl){
			return std::make_optional<entity>(std::move(*enum_decl));
		}
		else if(auto fn_decl = detail::parse_function_decl(path, infos, c, ns); fn_decl){
			return std::make_optional<entity>(std::move(*fn_decl));
		}
		else if(auto ns_decl = detail::parse_namespace(path, infos, c, ns); ns_decl){
			return std::make_optional<entity>(std::move(*ns_decl));
		}
		else if(auto alias = detail::parse_type_alias(path, infos, c, ns); alias){
			return std::make_optional<entity>(std::move(*alias));
		}

		return std::nullopt;
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

	auto include_dirs = info.all_include_dirs();
	auto options = info.file_options(path);

	options.emplace_back("-DMETACPP_TOOL_RUN");

	bool standard_given = false;

	while(1){
		auto res = std::find_if(
			options.begin(), options.end(),
			[](auto &&opt){ return std::string_view(opt).substr(0, 2) == "-I"; }
		);
		if(res == options.end()) break;
		else options.erase(res);
	}

	for(auto &&opt : options){
		if(std::string_view(opt).substr(0, 5) == "-std="){
			standard_given = true;
		}
	}

	for(auto &&dir : include_dirs){
		options.emplace_back(fmt::format("-I{}", dir.c_str()));
	}

	if(!standard_given){
		options.insert(options.begin(), "-std=c++17");
	}

	options.emplace_back("-Wno-ignored-optimization-argument");

	clang::translation_unit tu(index, path, options);

	const unsigned int num_diag = clang_getNumDiagnostics(tu);

	bool found_err = false;

	for(unsigned int i = 0; i < num_diag; i++){
		CXDiagnostic diagnotic = clang_getDiagnostic(tu, i);

		auto err_str = clang::detail::convert_str(clang_formatDiagnostic(diagnotic, clang_defaultDiagnosticDisplayOptions()));

		if(err_str.find("error:") != std::string::npos){
			found_err = true;
			fmt::print(stderr, "{}\n", err_str);
		}
	}

	if(found_err){
		throw std::runtime_error("AST parsing failed with errors");
	}

	info_map ret;
	ret.global.ns = nullptr;

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

		auto entity = ast::detail::parse_namespace_inner(path, ret, cursor, &ret.global);
		if(!entity){
			//ast::detail::print_parse_warning(path, "Unrecognized cursor '{}' of kind '{}'", cursor.spelling(), cursor.kind_spelling());
		}
	};

	tu.get_cursor().visit_children(visitor);

	return ret;
}
