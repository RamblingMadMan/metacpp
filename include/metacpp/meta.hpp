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

#ifndef METACPP_META_HPP
#define METACPP_META_HPP 1

#include <string_view>
#include <functional>

#include "fmt/format.h"

#include "metacpp/config.hpp"

/**
 * @defgroup Meta Metaprogramming utilities
 * @{
 */

namespace metapp{
	namespace detail{
		template<std::size_t, typename...>
		struct types_type_helper;

		template<typename T, typename ... Ts>
		struct types_type_helper<std::size_t(0), T, Ts...>{
			using type = T;
		};

		template<std::size_t I, typename T, typename ... Ts>
		struct types_type_helper<I, T, Ts...>: types_type_helper<I-1, Ts...>{};

		template<typename U, typename ... Ts>
		struct types_contains_helper;

		template<typename U>
		struct types_contains_helper<U>{
			static constexpr auto value = false;
		};

		template<typename T, typename ... Ts>
		struct types_contains_helper<T, T, Ts...>{
			static constexpr auto value = true;
		};

		template<typename U, typename T, typename ... Ts>
		struct types_contains_helper<U, T, Ts...>: types_contains_helper<U, Ts...>{};

		// value helpers

		template<std::size_t, auto...>
		struct values_value_helper;

		template<auto X, auto ... Xs>
		struct values_value_helper<0, X, Xs...>{
			static constexpr auto value = X;
		};

		template<std::size_t I, auto X, auto ... Xs>
		struct values_value_helper<I, X, Xs...>: values_value_helper<I-1, Xs...>{};

		template<auto Y, auto ... Xs>
		struct values_contains_helper;

		template<auto Y>
		struct values_contains_helper<Y>{
			static constexpr auto value = false;
		};

		template<auto X, auto ... Xs>
		struct values_contains_helper<X, X, Xs...>{
			static constexpr auto value = true;
		};

		template<auto Y, auto X, auto ... Xs>
		struct values_contains_helper<Y, X, Xs...>
			: std::conditional_t<X == Y, values_contains_helper<X, X>, values_value_helper<Y, Xs...>>
		{};
	}

	/**
	 * @brief Compile-time list of types.
	 */
	template<typename ... Ts>
	struct types{
		static constexpr auto size = sizeof...(Ts);

		/**
		 * @brief Retrieve a type by index.
		 */
		template<std::size_t I>
		using type = typename detail::types_type_helper<I, Ts...>::type;

		/**
		 * @brief Check if the list contains a type.
		 */
		template<typename U>
		static constexpr auto contains = detail::types_contains_helper<U, Ts...>::value;

		template<typename ... Us>
		constexpr bool operator==(types<Us...>) const noexcept{ return false; }

		constexpr bool operator==(types<Ts...>) const noexcept{ return true; }

		template<typename ... Us>
		constexpr bool operator!=(types<Us...>) const noexcept{ return true; }

		constexpr bool operator!=(types<Ts...>) const noexcept{ return false; }
	};

	/**
	 * @brief Convienience alias for 1-tuple of types.
	 */
	template<typename T>
	using type = types<T>;

	/**
	 * @brief Convienience function for retrieving a type from a list by index.
	 */
	template<typename Types, std::size_t I = 0>
	using get_t = typename Types::template type<I>;

	/**
	 * @brief Compile-time list of values.
	 */
	template<auto ... Xs>
	struct values{
		static constexpr auto size = sizeof...(Xs);

		/**
		 * @brief Retrieve a value by index.
		 */
		template<std::size_t I>
		static constexpr auto value = detail::values_value_helper<I, Xs...>::value;

		/**
		 * @brief Check if the list contains a value.
		 */
		template<auto Y>
		static constexpr auto contains = detail::values_contains_helper<Y, Xs...>::value;

		template<auto ... Ys>
		constexpr bool operator==(values<Ys...>) const noexcept{ return ((Xs == Ys) && ...); }

		constexpr bool operator==(values<Xs...>) const noexcept{ return true; }

		template<auto ... Ys>
		constexpr bool operator!=(values<Ys...>) const noexcept{ return ((Xs != Ys) || ...); }

		constexpr bool operator!=(values<Xs...>) const noexcept{ return false; }
	};

	/**
	 * @brief Convienience alias for 1-tuple of values.
	 */
	template<auto X>
	using value = values<X>;

