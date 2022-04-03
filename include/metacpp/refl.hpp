/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef METACPP_REFL_HPP
#define METACPP_REFL_HPP 1

#include <cstdlib>
#include <cstring>
#include <climits>
#include <functional>
#include <typeindex>

#include "meta.hpp"

/**
 * @defgroup Refl Run-time reflection utilities
 * @{
 */

namespace reflpp{
	struct args_pack_base;

	namespace detail{
		struct attribute_info_helper{
			virtual std::string_view scope() const noexcept = 0;
			virtual std::string_view name() const noexcept = 0;
			virtual std::size_t num_args() const noexcept = 0;
			virtual std::string_view arg(std::size_t idx) const noexcept = 0;
		};

		struct type_info_helper{
			virtual std::string_view name() const noexcept = 0;
			virtual std::size_t size() const noexcept = 0;
			virtual std::size_t alignment() const noexcept = 0;
			virtual void destroy(void *p) const noexcept = 0;
			virtual void *construct(void *p, args_pack_base *args) const = 0;
			virtual std::size_t num_attributes() const noexcept{ return 0; }
			virtual const attribute_info_helper *attribute(std::size_t idx) const noexcept{ return nullptr; }
			virtual std::type_index type_index() const noexcept = 0;
		};

		struct num_info_helper;
		struct int_info_helper;
		struct function_info_helper;
		struct fn_ptr_info_helper;
		struct class_info_helper;
		struct enum_info_helper;

		struct class_member_helper;
		struct class_method_helper;

		struct enum_value_helper;

		template<typename T, typename = void>
		struct reflect_helper;
	};

	/**
	 * @brief Handle to information about a type attribute.
	 */
	using attribute_info = const detail::attribute_info_helper*;

	/**
	 * @brief Handle to information about a type.
	 */
	using type_info = const detail::type_info_helper*;

	/**
	 * @brief Handle to information about an arithmetic type.
	 */
	using num_info = const detail::num_info_helper*;

	/**
	 * @brief Handle to information about an integer type.
	 */
	using int_info = const detail::int_info_helper*;

	/**
	 * @brief Handle to information about a function.
	 */
	using function_info = const detail::function_info_helper*;

	/**
	 * @brief Handle to information about a function pointer.
	 */
	using fn_ptr_info = const detail::fn_ptr_info_helper*;

	/**
	 * @brief Handle to information about a class/struct.
	 */
	using class_info = const detail::class_info_helper*;

	/**
	 * @brief Handle to information about an enum.
	 */
	using enum_info = const detail::enum_info_helper*;

	/**
	 * @brief Handle to information about a class method.
	 */
	using class_method_info = const detail::class_method_helper*;

	/**
	 * @brief Handle to information about a class member.
	 */
	using class_member_info = const detail::class_member_helper*;

	/**
	 * @brief Try to dynamically get information about a type by name.
	 * @param name type name to search for
	 * @returns `nullptr` on error, handle to found type on success
	 */
	type_info reflect(std::string_view name);

	/**
	 * @brief Get information about every reflected type in a program.
	 * @warning this can be an expensive operation
	 * @returns information about all reflected types
	 */
	std::vector<type_info> reflect_all();

	/**
	 * @brief Get information about every reflected class type in a program.
	 * @warning this can be an expensive operation
	 * @returns information about all reflected classes
	 */
	std::vector<class_info> reflect_all_classes();

	/**
	 * @brief Try to dynamically get information about a class type.
	 * @see reflect
	 */
	class_info reflect_class(std::string_view name);

	/**
	 * @brief Try to dynamically get information about an enum type.
	 * @see reflect
	 */
	enum_info reflect_enum(std::string_view name);

	/**
	 * @brief Helper function for reflecting class types.
	 * @see reflect_class
	 */
	inline class_info class_(std::string_view name){ return reflect_class(name); }

	/**
	 * @brief Helper function for reflecting enum types.
	 * @see reflect_enum
	 */
	inline enum_info enum_(std::string_view name){ return reflect_enum(name); }	

	/**
	 * @brief Get information about a function from it's pointer.
	 * @warning Currently unimplemented.
	 */
	template<typename Ret, typename ... Args>
	function_info reflect(Ret(*ptr)(Args...)){
		return nullptr;
	}

	/**
	 * @brief Get an attributes arguments from a type
	 * @param t type to query
	 * @param name name of the attribute to get
	 * @param placeholder_ values to return if the attribute doesn't exist
	 * @returns attribute arguments or placeholder values
	 */
	std::vector<std::string_view> attribute(type_info t, std::string_view name, std::vector<std::string_view> placeholder_ = {});

	/**
	 * @brief Get dynamic type information for a known type.
	 * @see reflect
	 */
	template<typename T>
	auto reflect(){
		static auto ret = detail::reflect_helper<T>::reflect();
		return ret;
	}

