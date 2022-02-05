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

#include "meta.hpp"

/**
 * @defgroup Refl Run-time reflection utilities
 * @{
 */

namespace reflpp{
	namespace detail{
		struct type_info_helper;
		struct num_info_helper;
		struct int_info_helper;
		struct function_info_helper;
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
	using num_info = const detail::num_info_helper*;
	using int_info = const detail::int_info_helper*;
	using function_info = const detail::function_info_helper*;
	using class_info = const detail::class_info_helper*;
	using enum_info = const detail::enum_info_helper*;

	using class_method_info = const detail::class_method_helper*;
	using class_variable_info = const detail::class_variable_helper*;

	type_info reflect(std::string_view name);
	class_info reflect_class(std::string_view name);
	enum_info reflect_enum(std::string_view name);

	inline class_info class_(std::string_view name){ return reflect_class(name); }
	inline enum_info enum_(std::string_view name){ return reflect_enum(name); }

	template<typename Ret, typename ... Args>
	function_info reflect(Ret(*ptr)(Args...)){
		return nullptr;
	}

	template<typename T>
	auto reflect(){
		static auto ret = detail::reflect_helper<T>::reflect();
		return ret;
	}

	bool has_base(class_info type, class_info base) noexcept;

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
			virtual void destroy(void *p) const noexcept = 0;
		};

		template<typename T, typename Helper>
		struct info_helper_base: Helper{
			std::string_view name() const noexcept override{ return metapp::type_name<T>; }
			std::size_t size() const noexcept override{ return sizeof(T); }
			std::size_t alignment() const noexcept override{ return alignof(T); }
			void destroy(void *p) const noexcept override{ std::destroy_at(reinterpret_cast<T*>(p)); }
		};

		struct ref_info_helper: type_info_helper{
			virtual type_info refered() const noexcept = 0;
		};

		struct ptr_info_helper: type_info_helper{
			virtual type_info pointed() const noexcept = 0;
		};

		type_info void_info() noexcept;
		int_info int_info(std::size_t bits, bool is_signed) noexcept;
		num_info float_info(std::size_t bits) noexcept;

		struct num_info_helper: type_info_helper{
			virtual bool is_floating_point() const noexcept = 0;
			virtual bool is_integer() const noexcept = 0;
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

		struct class_member_helper{};

		struct class_method_helper{
			virtual type_info result_type() const noexcept = 0;
			virtual std::size_t num_params() const noexcept = 0;
			virtual std::string_view param_name(std::size_t idx) const noexcept = 0;
			virtual type_info param_type(std::size_t idx) const noexcept = 0;
		};

		template<typename Cls, std::size_t Idx>
		struct class_method_impl: class_method_helper{
			using method_info = metapp::class_method<Cls, Idx>;

			type_info result_type() const noexcept override{
				static const auto ret = reflect<typename method_info::result>();
				return ret;
			}

			std::size_t num_params() const noexcept override{ return method_info::params::size; }

			std::string_view param_name(std::size_t idx) const noexcept override{
				if(idx >= num_params()) return "";
				std::string_view ret;
				metapp::for_all_i<typename method_info::params>([idx, &ret](auto info_type, auto info_idx){
					if(info_idx != idx) return;
					using info = metapp::get_t<decltype(info_type)>;
					ret = info::name;
				});
				return ret;
			}

			type_info param_type(std::size_t idx) const noexcept override{
				if(idx >= num_params()) return nullptr;
				type_info ret = nullptr;
				metapp::for_all_i<typename method_info::params>([idx, &ret](auto info_type, auto info_idx){
					if(info_idx != idx) return;
					using info = metapp::get_t<decltype(info_type)>;
					static const auto reflected = reflect<typename info::type>();
					ret = reflected;
				});
				return ret;
			}
		};

		struct class_variable_helper{};

		struct class_info_helper: type_info_helper{
			virtual std::size_t num_methods() const noexcept = 0;
			virtual const class_method_helper *method(std::size_t idx) const noexcept = 0;

			virtual std::size_t num_bases() const noexcept = 0;
			virtual class_info base(std::size_t i) const noexcept = 0;

			virtual void *cast_to_base(void *self, std::size_t idx) const noexcept = 0;
		};

		template<typename T>
		struct class_info_impl: info_helper_base<T, class_info_helper>{
			using class_meta = metapp::class_info<T>;

			class_info_impl(){ register_type(this, true); }

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

			std::size_t num_bases() const noexcept override{ return class_meta::bases::size; }

			class_info base(std::size_t idx) const noexcept override{
				if(idx >= num_bases()) return nullptr;

				class_info ret = nullptr;

				metapp::for_all_i<metapp::bases<T>>([idx, &ret](auto base_info, auto base_idx){
					if(idx != base_idx) return;

					using info = metapp::get_t<decltype(base_info)>;

					ret = reflect<typename info::type>();
				});

				return ret;
			}