	/**
	 * @brief Convienience function for getting a value by index.
	 */
	template<typename Values, std::size_t I = 0>
	inline constexpr auto get_v = Values::template value<I>;

	namespace detail{
		template<typename ... Ls>
		struct join_helper;

		template<typename ... Ts, typename ... Us>
		struct join_helper<types<Ts...>, types<Us...>>{
			using type = types<Ts..., Us...>;
		};

		template<typename L, typename T>
		struct append_helper;

		template<typename ... Ts, typename U>
		struct append_helper<types<Ts...>, U>{
			using type = types<Ts..., U>;
		};
	}

	template<typename L, typename T>
	using append = typename detail::append_helper<L, T>::type;

	template<typename ... Ls>
	using join = typename detail::join_helper<Ls...>::type;

	/**
	 * @brief Iterate all types in a type list.
	 * @param f Template function to apply to each type.
	 */
	template<typename ... Ts, typename Fn>
	void for_all(types<Ts...>, Fn &&f){
		(std::forward<Fn>(f).template operator()<Ts>(), ...);
	}

	/**
	 * @brief Iterate all values in a value list.
	 * @param f Template function to apply to each value.
	 */
	template<auto ... Xs, typename Fn>
	void for_all(values<Xs...>, Fn &&f){
		(std::forward<Fn>().template operator()<Xs>(), ...);
	}

	template<typename Ts, typename Fn>
	void for_all(Fn &&f){
		for_all(Ts{}, std::forward<Fn>(f));
	}

	namespace detail{
		template<typename ... Fs>
		struct overload_helper: Fs...{
			template<typename ... Gs>
			overload_helper(Gs &&... gs)
				: Fs(std::forward<Gs>(gs))...
			{}

			using Fs::operator()...;
		};

		template<typename ... Fs>
		overload_helper(Fs &&...) -> overload_helper<std::decay_t<Fs>...>;
	}

	template<typename F, typename ... Fs>
	auto overload(F &&f, Fs &&... fs){
		return detail::overload_helper(std::forward<F>(f), std::forward<Fs>(fs)...);
	}

#if __cplusplus >= 202002L
	/**
	 * @brief Compile-time constant string.
	 */
	template<std::size_t N>
	struct fixed_str{
		public:
			constexpr fixed_str(const char(&arr)[N + 1]) noexcept
				: fixed_str(std::make_index_sequence<N>(), 0, arr)
			{}

			constexpr fixed_str(const fixed_str &other) noexcept = default;

			constexpr operator std::string_view() const noexcept{ return std::string_view(data, size); }

			constexpr auto substr(std::size_t from, std::size_t len) const noexcept{
				constexpr std::size_t from_clamped = std::min(N, from);
				constexpr std::size_t rem = N - from_clamped;
				constexpr std::size_t len_clamped = std::min(rem, len);
				return fixed_str<len_clamped>(std::make_index_sequence<len_clamped>(), from_clamped, data);
			}

			template<std::size_t M>
			bool operator==(const fixed_str<M> &other) const noexcept{
				if constexpr(M == N){
					if(this == &other) return true;
					return std::string_view(*this) == std::string_view(other);
				}
				else{
					return false;
				}
			}

			template<std::size_t M>
			bool operator!=(const fixed_str<M> &other) const noexcept{
				if constexpr(M == N){
					if(this != &other){
						return std::string_view(*this) != std::string_view(other);
					}
				}

				return true;
			}

			static constexpr std::size_t size = N;
			const char data[N + 1];

		private:
			template<std::size_t M, std::size_t ... Is>
			constexpr fixed_str(std::index_sequence<Is...>, std::size_t offset, const char(&arr)[M]) noexcept
				: data{arr[offset + Is]..., '\0'}
			{}
	};

	template<std::size_t N>
	fixed_str(const char(&)[N]) -> fixed_str<N-1>;

	template<std::size_t N>
	inline constexpr auto str(const char(&str)[N]){ return fixed_str(str); }
#endif