	/**
	 * @brief Check if a type has a specified base class.
	 * @param type type to check
	 * @param base base to check against
	 */
	bool has_base(class_info type, class_info base) noexcept;

	template<typename Base>
	struct derived_info;

	/**
	 * @brief Helper function to get information about all classes deriving from a common parent.
	 * @warning this can be an expensive operation
	 * @return type information for all reflected derived classes
	 */
	template<typename Base>
	std::vector<derived_info<Base>> reflect_all_derived();

	/**
	 * @brief Helper class for storing information about a specific heirarchy of types
	 * @tparam Base base class for stored class type
	 */
	template<typename Base>
	struct derived_info{
		public:
			template<typename Derived>
			derived_info(meta::type<Derived>){
				static_assert(std::is_base_of_v<Base, Derived>);
				m_info = reflect<Derived>();
			}

			template<typename Derived>
			derived_info(const derived_info<Derived> &other) noexcept{
				static_assert(std::is_base_of_v<Base, Derived>);
				m_info = other.m_info;
			}

			derived_info(class_info cls){
				if(!has_base(cls, base_info())){
					throw std::runtime_error("class type is not derived from base");
				}

				m_info = cls;
			}

			template<typename Derived>
			derived_info &operator=(const derived_info<Derived> &other) noexcept{
				static_assert(std::is_base_of_v<Base, Derived>);

				if(this != &other){
					m_info = other.m_info;
				}

				return *this;
			}

			operator class_info() const noexcept{ return m_info; }

			class_info operator->() const noexcept{ return m_info; }

			static class_info base_info(){
				static const class_info ret = reflect<Base>();
				return ret;
			}

		private:
			struct unsafe_tag{};

			derived_info(unsafe_tag, class_info cls) noexcept
				: m_info(cls){}

			class_info m_info;

			friend std::vector<derived_info<Base>> reflect_all_derived<Base>();
	};

	template<typename Base>
	std::vector<derived_info<Base>> reflect_all_derived(){
		static auto all_cls = reflect_all_classes();
		static auto base = reflect<Base>();

		std::vector<derived_info<Base>> ret;

		using info = derived_info<Base>;

		for(auto cls : all_cls){
			if(has_base(cls, base)){
				ret.emplace_back(info(typename info::unsafe_tag{}, cls));
			}
		}

		return ret;
	}

	namespace detail{
		template<typename T>
		struct arg_type_helper{ using type = T; };

		template<typename T>
		struct arg_type_helper<std::reference_wrapper<T>>{ using type = T; };
	}

	/**
	 * @brief Helper type for dynamically passing arguments.
	 */
	struct args_pack_base{
		virtual std::size_t size() const noexcept = 0;
		//virtual void *arg(std::size_t idx) const noexcept = 0;
		virtual type_info arg_type(std::size_t idx) const noexcept = 0;
		virtual class_info this_type() const noexcept = 0;
	};

	template<typename ... Args>
	struct args_pack: args_pack_base{
		public:
			using arg_types = metapp::types<Args...>;

			template<typename ... UArgs>
			explicit args_pack(UArgs &&... args)
				: args_pack(std::make_index_sequence<sizeof...(UArgs)>(), std::forward<UArgs>(args)...)
			{}

			std::size_t size() const noexcept override{ return sizeof...(Args); }
			//void *arg(std::size_t idx) const noexcept{ return idx >= size() ? nullptr : m_ptrs[idx]; }
			type_info arg_type(std::size_t idx) const noexcept{ return idx >= size() ? nullptr : m_types[idx]; }

			class_info this_type() const noexcept override{ return reflect<args_pack<Args...>>(); }

			template<typename Fn>
			decltype(auto) apply(Fn &&f){
				return apply_impl(std::forward<Fn>(f), std::make_index_sequence<sizeof...(Args)>());
			}

		private:
			template<std::size_t ... Is, typename ... UArgs>
			args_pack(std::index_sequence<Is...>, UArgs &&... args)
				: m_vals(std::forward_as_tuple(std::forward<UArgs>(args)...))
				, m_types{ reflect<meta::get_t<arg_types, Is>>()... }
			{}

			template<typename Fn, std::size_t ... Is>
			decltype(auto) apply_impl(Fn &&f, std::index_sequence<Is...>){
				return std::forward<Fn>(f)(static_cast<Args>(std::get<Is>(m_vals))...);
			}

			using value_types = metapp::types<std::decay_t<Args>...>;
			std::tuple<Args...> m_vals;
			type_info m_types[sizeof...(Args)];
	};

	/**
	 * @brief Pack a set of arguments for passing dynamically.
	 * @warning Arguments passed to this function must stay alive until the returned pack is destroyed.
	 * @param args arguments to pack
	 * @returns newly created argument pack
	 */
	template<typename ... Args>
	auto pack_args(Args &&... args){
		return args_pack<typename detail::arg_type_helper<Args>::type...>(std::forward<Args>(args)...);
	}

