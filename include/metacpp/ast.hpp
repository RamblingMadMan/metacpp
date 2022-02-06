/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef METACPP_AST_HPP
#define METACPP_AST_HPP 1

#include <vector>
#include <variant>
#include <unordered_map>
#include <filesystem>

#include "metacpp/config.hpp"

/**
 * @defgroup Ast AST parsing utilities.
 * @{
 */

namespace astpp{
	class attribute{
		public:
			attribute(std::string scope_, std::string name_, std::vector<std::string> args_ = {}) noexcept
				: m_scope(std::move(scope_))
				, m_name(std::move(name_))
				, m_args(std::move(args_))
			{}

			attribute(std::string name_, std::vector<std::string> args_ = {}) noexcept
			: attribute({}, std::move(name_), std::move(args_))
			{}

			attribute(attribute&&) noexcept = default;

			attribute &operator=(attribute&&) noexcept = default;

			const std::string &scope() const noexcept{ return m_scope; }
			const std::string &name() const noexcept{ return m_name; }
			const std::vector<std::string> &args() const noexcept{ return m_args; }

			std::string str() const noexcept{
				std::string ret = has_scope() ? m_scope + "::" : "";
				ret += m_name;
				if(has_args()){
					ret += "(" + m_args[0];
					for(std::size_t i = 1; i < m_args.size(); i++){
						ret += ", " + m_args[i];
					}
					ret += ")";
				}
				return ret;
			}

			bool has_scope() const noexcept{ return !m_scope.empty(); }
			bool has_args() const noexcept{ return !m_args.empty(); }

		private:
			std::string m_scope, m_name;
			std::vector<std::string> m_args;
	};

	enum class entity_kind{
		class_, enum_, function, namespace_, type, type_alias,

		template_param,

		class_specialization,
		class_base,
		class_constructor,
		class_destructor,
		class_member,
		class_method,

		enum_value
	};

	enum class access_kind{
		public_, protected_, private_
	};

	class namespace_info;

	struct entity_info{
		entity_info() = default;

		entity_info(const entity_info&) = delete;
		entity_info(entity_info &&other)
			: name(std::move(other.name))
			, attributes(std::move(other.attributes))
		{}

		entity_info &operator=(const entity_info&) = delete;

		entity_info &operator=(entity_info &&other) noexcept{
			if(this != &other){
				name = std::move(other.name);
				attributes = std::move(other.attributes);
			}

			return *this;
		}

		virtual ~entity_info() = default;

		virtual entity_kind kind() const noexcept = 0;

		std::string name;
		std::vector<attribute> attributes;
		namespace_info *ns;
	};

	struct type_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::type; }
	};

	struct function_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::function; }

		std::string type;
		std::string result_type;
		std::vector<std::string> param_types, param_names;
	};

	struct class_member_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::class_member; }

		std::string type;
		bool is_accessable;
	};

	struct class_method_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::class_method; }

		std::size_t index;

		bool is_static;
		bool is_const;
		bool is_virtual;
		bool is_pure_virtual;
		bool is_defaulted;
		bool is_noexcept;
		bool is_accessable;

		std::string result_type;
		std::vector<std::string> param_types, param_names;
	};

	enum class constructor_kind{
		move, copy, default_, converting, generic
	};

	struct class_constructor_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::class_destructor; }

		bool is_noexcept;

		enum constructor_kind constructor_kind;
		std::vector<std::string> param_types, param_names;
	};

	struct class_destructor_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::class_destructor; }

		bool is_override;
	};

	struct template_param_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::template_param; }

		std::string declarator;
		std::string default_value;
		bool is_variadic = false;
	};

	struct class_base_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::class_base; }

		access_kind access;
		bool is_variadic = false;
	};

	struct class_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::class_; }

		bool is_abstract;
		bool is_template;
		bool is_specialization = false;

		std::vector<class_base_info> bases;
		std::unordered_map<std::string, std::vector<class_method_info*>> methods;
		std::vector<class_member_info> members;
		std::unordered_map<std::string, class_info*> classes;
		std::vector<class_constructor_info*> ctors;
		std::vector<template_param_info> template_params;
		std::vector<std::string> template_args;
		const class_destructor_info *dtor = nullptr;
	};

	struct class_specialization: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::class_specialization; }

		class_info *cls;
		std::vector<std::string> template_args;
	};

	struct enum_value_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::enum_value; }

		std::string name;
		std::uint64_t value;
	};

	struct enum_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::enum_; }

		bool is_scoped;

		std::vector<enum_value_info> values;
	};

	struct type_alias_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::type_alias; }

		std::string aliased;
	};

	struct namespace_info: entity_info{
		namespace_info() = default;

		namespace_info(namespace_info&&) noexcept = default;
		namespace_info(const namespace_info&) = delete;

		namespace_info &operator=(namespace_info&&) noexcept = default;

		entity_kind kind() const noexcept override{ return entity_kind::namespace_; }

		std::unordered_map<std::string, class_info*> classes;
		std::unordered_map<std::string, enum_info*> enums;
		std::unordered_map<std::string, std::vector<function_info*>> functions;
		std::unordered_map<std::string, namespace_info*> namespaces;
		std::unordered_map<std::string, type_alias_info*> aliases;
	};

	using entity = std::variant<class_info, enum_info, function_info, type_alias_info, namespace_info>;

	struct info_map{
		namespace_info global;
		std::unordered_map<std::string, namespace_info*> namespaces;
		std::vector<std::unique_ptr<entity_info>> storage;
	};

	enum class cppstd{
		_11, _14, _17, _20
	};

	class compile_info{
		public:
			explicit compile_info(const std::filesystem::path &build_dir);
			~compile_info();

			std::vector<std::string> all_options() const;
			std::vector<std::string> file_options(const std::filesystem::path &path) const;

			std::vector<std::filesystem::path> all_include_dirs() const;
			std::vector<std::filesystem::path> file_include_dirs(const std::filesystem::path &path) const;

			struct data;
			std::unique_ptr<data> impl;
	};

	info_map parse(const std::filesystem::path &path, const compile_info &info);
}

namespace METACPP_AST_NAMESPACE = astpp;

/**
 * @}
 */

#endif // !METACPP_AST_HPP