	namespace detail{
		template<typename T>
		constexpr auto get_type_name(){
			using namespace std::string_view_literals;
			#if defined(__clang__)
			constexpr std::string_view prefix = "[T = "sv;
			constexpr std::string_view suffix = "]"sv;
			constexpr std::string_view function = std::string_view(__PRETTY_FUNCTION__);
			#elif defined(__GNUC__)
			constexpr std::string_view prefix = "with T = "sv;
			constexpr std::string_view suffix = "]"sv;
			constexpr std::string_view function = std::string_view(__PRETTY_FUNCTION__);
			#elif defined(_MSC_VER)
			constexpr std::string_view prefix = "get_type_name<"sv; // NOTE: this is probably missing namespaces
			constexpr std::string_view suffix = ">(void)"sv;
			constexpr std::string_view function = std::string_view(__FUNCSIG__);
			#else
			#error "Unrecognized compiler"
			#endif

			static_assert(!function.empty());

			constexpr auto prefix_index = function.find(prefix);

			static_assert(prefix_index != std::string_view::npos);

			constexpr auto name_idx = prefix_index + prefix.size();
			constexpr auto name_end_idx = function.rfind(suffix);

			static_assert(name_idx < name_end_idx, "Compiler error in meta::detail::getTypeName");

			constexpr std::size_t name_len = name_end_idx - name_idx;

			return function.substr(name_idx, name_len);
		}
	}

	/**
	 * @brief Get the pretty name of a type.
	 */
	template<typename T>
	inline constexpr std::string_view type_name = detail::get_type_name<T>();

	namespace detail{
		template<typename Ent, std::size_t Idx>
		struct attrib_info_data;

		template<typename Ent, std::size_t Idx>
		struct param_info_data;

		template<typename Class, std::size_t Idx>
		struct class_ctor_info_data;

		template<typename Class>
		struct class_dtor_info_data;

		template<typename Class, std::size_t MethodIdx, std::size_t Idx>
		struct class_method_param_info_data;

		template<typename Class, std::size_t Idx>
		struct class_method_info_data;

		template<typename Class>
		struct class_info_data;

		template<typename Enum>
		struct enum_info_data;
	}

	template<typename Ent, std::size_t Idx>
	struct attrib_info{
		static constexpr std::string_view scope = detail::attrib_info_data<Ent, Idx>::scope;
		static constexpr std::string_view name = detail::attrib_info_data<Ent, Idx>::name;

		using args = typename detail::attrib_info_data<Ent, Idx>::args;
	};

	template<typename Ent, std::size_t Idx>
	struct param_info{
		static constexpr std::string_view name = detail::param_info_data<Ent, Idx>::name;

		using type = typename detail::param_info_data<Ent, Idx>::type;
	};

	template<typename Class, std::size_t Idx>
	struct class_ctor_info{
		static constexpr bool is_move_ctor = detail::class_ctor_info_data<Class, Idx>::is_move_ctor;
		static constexpr bool is_copy_ctor = detail::class_ctor_info_data<Class, Idx>::is_copy_ctor;
		static constexpr bool is_default_ctor = detail::class_ctor_info_data<Class, Idx>::is_default_ctor;

		static constexpr std::size_t num_params = detail::class_ctor_info_data<Class, Idx>::num_params;
		//using param_types = typename detail::param_info_data<class_ctor_info<Class, Idx>, Idx>::param_types;
	};

	template<typename Class>
	struct class_dtor_info{
		static constexpr bool is_virtual = std::has_virtual_destructor_v<Class>;
		//static constexpr bool is_defaulted = detail::class_dtor_info_data<Class>::is_defaulted;
	};

	template<typename Class, std::size_t MethodIdx, std::size_t Idx>
	struct class_method_param_info{
		static constexpr std::string_view name = detail::class_method_param_info_data<Class, MethodIdx, Idx>::name;
		using type = typename detail::class_method_param_info_data<Class, MethodIdx, Idx>::type;
	};

	template<typename Class, std::size_t Idx>
	struct class_method_info{
		static constexpr std::string_view name = detail::class_method_info_data<Class, Idx>::name;
		static constexpr bool is_virtual = detail::class_method_info_data<Class, Idx>::is_virtual;
		static constexpr auto ptr = detail::class_method_info_data<Class, Idx>::ptr;

		static constexpr std::size_t num_params = detail::class_method_info_data<Class, Idx>::num_params;

		using result = typename detail::class_method_info_data<Class, Idx>::result;
		using param_types = typename detail::class_method_info_data<Class, Idx>::param_types;
		using params = typename detail::class_method_info_data<Class, Idx>::params;
	};