	namespace detail{
		template<typename T, typename Helper>
		struct info_helper_base: Helper{
			std::string_view name() const noexcept override{ return metapp::type_name<T>; }
			std::size_t size() const noexcept override{ return sizeof(T); }
			std::size_t alignment() const noexcept override{ return alignof(T); }
			void destroy(void *p) const noexcept override{ std::destroy_at(reinterpret_cast<T*>(p)); }
			std::type_index type_index() const noexcept override{ return typeid(T); }
		};

		struct ref_info_helper: type_info_helper{
			virtual type_info refered() const noexcept = 0;

			void *construct(void *p, args_pack_base *args) const override{
				if(args->size() != 0) return nullptr;
				return p;
			}
		};

		struct ptr_info_helper: type_info_helper{
			virtual type_info pointed() const noexcept = 0;

			void *construct(void *p, args_pack_base *args) const override{
				if(args->size() != 0) return nullptr;
				return p;
			}
		};

		struct fn_ptr_info_helper: type_info_helper{
			virtual type_info result() const noexcept = 0;
			virtual std::size_t num_parameters() const noexcept= 0;
			virtual type_info parameter(std::size_t idx) const noexcept = 0;
		};

		type_info void_info() noexcept;
		int_info int_info(std::size_t bits, bool is_signed) noexcept;
		num_info float_info(std::size_t bits) noexcept;

		struct num_info_helper: type_info_helper{
			virtual bool is_floating_point() const noexcept = 0;
			virtual bool is_integer() const noexcept = 0;
			void *construct(void *p, args_pack_base *args) const override{
				if(args->size() != 0) return nullptr;
				return p;
			}
		};

		struct int_info_helper: num_info_helper{
			bool is_floating_point() const noexcept override{ return false; }
			bool is_integer() const noexcept override{ return true; }

			virtual bool is_signed() const noexcept = 0;
		};

		template<typename T>
		struct float_info_helper_impl: info_helper_base<float_info_helper_impl<T>, num_info_helper>{
			bool is_floating_point() const noexcept override{ return true; }
			bool is_integer() const noexcept override{ return false; }
		};

		template<typename T>
		struct int_info_helper_impl: info_helper_base<int_info_helper_impl<T>, int_info_helper>{
			bool is_signed() const noexcept override{ return std::is_signed_v<T>; }
		};

		struct function_info_helper{
			virtual std::string_view name() const noexcept = 0;
			virtual type_info result_type() const noexcept = 0;
			virtual std::size_t num_params() const noexcept = 0;
			virtual std::string_view param_name(std::size_t idx) const noexcept = 0;
			virtual type_info param_type(std::size_t idx) const noexcept = 0;
		};

		struct class_member_helper{
			virtual std::string_view name() const noexcept = 0;
			virtual type_info type() const noexcept = 0;
			virtual std::size_t num_attributes() const noexcept = 0;
			virtual attribute_info attribute(std::size_t idx) const noexcept = 0;
			virtual void *get(void *self) const noexcept = 0;
		};

		struct class_method_helper{
			virtual std::string_view name() const noexcept = 0;
			virtual type_info result_type() const noexcept = 0;
			virtual std::size_t num_params() const noexcept = 0;
			virtual std::string_view param_name(std::size_t idx) const noexcept = 0;
			virtual type_info param_type(std::size_t idx) const noexcept = 0;
		};

		template<typename Cls, std::size_t Idx>
		struct class_member_impl final: class_member_helper{
			using member_info = metapp::class_member<Cls, Idx>;

			std::string_view name() const noexcept override{ return member_info::name; }

			type_info type() const noexcept override{
				static const auto ret = reflect<typename member_info::type>();
				return ret;
			}

			std::size_t num_attributes() const noexcept override{
				return member_info::attributes::size;
			}

			attribute_info attribute(std::size_t idx) const noexcept override{
				using attributes = typename member_info::attributes;

				if(idx >= num_attributes()) return nullptr;

				attribute_info ret = nullptr;

				metapp::for_all_i<attributes>([idx, &ret](auto info_type, auto info_idx){
					if(ret || idx != info_idx){
						return;
					}

					using info = metapp::get_t<decltype(info_type)>;

					struct attribute_info_impl: attribute_info_helper{
						std::string_view scope() const noexcept override{ return info::scope; }
						std::string_view name() const noexcept override{ return info::name; }
						std::size_t num_args() const noexcept override{ return info::args::size; }
						std::string_view arg(std::size_t idx0) const noexcept override{
							using arguments = typename info::args;

							if(idx0 >= num_args()) return "";

							std::string_view ret0;

							metapp::for_all_i<arguments>([idx0, &ret0](auto info_type, auto info_idx){
								if(!ret0.empty() || idx0 != info_idx){
									return;
								}

								using argument = metapp::get_t<decltype(info_type)>;

								ret0 = argument::value;
							});

							return ret0;
						}
					} static ret_val;

					ret = &ret_val;
				});

				return ret;
			}

