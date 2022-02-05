/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef METACPP_META_HPP
#define METACPP_META_HPP 1

#include <stdexcept>
#include <string_view>
#include <functional>

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
		static constexpr bool empty = size == 0;

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
		static constexpr bool empty = size == 0;

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

	template<auto X>
	struct values<X>{
		static constexpr auto size = 1;
		static constexpr bool empty = false;

		constexpr operator decltype(X)() const noexcept{ return X; }

		template<std::size_t I>
		static constexpr std::enable_if_t<I == 0, decltype(X)> value = X;

		template<auto Y>
		static constexpr bool contains = X == Y;

		template<typename T>
		constexpr bool operator==(const T &other) const noexcept{ return X == other; }

		template<auto Y>
		constexpr bool operator==(values<Y>) const noexcept{ return X == Y; }

		template<auto ... Ys>
		constexpr bool operator==(values<Ys...>) const noexcept{ return false; }

		constexpr bool operator==(values<X>) const noexcept{ return true; }

		template<typename T>
		constexpr bool operator!=(const T &other) const noexcept{ return X != other; }

		template<auto Y>
		constexpr bool operator!=(values<Y>) const noexcept{ return X != Y; }

		template<auto ... Ys>
		constexpr bool operator!=(values<Ys...>) const noexcept{ return true; }

		constexpr bool operator!=(values<X>) const noexcept{ return false; }
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

	template<template<typename...> class T, typename ... Us>
	struct partial{
		using args = types<Us...>;

		template<typename ... Vs>
		using apply = T<Us..., Vs...>;
	};

	namespace detail{
		template<template<typename...> class T, typename Args>
		struct instantiate_helper;

		template<template<typename...> class T, typename ... Args>
		struct instantiate_helper<T, types<Args...>>{
			using type = T<Args...>;
		};
	}

	template<template<typename...> class T, typename Args>
	using instantiate = typename detail::instantiate_helper<T, Args>::type;

	namespace detail{
		template<typename Ts, typename Indices>
		struct make_numbered_helper;

		template<typename ... Ts, std::size_t ... Indices>
		struct make_numbered_helper<types<Ts...>, std::index_sequence<Indices...>>{
			using type = types<
				typename Ts::template apply<values<Indices>>...
			>;
		};

		template<typename ... Ts>
		using make_numbered = typename make_numbered_helper<types<Ts...>, std::make_index_sequence<sizeof...(Ts)>>::type;
	}

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

		template<typename T>
		struct template_args_helper;

		template<template<typename...> typename Tmpl, typename ... Ts>
		struct template_args_helper<Tmpl<Ts...>>{
			using type = types<Ts...>;
		};
	}

	template<typename L, typename T>
	using append = typename detail::append_helper<L, T>::type;

	template<typename ... Ls>
	using join = typename detail::join_helper<Ls...>::type;

	template<typename T>
	using template_args = typename detail::template_args_helper<T>::type;

	namespace detail{
		template<typename Ts>
		struct for_all_helper;

		template<typename ... Ts>
		struct for_all_helper<std::tuple<Ts...>>{
			public:
				template<typename Tup, typename Fn>
				void invoke(Tup &&tup, Fn &&f){
					invoke_impl(std::forward<Tup>(tup), std::forward<Fn>(f), indices{});
				}

			private:
				template<typename Tup, typename Fn, std::size_t ... Is>
				void invoke_impl(Tup &&tup, Fn &&f, std::index_sequence<Is...>){
					(std::forward<Fn>(f)(std::get<Is>(std::forward<Tup>(tup))), ...);
				}

				using indices = std::make_index_sequence<sizeof...(Ts)>;
		};
	}

	template<typename Tup, typename Fn>
	void for_all(Tup &&tup, Fn &&f){
		using tuple_type = std::decay_t<Tup>;
		detail::for_all_helper<tuple_type>::invoke(std::forward<Tup>(tup), std::forward<Fn>(f));
	}

	/**
	 * @brief Iterate all types in a type list.
	 * @param f Template function to apply to each type.
	 */
	template<typename ... Ts, typename Fn>
	void for_all(types<Ts...>, Fn &&f){
		if constexpr((std::is_invocable_v<std::decay_t<Fn>, type<Ts>> && ...)){
			(std::forward<Fn>(f)(type<Ts>{}), ...);
		}
		else{
			(std::forward<Fn>(f).template operator()<Ts>(), ...);
		}
	}

	/**
	 * @brief Iterate all values in a value list.
	 * @param f Template function to apply to each value.
	 */
	template<auto ... Xs, typename Fn>
	void for_all(values<Xs...>, Fn &&f){
		if constexpr((std::is_invocable_v<std::decay_t<Fn>, value<Xs>> && ...)){
			(std::forward<Fn>(f)(value<Xs>{}), ...);
		}
		else{
			(std::forward<Fn>(f).template operator()<Xs>(), ...);
		}
	}

	template<typename Ts, typename Fn>
	void for_all(Fn &&f){
		for_all(Ts{}, std::forward<Fn>(f));
	}

	namespace detail{
		template<typename Ts>
		struct for_all_i_helper;

		template<typename ... Ts>
		struct for_all_i_helper<types<Ts...>>{
			public:
				template<typename Fn>
				static constexpr void invoke(Fn &&f){
					invoke_impl(std::forward<Fn>(f), indices{});
				}

			private:
				template<typename Fn, std::size_t ... Is>
				static constexpr void invoke_impl(Fn &&f, std::index_sequence<Is...>){
					if constexpr((std::is_invocable_v<std::decay_t<Fn>, type<Ts>, values<Is>> && ...)){
						(std::forward<Fn>(f)(type<Ts>{}, values<Is>{}), ...);
					}
					else if constexpr((std::is_invocable_v<std::decay_t<Fn>, type<Ts>, std::size_t> && ...)){
						(std::forward<Fn>(f)(type<Ts>{}, Is), ...);
					}
					else{
						(std::forward<Fn>(f).template operator()<Ts, Is>(), ...);
					}
				}

				using indices = std::make_index_sequence<sizeof...(Ts)>;
		};

		template<typename ... Ts>
		struct for_all_i_helper<std::tuple<Ts...>>{
			public:
				template<typename Tup, typename Fn>
				static constexpr void invoke(Tup &&tup, Fn &&f){
					invoke_impl(std::forward<Tup>(tup), std::forward<Fn>(f), indices{});
				}

			private:
				template<typename Tup, typename Fn, std::size_t ... Is>
				static constexpr void invoke_impl(Tup &&tup, Fn &&f, std::index_sequence<Is...>){
					if constexpr((std::is_invocable_v<std::decay_t<Fn>, std::tuple_element<Is, std::decay_t<Tup>>, values<Is>> && ...)){
						(std::forward<Fn>(f)(std::get<Is>(std::forward<Tup>(tup)), values<Is>{}), ...);
					}
					else if constexpr((std::is_invocable_v<std::decay_t<Fn>, std::tuple_element<Is, std::decay_t<Tup>>, std::size_t> && ...)){
						(std::forward<Fn>(f)(std::get<Is>(std::forward<Tup>(tup)), Is), ...);
					}
					else{
						(std::forward<Fn>(f).template operator()<Is>(std::get<Is>(std::forward<Tup>(tup))), ...);
					}

					(std::forward<Fn>(f).template operator()<Is>(std::get<Is>(std::forward<Tup>(tup))), ...);
				}

				using indices = std::make_index_sequence<sizeof...(Ts)>;
		};
	}

	template<typename Tup, typename Fn>
	void for_all_i(Tup &&tup, Fn &&f){
		detail::for_all_i_helper<std::decay_t<Tup>>::invoke(std::forward<Tup>(tup), std::forward<Fn>(f));
	}

	template<typename Ts, typename Fn>
	void for_all_i(Fn &&f){
		detail::for_all_i_helper<Ts>::invoke(std::forward<Fn>(f));
	}

	namespace detail{
		template<typename Ts, typename = void>
		struct map_all_helper;

		template <typename AlwaysVoid, typename... Ts>
		struct has_common_type_impl : std::false_type {};

		template <typename... Ts>
		struct has_common_type_impl<std::void_t<std::common_type_t<Ts...>>, Ts...> : std::true_type {};

		template <typename... Ts>
		inline constexpr bool has_common_type = has_common_type_impl<void, Ts...>::value;

		template<typename ... Ts>
		struct map_all_helper<types<Ts...>, std::void_t<std::common_type_t<Ts...>>>{
			public:
				template<typename Fn>
				static auto invoke(Fn &&f){
					using fn_type = std::decay_t<Fn>;

					if constexpr(has_common_type<Ts...>){
						using common_type = std::common_type_t<Ts...>;
						using result_type = std::array<common_type, sizeof...(Ts)>;

						if constexpr((std::is_invocable_v<fn_type, metapp::type<Ts>> && ...)){
							return result_type(std::forward<Fn>(f)(metapp::type<Ts>{})...);
						}
						else{
							return result_type(std::forward<Fn>(f).template operator()<Ts>()...);
						}
					}
					else{
						if constexpr((std::is_invocable_v<fn_type, metapp::type<Ts>> && ...)){
							return std::make_tuple(std::forward<Fn>(f)(metapp::type<Ts>{})...);
						}
						else{
							return std::make_tuple(std::forward<Fn>(f).template operator()<Ts>()...);
						}
					}
				}
		};
	}

	template<typename ... Ts, typename Fn>
	auto map_all(types<Ts...>, Fn &&f){
		return detail::map_all_helper<types<Ts...>>::invoke(std::forward<Fn>(f));
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

			constexpr bool empty() const noexcept{
				return size == 0;
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

		template<typename Ent, std::size_t AttribIdx, std::size_t Idx>
		struct attrib_arg_info_data;

		template<auto Fn>
		struct function_info_data;

		template<typename Ent, std::size_t Idx>
		struct param_info_data;

		template<typename Class, std::size_t Idx>
		struct class_base_info_data;

		template<typename Class, std::size_t Idx>
		struct class_ctor_info_data;

		template<typename Class>
		struct class_dtor_info_data;

		template<typename Class, std::size_t MethodIdx, std::size_t Idx>
		struct class_method_param_info_data;

		template<typename Class, std::size_t Idx>
		struct class_method_info_data;

		template<typename Class, std::size_t Idx>
		struct class_member_info_data;

		template<typename Class>
		struct class_info_data;

		template<typename Enum, std::size_t Idx>
		struct enum_value_info_data;

		template<typename Enum>
		struct enum_info_data;
	}

	enum class access_kind{
		public_, protected_, private_
	};

	template<typename Ent, typename AttribIdx, typename Idx>
	struct attrib_arg_info{
		static constexpr std::string_view value = detail::attrib_arg_info_data<Ent, get_v<AttribIdx>, get_v<Idx>>::value;
	};

	template<typename Ent, typename Idx>
	struct attrib_info{
		using args = typename detail::attrib_info_data<Ent, get_v<Idx>>::args;

		static constexpr std::string_view scope = detail::attrib_info_data<Ent, get_v<Idx>>::scope;
		static constexpr std::string_view name = detail::attrib_info_data<Ent, get_v<Idx>>::name;
	};

	template<typename Ent, typename Idx>
	struct param_info{
		using type = typename detail::param_info_data<Ent, get_v<Idx>>::type;

		static constexpr std::string_view name = detail::param_info_data<Ent, get_v<Idx>>::name;
	};

	template<auto Fn>
	struct function_info{
		using type = typename detail::function_info_data<Fn>::type;
		using result = typename detail::function_info_data<Fn>::result;
		using params = typename detail::function_info_data<Fn>::params;

		static constexpr std::string_view name = detail::function_info_data<Fn>::name;
	};

	template<typename Class, typename Idx>
	struct class_base_info{
		using type = typename detail::class_base_info_data<Class, get_v<Idx>>::type;

		static constexpr bool is_variadic = detail::class_base_info_data<Class, get_v<Idx>>::is_variadic;
		static constexpr access_kind access = detail::class_base_info_data<Class, get_v<Idx>>::access;
	};

	template<typename Class, typename Idx>
	struct class_ctor_info{
		using params = typename detail::class_ctor_info_data<Class, get_v<Idx>>::params;

		static constexpr bool is_move_ctor = detail::class_ctor_info_data<Class, get_v<Idx>>::is_move_ctor;
		static constexpr bool is_copy_ctor = detail::class_ctor_info_data<Class, get_v<Idx>>::is_copy_ctor;
		static constexpr bool is_default_ctor = detail::class_ctor_info_data<Class, get_v<Idx>>::is_default_ctor;

		static constexpr std::size_t num_params = params::size;

		//using param_types = typename detail::param_info_data<class_ctor_info<Class, get_v<Idx>>, Idx>::param_types;
	};

	template<typename Class>
	struct class_dtor_info{
		static constexpr bool is_virtual = std::has_virtual_destructor_v<Class>;
		//static constexpr bool is_defaulted = detail::class_dtor_info_data<Class>::is_defaulted;
	};

	template<typename Class, typename MethodIdx, typename Idx>
	struct class_method_param_info{
		static constexpr std::string_view name = detail::class_method_param_info_data<Class, get_v<MethodIdx>, get_v<Idx>>::name;
		using type = typename detail::class_method_param_info_data<Class, get_v<MethodIdx>, get_v<Idx>>::type;
	};

	template<typename Class, typename Idx>
	struct class_method_info{
		using ptr_type = typename detail::class_method_info_data<Class, get_v<Idx>>::ptr_type;
		using result = typename detail::class_method_info_data<Class, get_v<Idx>>::result;
		using param_types = typename detail::class_method_info_data<Class, get_v<Idx>>::param_types;
		using params = typename detail::class_method_info_data<Class, get_v<Idx>>::params;

		static constexpr std::string_view name = detail::class_method_info_data<Class, get_v<Idx>>::name;
		static constexpr bool is_virtual = detail::class_method_info_data<Class, get_v<Idx>>::is_virtual;
		static constexpr auto ptr = detail::class_method_info_data<Class, get_v<Idx>>::ptr;

		template<typename T, typename ... Args>
		static constexpr decltype(auto) call(T &&cls, Args &&... args){
			return (std::forward<T>(cls).*ptr)(std::forward<Args>(args)...);
		}
	};

	template<typename Class, typename Idx>
	struct class_member_info{
		using type = typename detail::class_member_info_data<Class, get_v<Idx>>::type;

		static constexpr std::string_view name = detail::class_member_info_data<Class, get_v<Idx>>::name;
		static constexpr auto ptr = detail::class_member_info_data<Class, get_v<Idx>>::ptr;

		template<typename T>
		static constexpr auto get(T &&cls){
			return std::forward<T>(cls).*ptr;
		}
	};

	struct ignore{
		template<typename T>
		constexpr bool operator==(T&&) const noexcept{ return true; }

		template<typename T>
		constexpr bool operator!=(T&&) const noexcept{ return false; }
	};

#if __cplusplus >= 202002L && !METACPP_TOOL_RUN
	namespace detail{
		template<fixed_str Scope, fixed_str Name, typename Attribs, typename ... Results>
		struct query_attribs_helper;

		template<
			fixed_str Scope, fixed_str Name,
			typename Attrib, typename ... Attribs,
			typename ... Results
		>
		struct query_attribs_helper<Scope, Name, types<Attrib, Attribs...>, Results...>
			: std::conditional_t<
				Attrib::name == Name,
				std::conditional_t<
					Scope.empty(),
					query_attribs_helper<Scope, Name, types<Attribs...>, Results..., Attrib>,
					std::conditional_t<
						Attrib::scope == Scope,
						query_attribs_helper<Scope, Name, types<Attribs...>, Results..., Attrib>,
						query_attribs_helper<Scope, Name, types<Attribs...>, Results...>
					>
				>,
				query_attribs_helper<Scope, Name, types<Attribs...>, Results...>
			>
		{};

		template<fixed_str Scope, fixed_str Name, typename ... Results>
		struct query_attribs_helper<Scope, Name, types<>, Results...>{
			using type = types<Results...>;
		};

		template<typename Signature, auto Name, typename Infos, typename ... Results>
		struct query_methods_helper;

		template<fixed_str Name, typename MethodInfo, typename ... MethodInfos, typename ... Results>
		struct query_methods_helper<ignore, Name, types<MethodInfo, MethodInfos...>, Results...>
			: std::conditional_t<
				MethodInfo::name == Name,
				query_methods_helper<ignore, Name, types<MethodInfos...>, Results..., MethodInfo>,
				query_methods_helper<ignore, Name, types<MethodInfos...>, Results...>
			>
		{};

		template<
			typename Ret, typename ... Params,
			fixed_str Name,
			typename MethodInfo, typename ... MethodInfos,
			typename ... Results
		>
		struct query_methods_helper<Ret(Params...), Name, types<MethodInfo, MethodInfos...>, Results...>{
			private:
				static constexpr auto dummy_query(){
					if constexpr(MethodInfo::name == Name){
						if constexpr(std::is_same_v<Ret, ignore> || std::is_same_v<Ret, typename MethodInfo::result>){
							if constexpr(types<Params...>{} == types<ignore>{} || types<Params...>{} == typename MethodInfo::param_types{}){
								return typename query_methods_helper<Ret(Params...), Name, types<MethodInfos...>, Results..., MethodInfo>::type{};
							}
							else{
								return typename query_methods_helper<Ret(Params...), Name, types<MethodInfos...>, Results...>::type{};
							}
						}
						else{
							return typename query_methods_helper<Ret(Params...), Name, types<MethodInfos...>, Results...>::type{};
						}
					}
					else{
						return typename query_methods_helper<Ret(Params...), Name, types<MethodInfos...>, Results...>::type{};
					}
				}

			public:
				using type = decltype(dummy_query());
		};

		template<typename Signature, fixed_str Name, typename ... Results>
		struct query_methods_helper<Signature, Name, types<>, Results...>{
			using type = types<Results...>;
		};
	}
#endif

	template<typename Class>
	struct class_info{
		using attributes = typename detail::class_info_data<Class>::attributes;
		using bases = typename detail::class_info_data<Class>::bases;
		using ctors = typename detail::class_info_data<Class>::ctors;
		using methods = typename detail::class_info_data<Class>::methods;
		using members = typename detail::class_info_data<Class>::members;		

		static constexpr std::string_view name = detail::class_info_data<Class>::name;

		static constexpr bool is_abstract = std::is_abstract_v<Class>;

#if __cplusplus >= 202002L && !METACPP_TOOL_RUN
		template<fixed_str Scope, fixed_str Name>
		using query_attributes = typename detail::query_attribs_helper<Scope, Name, attributes>::type;

		template<fixed_str Name, typename Signature = ignore>
		using query_methods = typename detail::query_methods_helper<Signature, Name, methods>::type;
#endif
	};

#if __cplusplus >= 202002L && !METACPP_TOOL_RUN
	template<typename Class, fixed_str Name, typename Signature = ignore>
	using query_methods = typename detail::query_methods_helper<Signature, Name, typename class_info<Class>::methods>::type;
#endif

	template<typename Class, std::size_t Idx>
	using class_ctor = get_t<typename class_info<Class>::ctors, Idx>;

	template<typename Class>
	using class_dtor = typename class_info<Class>::dtor;

	template<typename Class, std::size_t Idx>
	using class_method = get_t<typename class_info<Class>::methods, Idx>;

	namespace detail{
		template<typename Params>
		struct param_types_helper;

		template<typename ... ParamInfos>
		struct param_types_helper<types<ParamInfos...>>{
			using type = types<typename ParamInfos::type...>;
		};
	}

	template<typename Ent>
	using param_types = typename detail::param_types_helper<typename Ent::params>::types;

	template<typename Class>
	using bases = typename class_info<Class>::bases;

	template<typename Class>
	using ctors = typename class_info<Class>::ctors;

	template<typename Class>
	using methods = typename class_info<Class>::methods;

	template<typename Class>
	using members = typename class_info<Class>::members;

	template<typename Enum, typename Idx>
	struct enum_value_info{
		static constexpr std::string_view name = detail::enum_value_info_data<Enum, get_v<Idx>>::name;
		static constexpr std::uint64_t value = detail::enum_value_info_data<Enum, get_v<Idx>>::value;
	};

	template<typename Enum>
	struct enum_info{
		using values = typename detail::enum_info_data<Enum>::values;

		static constexpr std::string_view name = detail::enum_info_data<Enum>::name;
		static constexpr bool is_scoped = detail::enum_info_data<Enum>::is_scoped;
	};

	namespace detail{
		template<typename Enum, typename Values>
		struct get_value_helper;

		template<typename Enum>
		struct get_value_helper<Enum, types<>>{
			static constexpr Enum get(const std::string_view name){
				throw std::logic_error("enum does not contain value by that name");
			}
		};

		template<typename Enum, typename Value, typename ... Values>
		struct get_value_helper<Enum, types<Value, Values...>>{
			static constexpr Enum get(const std::string_view name){
				if(Value::name == name){
					return static_cast<Enum>(Value::value);
				}
				else{
					return get_value_helper<Enum, types<Values...>>::get(name);
				}
			}
		};
	}

	template<typename Enum>
	inline constexpr Enum get_value(const std::string_view name){
		using info = enum_info<Enum>;
		return detail::get_value_helper<Enum, typename info::values>::get(name);
	}

	template<typename Enum>
	inline constexpr std::size_t num_enum_values = enum_info<Enum>::num_values;

	template<typename Enum, std::size_t Idx>
	using enum_value = get_t<typename enum_info<Enum>::values, Idx>;

	namespace detail{
		template<typename Ent, typename = void>
		struct info_helper;

		template<typename Class>
		struct info_helper<Class, std::enable_if_t<std::is_class_v<Class>>>{
			using type = class_info<Class>;
		};
	}

	template<typename Ent>
	using info = typename detail::info_helper<Ent>::type;

	namespace detail{
		template<typename Ent, typename = void>
		struct has_info_helper{
			static constexpr bool value = false;
		};

		template<typename Class>
		struct has_info_helper<Class, std::enable_if_t<std::is_class_v<Class>>>{
			template<typename U>
			static decltype(detail::class_info_data<U>::name, std::true_type{}) test(std::remove_reference_t<U>*);

			template<typename U>
			static std::false_type test(...);

			using type = decltype(test<Class>(nullptr));

			static constexpr bool value = type::value;
		};
	}

	template<typename Ent>
	inline constexpr bool has_info = detail::has_info_helper<Ent>::value;

	template<typename Ent>
	using attributes = typename info<Ent>::attributes;

	template<typename Ent, std::size_t Idx>
	using attribute_args = typename get_t<typename info<Ent>::attributes, Idx>::args;

#if __cplusplus >= 202002L && !METACPP_TOOL_RUN
	template<typename Ent, fixed_str Scope, fixed_str Name>
	using query_attribs = typename detail::query_attribs_helper<Scope, Name, typename info<Ent>::attributes>::type;
#endif
}

namespace METACPP_META_NAMESPACE = metapp;

/**
 * @}
 */

#endif // !METACPP_META_HPP
