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

#include "meta.hpp"

/**
 * @defgroup Refl Run-time reflection utilities
 * @{
 */

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
			virtual void destroy(void *p) const noexcept = 0;
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
			void destroy(void *p) const noexcept override{ std::destroy_at(reinterpret_cast<T*>(p)); }
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