			void *get(void *self) const noexcept override{
				auto p = reinterpret_cast<Cls*>(self);
				return &member_info::get(*p);
			}
		};

		template<typename Cls, std::size_t Idx>
		struct class_method_impl final: class_method_helper{
			using method_info = metapp::class_method<Cls, Idx>;

			std::string_view name() const noexcept override{
				return method_info::name;
			}

			type_info result_type() const noexcept override{
				static const auto ret = reflect<typename method_info::result>();
				return ret;
			}

			std::size_t num_params() const noexcept override{
				std::size_t ret = 0;
				metapp::for_all<typename method_info::params>([&](auto info_type){
					using info = metapp::get_t<decltype(info_type)>;
					if constexpr(info::is_variadic){
						ret += info::type::size;
					}
					else{
						++ret;
					}
				});

				return ret;
			}

			std::string_view param_name(std::size_t idx) const noexcept override{
				if(idx >= num_params()) return "";
				std::string_view ret;
				std::size_t offset = 0;
				metapp::for_all<typename method_info::params>([&](auto info_type){
					using info = metapp::get_t<decltype(info_type)>;
					if constexpr(info::is_variadic){
						metapp::for_all<typename info::type>([&](auto){
							if(offset++ != idx) return;
							ret = info::name;
						});
					}
					else{
						if(offset++ != idx) return;
						ret = info::name;
					}
				});
				return ret;
			}

			type_info param_type(std::size_t idx) const noexcept override{
				if(idx >= num_params()) return nullptr;
				type_info ret = nullptr;
				std::size_t offset = 0;
				metapp::for_all<typename method_info::params>([&](auto info_type){
					using info = metapp::get_t<decltype(info_type)>;

					if constexpr(info::is_variadic){
						metapp::for_all<typename info::type>([&](auto inner_type){
							using inner = metapp::get_t<decltype(inner_type)>;
							if(offset++ != idx) return;
							static const auto reflected = reflect<inner>();
							ret = reflected;
						});
					}
					else{
						if(offset != idx) return;
						static const auto reflected = reflect<typename info::type>();
						ret = reflected;
					}
				});
				return ret;
			}
		};

		struct class_info_helper: type_info_helper{
			virtual std::size_t num_methods() const noexcept = 0;
			virtual const class_method_helper *method(std::size_t idx) const noexcept = 0;

			virtual std::size_t num_members() const noexcept = 0;
			virtual const class_member_helper *member(std::size_t idx) const noexcept = 0;

			virtual std::size_t num_bases() const noexcept = 0;
			virtual class_info base(std::size_t i) const noexcept = 0;

			virtual void *cast_to_base(void *self, std::size_t idx) const noexcept = 0;

			template<typename T>
			T *cast_to(void *self_void, const class_info to = reflect<T>()) const noexcept{
				if(this == to){
					return reinterpret_cast<T*>(self_void);
				}

				const auto num_bases_ = num_bases();

				for(std::size_t i = 0; i < num_bases_; i++){
					auto base_ = base(i);
					auto as_base = cast_to_base(self_void, i);
					if(base_ == to){
						return reinterpret_cast<T*>(as_base);
					}
					else{
						auto as_inner_base = base_->cast_to<T>(as_base, to);
						if(as_inner_base) return as_inner_base;
					}
				}

				return nullptr;
			}
		};

		template<typename T>
		struct class_info_impl final: info_helper_base<T, class_info_helper>{
			using class_meta = metapp::class_info<T>;

			std::size_t m_num_bases = 0;

			class_info_impl(){
				metapp::for_all<metapp::bases<T>>([&](auto base_info){
					using info = metapp::get_t<decltype(base_info)>;
					if constexpr(info::is_variadic){
						m_num_bases += info::type::size;
					}
					else{
						++m_num_bases;
					}
				});

				register_type(this, true);
			}

			std::size_t num_attributes() const noexcept override{ return class_meta::attributes::size; }

			attribute_info attribute(std::size_t idx) const noexcept override{
				using attributes = typename class_meta::attributes;

				if(idx >= num_attributes()) return nullptr;

				attribute_info ret = nullptr;

				metapp::for_all_i<attributes>([idx, &ret](auto info_type, auto info_idx){
					if(ret || idx != info_idx){
						return;
					}

					using info = metapp::get_t<decltype(info_type)>;

					struct attribute_info_impl: attribute_info_helper{
						std::string_view scope() const noexcept override{ return info::scope; }
						std::string_view name() const noexcept override{ return info::name; }
						std::size_t num_args() const noexcept override{ return info::args::size; }
						std::string_view arg(std::size_t idx0) const noexcept override{
							using arguments = typename info::args;

							if(idx0 >= num_args()) return "";

							std::string_view ret0;

							metapp::for_all_i<arguments>([idx0, &ret0](auto info_type, auto info_idx){
								if(!ret0.empty() || idx0 != info_idx){
									return;
								}

								using argument = metapp::get_t<decltype(info_type)>;

								ret0 = argument::value;
							});

							return ret0;
						}
					} static ret_val;

					ret = &ret_val;
				});

				return ret;
			}

