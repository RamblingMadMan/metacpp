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

#ifndef METACPP_REFL_HPP
#define METACPP_REFL_HPP 1

#include "metacpp/config.hpp"
#include "meta.hpp"

namespace reflpp{
	namespace detail{
		struct type_info_helper;
		struct class_info_helper;
		struct enum_info_helper;

		struct class_member_helper;
		struct class_method_helper;
		struct class_variable_helper;

		struct enum_value_helper;

		template<typename T, typename = void>
		struct reflect_helper;
	};

	using type_info = const detail::type_info_helper*;
	using class_info = const detail::class_info_helper*;
	using enum_info = const detail::enum_info_helper*;

	using class_method_info = const detail::class_method_helper*;
	using class_variable_info = const detail::class_variable_helper*;

	type_info reflect(std::string_view name);
	class_info reflect_class(std::string_view name);
	enum_info reflect_enum(std::string_view name);

	template<typename T>
	auto reflect(){
		static auto ret = detail::reflect_helper<T>::reflect();
		return ret;
	}

	struct args_pack_base{
		virtual std::size_t size() const noexcept = 0;
		virtual void *arg(std::size_t idx) const noexcept = 0;
		virtual type_info arg_type(std::size_t idx) const noexcept = 0;
	};

	template<typename ... Args>
	struct args_pack: args_pack_base{
		public:
			template<typename ... UArgs>
			explicit args_pack(UArgs &&... args)
				: args_pack(std::make_index_sequence<sizeof...(Args)>(), std::forward<UArgs>(args)...)
			{}

			std::size_t size() const noexcept override{ return sizeof...(Args); }
			void *arg(std::size_t idx) const noexcept{ return idx >= size() ? nullptr : m_ptrs[idx]; }
			type_info arg_type(std::size_t idx) const noexcept{ return idx >= size() ? nullptr : m_types[idx]; }

		private:
			template<std::size_t ... Is, typename ... UArgs>
			args_pack(std::index_sequence<Is...>, UArgs &&... args)
				: m_vals(std::forward_as_tuple(std::forward<UArgs>(args)...))
				, m_ptrs{ &std::get<Is>(m_vals)... }
				, m_types{ reflect<std::tuple_element_t<Is, decltype(m_vals)>>()... }
			{}

			std::tuple<Args...> m_vals;
			void *m_ptrs[sizeof...(Args)];
			type_info m_types[sizeof...(Args)];
	};

	namespace detail{
		struct type_info_helper{
			virtual std::string_view name() const noexcept = 0;
			virtual std::size_t size() const noexcept = 0;
			virtual std::size_t alignment() const noexcept = 0;
		};

		struct class_member_helper{};

		struct class_method_helper{
			virtual type_info result_type() const noexcept = 0;
			virtual std::size_t num_params() const noexcept = 0;
			virtual std::string_view param_name(std::size_t idx) const noexcept = 0;
			virtual type_info param_type(std::size_t idx) const noexcept = 0;
		};

		struct class_variable_helper{};

		struct enum_value_helper{
			virtual std::uint64_t value() const noexcept = 0;
		};

		struct class_info_helper: type_info_helper{
			virtual std::size_t num_methods() const noexcept = 0;
			virtual const class_method_helper *method(std::size_t idx) const noexcept = 0;

			virtual std::size_t num_bases() const noexcept = 0;
			virtual class_info base(std::size_t i) const noexcept = 0;

			virtual void *cast_to_base(void *self, std::size_t idx) const noexcept = 0;
		};

		struct enum_info_helper: type_info_helper{
			virtual std::size_t num_values() const noexcept = 0;
			virtual const enum_value_helper *value(std::size_t idx) const noexcept = 0;
		};

		template<typename T, typename Helper>
		struct info_helper_base: Helper{
			std::string_view name() const noexcept override{ return metapp::type_name<T>; }
			std::size_t size() const noexcept override{ return sizeof(T); }
			std::size_t alignment() const noexcept override{ return alignof(T); }
		};

		template<typename T>
		struct reflect_helper<T, std::enable_if_t<std::is_class_v<T>>>{
			static class_info reflect(){ return reflect_class(metapp::type_name<T>); }
		};

		template<typename T>
		struct reflect_helper<T, std::enable_if_t<std::is_enum_v<T>>>{
			static enum_info reflect(){ return reflect_enum(metapp::type_name<T>); }
		};

		template<typename T>
		REFLCPP_API extern type_info type_export();

		using type_export_fn = type_info(*)();
	}
}

namespace METACPP_REFL_NAMESPACE = reflpp;

#endif // !METACPP_REFL_HPP
