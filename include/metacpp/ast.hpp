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
		class_, function, namespace_, type, type_alias,

		class_constructor,
		class_destructor,
		class_member,
		class_method
	};

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
	};

	struct type_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::type; }
	};

	struct class_member_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::class_member; }

		std::size_t index;

		std::string type;
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

	struct class_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::class_; }

		bool is_abstract;

		std::vector<const class_info*> bases;
		std::unordered_map<std::string, std::vector<const class_method_info*>> methods;
		std::unordered_map<std::string, const class_member_info*> members;
		std::unordered_map<std::string, const class_info*> classes;
		std::vector<const class_constructor_info*> ctors;
		const class_destructor_info *dtor = nullptr;
	};

	struct function_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::function; }
	};

	struct type_alias_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::type_alias; }

		std::string aliased;
	};

	struct namespace_info: entity_info{
		entity_kind kind() const noexcept override{ return entity_kind::namespace_; }

		std::unordered_map<std::string, class_info*> classes;
		std::unordered_map<std::string, std::vector<function_info*>> functions;
		std::unordered_map<std::string, namespace_info*> namespaces;
		std::unordered_map<std::string, type_alias_info*> aliases;
	};

	using entity = std::variant<class_info, function_info, type_alias_info, namespace_info>;

	struct info_map{
		namespace_info global;
		std::vector<std::unique_ptr<entity_info>> storage;
	};

	enum class cppstd{
		_11, _14, _17, _20
	};

	class compile_info{
		public:
			explicit compile_info(const std::filesystem::path &build_dir);
			~compile_info();

			std::vector<std::string> file_options(const std::filesystem::path &path) const;

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