			std::size_t num_methods() const noexcept override{ return class_meta::methods::size; }

			const class_method_helper *method(std::size_t idx) const noexcept override{
				if(idx >= num_methods()) return nullptr;

				const class_method_helper *ret = nullptr;

				metapp::for_all_i<metapp::methods<T>>([idx, &ret](auto info_type, auto info_idx){
					if(idx != info_idx) return;

					static const class_method_impl<T, metapp::get_v<decltype(info_idx)>> method_impl;
					ret = &method_impl;
				});

				return ret;
			}

			std::size_t num_members() const noexcept override{ return class_meta::members::size; }

			const class_member_helper *member(std::size_t idx) const noexcept override{
				if(idx >= num_members()) return nullptr;

				const class_member_helper *ret = nullptr;

				metapp::for_all_i<metapp::members<T>>([idx, &ret](auto info_type, auto info_idx){
					if(idx != info_idx) return;

					static const class_member_impl<T, metapp::get_v<decltype(info_idx)>> member_impl;
					ret = &member_impl;
				});

				return ret;
			}

			std::size_t num_bases() const noexcept override{ return m_num_bases; }

			class_info base(std::size_t idx) const noexcept override{
				if(idx >= num_bases()) return nullptr;

				class_info ret = nullptr;

				std::size_t cur_idx = 0;

				metapp::for_all<metapp::bases<T>>([idx, &cur_idx, &ret](auto base_info){
					using info = metapp::get_t<decltype(base_info)>;

					if constexpr(info::is_variadic){
						metapp::for_all<typename info::type>([idx, &cur_idx, &ret](auto inner_info){
							using info_inner = metapp::get_t<decltype(inner_info)>;

							if(idx == cur_idx++){
								ret = reflect<info_inner>();
							}
						});
					}
					else{
						if(idx == cur_idx++){
							ret = reflect<typename info::type>();
						}
					}
				});

				return ret;
			}

			void *cast_to_base(void *self_void, std::size_t idx) const noexcept override{
				if(idx >= num_bases()) return nullptr;

				void *ret = nullptr;

				auto self = reinterpret_cast<T*>(self_void);

				std::size_t cur_idx = 0;

				metapp::for_all<metapp::bases<T>>([idx, self, &cur_idx, &ret](auto base_info){
					using info = metapp::get_t<decltype(base_info)>;

					if constexpr(info::access != metapp::access_kind::public_){
						return;
					}
					else{
						if(ret) return;
						if constexpr(info::is_variadic){
							metapp::for_all<typename info::type>([idx, self, &cur_idx, &ret](auto inner_info){
								using info_inner = metapp::get_t<decltype(inner_info)>;

								if(idx != cur_idx++){
									return;
								}

								ret = dynamic_cast<info_inner*>(self);
							});
						}
						else{
							if(idx != cur_idx++){
								return;
							}

							ret = dynamic_cast<typename info::type*>(self);
						}
					}
				});

				return ret;
			}

			void *construct(void *p, args_pack_base *args) const override{
				if constexpr(std::is_abstract_v<T>){
					return nullptr;
				}
				else{
					void *ret = nullptr;

					metapp::for_all<metapp::ctors<T>>([&](auto info_type){
						using ctor_info = metapp::get_t<decltype(info_type)>;
						if constexpr(ctor_info::is_accessable){
							if(ret) return;

							if constexpr(ctor_info::params::size == 0){
								if(args->size() != 0) return;
								else ret = new(p) T();
							}
							else{
								using param_types = metapp::param_types<ctor_info>;
								using args_derived = metapp::instantiate<args_pack, param_types>;

								auto args_ptr = dynamic_cast<args_derived*>(args);
								if(!args_ptr){
									return;
								}

								args_ptr->apply([&](auto &&... args){
									ret = new(p) T(std::forward<decltype(args)>(args)...);
								});
							}
						}
					});

					return ret;
				}
			}
		};

		struct enum_value_helper{
			virtual std::string_view name() const noexcept = 0;
			virtual std::uint64_t value() const noexcept = 0;
		};

		template<typename Enum, std::size_t Idx>
		struct enum_value_impl: enum_value_helper{
			using info = metapp::enum_value<Enum, Idx>;
			std::string_view name() const noexcept override{ return info::name; }
			std::uint64_t value() const noexcept override{ return info::value; }
		};

		struct enum_info_helper: type_info_helper{
			virtual std::size_t num_values() const noexcept = 0;
			virtual const enum_value_helper *value(std::size_t idx) const noexcept = 0;
		};

		template<typename T>
		struct enum_info_impl: info_helper_base<T, enum_info_helper>{
			using enum_meta = metapp::enum_info<T>;

			enum_info_impl(){ register_type(this, true); }

