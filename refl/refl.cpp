/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <optional>

#include "fmt/format.h"

#include "metacpp/refl.hpp"

namespace {
	class type_loader{
		public:
			type_loader(){}

			~type_loader(){}

			refl::type_info load(std::string_view name){
				if(!m_types.empty()){
					auto registered_res = m_types.find(name);
					if(registered_res != m_types.end()){
						return registered_res->second;
					}
				}

				//fmt::print(stderr, "Failed to import reflected type '{}'\n", name);

				return nullptr;
			}

			bool register_type(refl::type_info info, bool overwrite){
				if(!m_types.empty() && !overwrite){
					auto res = m_types.find(info->name());
					if(res != m_types.end()){
						return false;
					}

					auto emplace_res = m_types.try_emplace(info->name(), info);
					if(!emplace_res.second){
						return false;
					}
				}
				else{
					m_types[info->name()] = info;
				}

				return true;
			}

		private:
			std::unordered_map<std::string_view, refl::type_info> m_types;
	};

	type_loader REFLCPP_EXPORT_SYMBOL loader;

	struct void_info_helper: refl::detail::type_info_helper{
		std::string_view name() const noexcept override{ return "void"; }
		std::size_t size() const noexcept override{ return 0; }
		std::size_t alignment() const noexcept override{ return 0; }
		void destroy(void*) const noexcept override{}
		void *construct(void *p, refl::args_pack_base *args) const override{ return nullptr; }
		std::type_index type_index() const noexcept override{ return typeid(void); }
	};

	static refl::detail::int_info_helper_impl<std::int8_t> int8_refl;
	static refl::detail::int_info_helper_impl<std::int16_t> int16_refl;
	static refl::detail::int_info_helper_impl<std::int32_t> int32_refl;
	static refl::detail::int_info_helper_impl<std::int64_t> int64_refl;

	static refl::detail::int_info_helper_impl<std::uint8_t> uint8_refl;
	static refl::detail::int_info_helper_impl<std::uint16_t> uint16_refl;
	static refl::detail::int_info_helper_impl<std::uint32_t> uint32_refl;
	static refl::detail::int_info_helper_impl<std::uint64_t> uint64_refl;

	static refl::detail::float_info_helper_impl<float> float_refl;
	static refl::detail::float_info_helper_impl<double> double_refl;
}

refl::type_info refl::detail::void_info() noexcept{
	static void_info_helper ret;
	return &ret;
}

refl::int_info refl::detail::int_info(std::size_t bits, bool is_signed) noexcept{
	if(is_signed){
		switch(bits){
			case 8: return &uint8_refl;
			case 16: return &uint16_refl;
			case 32: return &uint32_refl;
			case 64: return &uint64_refl;
			default: return nullptr;
		}
	}
	else{
		switch(bits){
			case 8: return &int8_refl;
			case 16: return &int16_refl;
			case 32: return &int32_refl;
			case 64: return &int64_refl;
			default: return nullptr;
		}
	}
}

refl::num_info refl::detail::float_info(std::size_t bits) noexcept{
	switch(bits){
		case 32: return &float_refl;
		case 64: return &double_refl;
		default: return nullptr;
	}
}

bool refl::detail::register_type(refl::type_info info, bool overwrite){
	return loader.register_type(info, overwrite);
}

refl::type_info refl::reflect(std::string_view name){
	return loader.load(name);
}

refl::class_info refl::reflect_class(std::string_view name){
	auto ret = reflect(name);
	return dynamic_cast<class_info>(ret);
}

refl::enum_info refl::reflect_enum(std::string_view name){
	auto ret = reflect(name);
	return dynamic_cast<enum_info>(ret);
}

std::vector<std::string_view> refl::attribute(refl::type_info t, std::string_view name, std::vector<std::string_view> placeholder_){
	const std::size_t num_attribs = t->num_attributes();

	std::string attrib_name;

	for(std::size_t attrib_i = 0; attrib_i < num_attribs; attrib_i++){
		const auto attrib = t->attribute(attrib_i);
		const auto scope = attrib->scope();

		if(scope.empty()){
			attrib_name = attrib->name();
		}
		else{
			attrib_name = fmt::format("{}::{}", scope, attrib->name());
		}

		if(attrib_name == name){
			const std::size_t num_args = attrib->num_args();

			std::vector<std::string_view> ret;
			ret.reserve(num_args);

			for(std::size_t arg_i = 0; arg_i < num_args; arg_i++){
				ret.emplace_back(attrib->arg(arg_i));
			}

			return ret;
		}
	}

	return placeholder_;
}

bool refl::has_base(refl::class_info type, refl::class_info base) noexcept{
	if(!type || !base) return false;

	for(std::size_t i = 0; i < type->num_bases(); i++){
		auto type_base = type->base(i);
		if(type_base == base || refl::has_base(type_base, base)){
			return true;
		}
	}

	return false;
}