			void *cast_to_base(void *self_void, std::size_t idx) const noexcept override{
				if(idx >= num_bases()) return nullptr;

				void *ret = nullptr;

				auto self = reinterpret_cast<T*>(self_void);

				metapp::for_all_i<metapp::bases<T>>([self, idx, &ret](auto base_info, auto base_idx){
					if(idx != base_idx) return;

					using info = metapp::get_t<decltype(base_info)>;

					if constexpr(info::access == metapp::access_kind::public_){
						ret = static_cast<typename info::type*>(self);
					}
				});

				return ret;
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
		};

		bool register_type(type_info info, bool overwrite = false);

		template<typename T>
		struct reflect_simple{
			static auto reflect(){
				if constexpr(std::is_class_v<T>){
					struct class_info_impl: info_helper_base<T, class_info_helper>{
						class_info_impl(){ register_type(this); }

						std::size_t num_methods() const noexcept override{ return 0; }
						const class_method_helper *method(std::size_t idx) const noexcept override{ return nullptr; }

						std::size_t num_bases() const noexcept override{ return 0; }
						class_info base(std::size_t i) const noexcept override{ return nullptr; }

						void *cast_to_base(void *self, std::size_t idx) const noexcept override{ return nullptr; }
					} static ret;
					return &ret;
				}
				else{
					struct type_info_impl: info_helper_base<T, type_info_helper>{
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
				struct ref_info_impl: ref_info_helper{
					ref_info_impl(){ register_type(this); }
					std::string_view name() const noexcept override{ return metapp::type_name<T>; }
					std::size_t size() const noexcept override{ return sizeof(void*); }
					std::size_t alignment() const noexcept override{ return alignof(void*); }
					void destroy(void *p) const noexcept override{ }
					type_info refered() const noexcept override{ static auto ret = reflpp::reflect<std::remove_reference_t<T>>(); return ret; }
				} static ret;
				return &ret;
			}
		};

		template<typename T>
		struct reflect_helper<T, std::enable_if_t<std::is_pointer_v<T>>>{
			static type_info reflect(){
				struct ptr_info_impl: ptr_info_helper{
					ptr_info_impl(){ register_type(this); }
					std::string_view name() const noexcept override{ return metapp::type_name<T>; }
					std::size_t size() const noexcept override{ return sizeof(T); }
					std::size_t alignment() const noexcept override{ return alignof(T); }
					void destroy(void *p) const noexcept override{ }
					type_info pointed() const noexcept override{ static auto ret = reflpp::reflect<std::remove_pointer_t<T>>(); return ret; }
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
	}

	class alignas(16) value{
		public:
			value() = default;

			template<typename T, typename ... Args>
			value(metapp::type<T>, Args &&... args)
				: m_type(reflect<T>())
			{
				if constexpr(sizeof(T) <= 16 && alignof(T) <= 16){
					new(m_storage.bytes) T(std::forward<Args>(args)...);
				}
				else{
					m_storage.pointer = std::aligned_alloc(alignof(T), sizeof(T));
					new(m_storage.pointer) T(std::forward<Args>(args)...);
				}
			}

			value(const value&) = delete;

			value(value &&other) noexcept
				: m_type(std::exchange(other.m_type, nullptr))
			{
				if(m_type->size() <= 16 && m_type->alignment() <= 16){
					std::memcpy(m_storage.bytes, other.m_storage.bytes, m_type->size());
				}
				else{
					m_storage.pointer = other.m_storage.pointer;
					other.m_storage.pointer = nullptr;
				}
			}

			~value(){
				destroy();
			}

			value &operator=(const value&) = delete;

			value &operator=(value &&other) noexcept{
				if(this != &other){
					destroy();

					m_type = std::exchange(other.m_type, nullptr);

					if(m_type->size() <= 16 && m_type->alignment() <= 16){
						std::memcpy(m_storage.bytes, other.m_storage.bytes, m_type->size());
					}
					else{
						m_storage.pointer = other.m_storage.pointer;
						other.m_storage.pointer = nullptr;
					}
				}

				return *this;
			}

			bool is_valid() const noexcept{ return !!m_type; }

			template<typename T>
			T *as() noexcept{
				if(!is_valid() || m_type != reflect<T>()){
					return nullptr;
				}
				else{
					return reinterpret_cast<T*>(m_storage.pointer);
				}
			}

			template<typename T>
			const T *as() const noexcept{
				if(!is_valid() || m_type != reflect<T>()){
					return nullptr;
				}
				else{
					return reinterpret_cast<const T*>(m_storage.pointer);
				}
			}

		private:
			void destroy(){
				if(!m_type) return;

				if(m_type->size() <= 16 && m_type->alignment() <= 16){
					m_type->destroy(m_storage.bytes);
				}
				else{
					m_type->destroy(m_storage.pointer);
					std::free(m_storage.pointer);
				}

				m_type = nullptr;
			}

			type_info m_type = nullptr;

			union storage_t{
				void *pointer;
				unsigned char bytes alignas(16)[16];
			} m_storage;
	};
}

namespace METACPP_REFL_NAMESPACE = reflpp;

/**
 * @}
 */

#endif // !METACPP_REFL_HPP