			std::size_t num_values() const noexcept override{ return enum_meta::values::size; }

			const enum_value_helper *value(std::size_t idx) const noexcept override{
				if(idx >= num_values()) return nullptr;

				const enum_value_helper *ret = nullptr;

				metapp::for_all_i<typename enum_meta::values>([idx, &ret](auto info_type, auto info_idx){
					if(idx != info_idx) return;

					static const enum_value_impl<T, metapp::get_v<decltype(info_idx)>> reflected;
					ret = &reflected;
				});

				return ret;
			}

			void *construct(void *p, args_pack_base *args) const override{
				if(args->size() != 0) return nullptr;
				return p;
			}
		};

		bool register_type(type_info info, bool overwrite = false);

		template<typename T>
		struct reflect_simple{
			static auto reflect(){
				if constexpr(std::is_class_v<T>){
					struct class_info_impl final: info_helper_base<T, class_info_helper>{
						class_info_impl(){ register_type(this); }

						std::size_t num_methods() const noexcept override{ return 0; }
						const class_method_helper *method(std::size_t) const noexcept override{ return nullptr; }

						std::size_t num_members() const noexcept override{ return 0; }
						const class_member_helper *member(std::size_t) const noexcept override{ return nullptr; }

						std::size_t num_bases() const noexcept override{ return 0; }
						class_info base(std::size_t) const noexcept override{ return nullptr; }

						void *cast_to_base(void *self, std::size_t) const noexcept override{ return nullptr; }

						void *construct(void *p, args_pack_base *args) const override{
							return nullptr;
						}
					} static ret;
					return &ret;
				}
				else{
					struct type_info_impl final: info_helper_base<T, type_info_helper>{
						type_info_impl(){ register_type(this); }
					} static ret;
					return &ret;
				}
			}
		};

		template<typename T, typename = void>
		struct reflect_info;

		template<typename Enum>
		struct reflect_info<Enum, std::enable_if_t<std::is_enum_v<Enum>>>{
			static auto reflect(){
				static const enum_info_impl<Enum> ret;
				return &ret;
			}
		};

		template<typename Class>
		struct reflect_info<Class, std::enable_if_t<std::is_class_v<Class>>>{
			static auto reflect(){
				static const class_info_impl<Class> ret;
				return &ret;
			}
		};

		template<typename T>
		struct reflect_helper<T, std::enable_if_t<std::is_reference_v<T>>>{
			static type_info reflect(){
				struct ref_info_impl final: ref_info_helper{
					ref_info_impl(){ register_type(this); }
					std::string_view name() const noexcept override{ return metapp::type_name<T>; }
					std::size_t size() const noexcept override{ return sizeof(void*); }
					std::size_t alignment() const noexcept override{ return alignof(void*); }
					void destroy(void *p) const noexcept override{ }
					std::type_index type_index() const noexcept override{ return typeid(T); }
					type_info refered() const noexcept override{ static auto ret = reflpp::reflect<std::remove_reference_t<T>>(); return ret; }
				} static ret;
				return &ret;
			}
		};

		template<typename T>
		struct reflect_helper<T, std::enable_if_t<std::is_pointer_v<T> && !std::is_function_v<std::remove_pointer_t<T>>>>{
			static type_info reflect(){
				struct ptr_info_impl final: ptr_info_helper{
					ptr_info_impl(){ register_type(this); }
					std::string_view name() const noexcept override{ return metapp::type_name<T>; }
					std::size_t size() const noexcept override{ return sizeof(T); }
					std::size_t alignment() const noexcept override{ return alignof(T); }
					void destroy(void *p) const noexcept override{ }
					std::type_index type_index() const noexcept override{ return typeid(T); }
					type_info pointed() const noexcept override{ static auto ret = reflpp::reflect<std::remove_pointer_t<T>>(); return ret; }
				} static ret;
				return &ret;
			}
		};

		template<typename Ret, typename ... Args>
		struct reflect_helper<Ret(*)(Args...), void>{
			static fn_ptr_info reflect(){
				struct fn_ptr_info_impl final: fn_ptr_info_helper{
					fn_ptr_info_impl(){ register_type(this); }
					std::string_view name() const noexcept override{ return metapp::type_name<Ret(*)(Args...)>; }
					std::size_t size() const noexcept override{ return sizeof(Ret(*)(Args...)); }
					std::size_t alignment() const noexcept override{ return alignof(Ret(*)(Args...)); }
					void destroy(void *p) const noexcept override{}
					std::type_index type_index() const noexcept override{ return typeid(Ret(*)(Args...)); }
					type_info result() const noexcept override{ return reflpp::reflect<Ret>(); }
					std::size_t num_parameters() const noexcept override{ return sizeof...(Args); }
					type_info parameter(std::size_t idx) const noexcept override{
						if(idx >= sizeof...(Args)){
							return nullptr;
						}

						type_info ret = nullptr;

						metapp::for_all_i<meta::types<Args...>>([idx, &ret](auto info_type, auto index_value){
							using type = metapp::get_t<decltype(info_type)>;
							constexpr auto index = metapp::get_v<decltype(index_value)>;

							if(ret) return;
							else if(index == idx){
								ret = reflpp::reflect<type>();
							}
						});

						return ret;
					}

					void *construct(void *p, args_pack_base *args) const override{
						if(args->size() != 0) return nullptr;
						return new(p) (Ret(*)(Args...));
					}
				} static ret;
				return &ret;
			}
		};