	struct ignore{
		template<typename T>
		constexpr bool operator==(T&&) const noexcept{ return true; }

		template<typename T>
		constexpr bool operator!=(T&&) const noexcept{ return false; }
	};

#if __cplusplus >= 202002L
	namespace detail{
		template<typename Signature, auto Name, typename Infos, typename ... Results>
		struct query_method_helper;

		template<fixed_str Name, typename MethodInfo, typename ... MethodInfos, typename ... Results>
		struct query_method_helper<ignore, Name, types<MethodInfo, MethodInfos...>, Results...>
			: std::conditional_t<
				MethodInfo::name == Name,
				query_method_helper<ignore, Name, types<MethodInfos...>, Results..., MethodInfo>,
				query_method_helper<ignore, Name, types<MethodInfos...>, Results...>
			>
		{};

		template<
			typename Ret, typename ... Params,
			fixed_str Name,
			typename MethodInfo, typename ... MethodInfos,
			typename ... Results
		>
		struct query_method_helper<Ret(Params...), Name, types<MethodInfo, MethodInfos...>, Results...>{
			private:
				static constexpr auto dummy_query(){
					if constexpr(MethodInfo::name == Name){
						if constexpr(std::is_same_v<Ret, ignore> || std::is_same_v<Ret, typename MethodInfo::result>){
							if constexpr(types<Params...>{} == types<ignore>{} || types<Params...>{} == typename MethodInfo::param_types{}){
								return typename query_method_helper<Ret(Params...), Name, types<MethodInfos...>, Results..., MethodInfo>::type{};
							}
							else{
								return typename query_method_helper<Ret(Params...), Name, types<MethodInfos...>, Results...>::type{};
							}
						}
						else{
							return typename query_method_helper<Ret(Params...), Name, types<MethodInfos...>, Results...>::type{};
						}
					}
					else{
						return typename query_method_helper<Ret(Params...), Name, types<MethodInfos...>, Results...>::type{};
					}
				}

			public:
				using type = decltype(dummy_query());
		};

		template<typename Signature, fixed_str Name, typename ... Results>
		struct query_method_helper<Signature, Name, types<>, Results...>{
			using type = types<Results...>;
		};
	}
#endif

	template<typename Class>
	struct class_info{
		static constexpr std::string_view name = type_name<Class>;

		static constexpr bool is_abstract = std::is_abstract_v<Class>;

		static constexpr std::size_t num_methods = detail::class_info_data<Class>::num_methods;

		using ctors = typename detail::class_info_data<Class>::ctors;
		using methods = typename detail::class_info_data<Class>::methods;
		using members = typename detail::class_info_data<Class>::members;

#if __cplusplus >= 202002L
		template<fixed_str Name, typename Signature = ignore>
		using query_method = typename detail::query_method_helper<Signature, Name, methods>::type;
#endif
	};

#if __cplusplus >= 202002L
	template<typename Class, fixed_str Name, typename Signature = ignore>
	using query_method = typename detail::query_method_helper<Signature, Name, typename class_info<Class>::methods>::type;
#endif

	template<typename Class, std::size_t Idx>
	using class_ctor = get_t<typename class_info<Class>::ctors, Idx>;

	template<typename Class>
	using class_dtor = typename class_info<Class>::dtor;

	template<typename Class, std::size_t Idx>
	using class_method = get_t<typename class_info<Class>::methods, Idx>;

	template<typename Class>
	using methods = typename class_info<Class>::methods;

	template<typename Enum>
	struct enum_info{
		static constexpr std::string_view name = detail::enum_info_data<Enum>::name;
		static constexpr std::size_t num_values = detail::enum_info_data<Enum>::num_values;

		using value_names = typename detail::enum_info_data<Enum>::value_names;
		using values = typename detail::enum_info_data<Enum>::values;
	};

	template<typename Enum>
	inline constexpr std::size_t num_enum_values = enum_info<Enum>::num_values;

	template<typename Enum, std::size_t Idx>
	inline constexpr std::size_t enum_value = get_v<typename enum_info<Enum>::values, Idx>;
}

namespace METACPP_META_NAMESPACE = metapp;

/**
 * @}
 */

#endif // !METACPP_META_HPP