		template<typename T>
		struct reflect_helper<T, std::enable_if_t<std::is_void_v<T>>>{
			static type_info reflect(){ return void_info(); }
		};

		template<typename Int>
		struct reflect_helper<Int, std::enable_if_t<std::is_integral_v<Int>>>{
			static reflpp::int_info reflect(){ return detail::int_info(sizeof(Int) * CHAR_BIT, std::is_signed_v<Int>); }
		};

		template<typename Float>
		struct reflect_helper<Float, std::enable_if_t<std::is_floating_point_v<Float>>>{
			static num_info reflect(){ return detail::float_info(sizeof(Float) * CHAR_BIT); }
		};

		template<typename T>
		struct reflect_helper<T, std::enable_if_t<std::is_class_v<T>>>{
			static class_info reflect(){
				if(auto ret = reflect_class(metapp::type_name<T>); ret){
					if constexpr(metapp::has_info<T>){
						auto elaborated = dynamic_cast<const class_info_impl<T>*>(ret);
						if(!elaborated){
							return reflect_info<T>::reflect();
						}
					}

					return ret;
				}
				else{
					if constexpr(metapp::has_info<T>){
						return reflect_info<T>::reflect();
					}
					else{
						return reflect_simple<T>::reflect();
					}
				}
			}
		};

		template<typename T>
		struct reflect_helper<T, std::enable_if_t<std::is_enum_v<T>>>{
			static enum_info reflect(){ return reflect_enum(metapp::type_name<T>); }
		};

		template<typename T>
		REFLCPP_API extern type_info type_export();

		template<auto Ptr>
		REFLCPP_API extern function_info function_export();

		using type_export_fn = type_info(*)();
		using function_export_fn = function_info(*)();

		static void *aligned_alloc(std::size_t align, std::size_t len){
#ifdef __linux__
			return ::aligned_alloc(align, len);
#elif defined(_WIN32)
			return _aligned_malloc(len, align);
#else
			return std::aligned_alloc(align, len);
#endif
		}

		static void aligned_free(void *p){
#ifdef __linux__
			::free(p);
#elif defined(_WIN32)
			_aligned_free(p);
#else
			std::free(p);
#endif
		}
	}

	/**
	 * @brief Class for dynamically creating values of statically-unknown types.
	 * @tparam Base base class of all created values or `void` for any value.
	 */
	template<typename Base = void, template<typename> class AllocT = std::allocator>
	class alignas(16) value{
		public:
			using info_type = std::conditional_t<std::is_same_v<Base, void>, type_info, class_info>;

			value() = default;

			template<typename T, typename ... Args>
			value(metapp::type<T>, Args &&... args)
				: m_type(reflect<T>())
			{
				if constexpr(sizeof(T) <= 16 && alignof(T) <= 16){
					new(m_storage.bytes) T(std::forward<Args>(args)...);
					m_destroy_fn = [](void *mem, info_type){
						auto ptr = reinterpret_cast<T*>(mem);
						std::destroy_at(ptr);
					};
				}
				else{
					using aligned_storage = std::aligned_storage_t<sizeof(T), alignof(T)>;
					AllocT<aligned_storage> alloc;

					m_storage.pointer = alloc.allocate(1);
					m_destroy_fn = [](void *mem, info_type){
						AllocT<aligned_storage> alloc;

						auto ptr = reinterpret_cast<T*>(mem);
						std::destroy_at(ptr);
						alloc.deallocate(reinterpret_cast<aligned_storage*>(mem), 1);
					};
				}
			}

			template<typename ... Args>
			explicit value(info_type type_, Args &&... args)
				: m_type(nullptr)
			{
				construct(type_, std::forward<Args>(args)...);
			}

			value(const value&) = delete;

			template<typename Derived, template<typename> class AllocU>
			value(value<Derived, AllocU> &&other) noexcept{
				if constexpr(std::is_class_v<Base>){
					static_assert(std::is_base_of_v<Base, Derived>);
				}
				else if constexpr(!std::is_same_v<Base, void>){
					static_assert(std::is_same_v<Base, Derived>);
				}

				m_type = std::exchange(other.m_type, nullptr);
				m_destroy_fn = std::exchange(other.m_destroy_fn, [](auto...){});

				if(!m_type){
					return;
				}

				if(m_type->size() <= 16 && m_type->alignment() <= 16){
					std::memcpy(m_storage.bytes, other.m_storage.bytes, m_type->size());
					//std::memset(other.m_storage.bytes, 0, m_type->size());
				}
				else{
					m_storage.pointer = std::exchange(other.m_storage.pointer, nullptr);
				}
			}

			~value(){
				destroy();
			}

			value &operator=(const value&) = delete;

			template<typename Derived, template<typename> class AllocU>
			value &operator=(value<Derived, AllocU> &&other) noexcept{
				if constexpr(std::is_same_v<Base, Derived>){
					if(this == &other) return *this;
				}
				else if constexpr(std::is_class_v<Base>){
					static_assert(std::is_base_of_v<Base, Derived>);
				}
				else if constexpr(!std::is_same_v<Base, void>){
					static_assert(std::is_same_v<Base, Derived>);
				}

				destroy();

				m_type = std::exchange(other.m_type, nullptr);
				m_destroy_fn = std::exchange(other.m_destroy_fn, [](auto...){});

				if(m_type){
					if(m_type->size() <= 16 && m_type->alignment() <= 16){
						std::memcpy(m_storage.bytes, other.m_storage.bytes, m_type->size());
						//std::memset(other.m_storage.bytes, 0, m_type->size());
					}
					else{
						m_storage.pointer = std::exchange(other.m_storage.pointer, nullptr);
					}
				}

				return *this;
			}

			Base *operator->() noexcept{ return as<Base>(); }
			const Base *operator->() const noexcept{ return as<Base>(); }

			/**
			 * @brief Check that a value is contained in the object.
			 */
			bool is_valid() const noexcept{ return !!m_type; }

			/**
			 * @brief Get the type of the contained value.
			 */
			info_type type() const noexcept{ return m_type; }

			/**
			 * @brief Try to get the value as a specified type.
			 */
			template<typename T>
			T *as() noexcept{
				if(!is_valid()){
					return nullptr;
				}

				if constexpr(std::is_class_v<Base>){
					return m_type->template cast_to<T>(ptr());
				}
				else if constexpr(std::is_class_v<T>){
					auto cls = dynamic_cast<class_info>(m_type);
					if(!cls) return nullptr;
					return cls->template cast_to<T>(ptr());
				}
				else{
					return m_type == reflect<T>() ? reinterpret_cast<T*>(ptr()) : nullptr;
				}
			}

			/**
			 * @brief Try to get the value as a specified type.
			 */
			template<typename T>
			const T *as() const noexcept{
				if(!is_valid()){
					return nullptr;
				}

				if constexpr(std::is_class_v<Base>){
					return m_type->template cast_to<T>(ptr());
				}
				else if constexpr(std::is_class_v<T>){
					auto cls = dynamic_cast<class_info>(m_type);
					if(!cls) return nullptr;
					return cls->template cast_to<T>(ptr());
				}
				else{
					return m_type == reflect<T>() ? reinterpret_cast<T*>(ptr()) : nullptr;
				}
			}

		private:
			void destroy(){
				if(!m_type) return;

				m_destroy_fn(ptr(), m_type);

				m_type = nullptr;
				m_destroy_fn = [](auto...){};
			}

			template<typename ... Args>
			void construct(info_type type_, Args &&... args){
				destroy();

				auto pack = pack_args(std::forward<Args>(args)...);

				if(type_->size() <= 16 && type_->alignment() <= 16){
					std::memset(m_storage.bytes, 0, type_->size());
					if(!type_->construct(m_storage.bytes, &pack)){
						throw std::runtime_error("Could not construct small value");
					}

					m_destroy_fn = [](void *mem, info_type info){
						info->destroy(mem);
					};
				}
				else{
					m_storage.pointer = detail::aligned_alloc(type_->alignment(), type_->size());
					if(!type_->construct(m_storage.pointer, &pack)){
						throw std::runtime_error("Could not construct value");
					}

					m_destroy_fn = [](void *mem, info_type info){
						info->destroy(mem);
						detail::aligned_free(mem);
					};
				}

				m_type = type_;
			}

			void *ptr() noexcept{
				if(!m_type) return nullptr;

				if(m_type->size() <= 16 && m_type->alignment() <= 16){
					return m_storage.bytes;
				}
				else{
					return m_storage.pointer;
				}
			}

			const void *ptr() const noexcept{
				if(!m_type) return nullptr;

				if(m_type->size() <= 16 && m_type->alignment() <= 16){
					return m_storage.bytes;
				}
				else{
					return m_storage.pointer;
				}
			}

			info_type m_type = nullptr;
			void(*m_destroy_fn)(void*, info_type);

			union storage_t{
				void *pointer;
				unsigned char bytes alignas(16)[16];
			} m_storage;

			template<typename UBase, template<typename> class AllocU>
			friend class value;
	};
}

#ifndef METACPP_NO_NAMESPACE_ALIAS
namespace METACPP_REFL_NAMESPACE = reflpp;
#endif

/**
 * @}
 */

#endif // !METACPP_REFL_HPP
