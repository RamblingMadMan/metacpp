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
#include "metacpp/plugin.hpp"

namespace {
	class type_loader{
		public:
			type_loader()
				: program_symbols(plugin::self()->symbols())
			{}

			~type_loader(){}

			refl::type_info load(std::string_view name){
				auto program_lib = plugin::self();

				for(auto &&sym : program_symbols){
					auto readable = program_lib->demangle(sym);

					constexpr std::string_view export_fn_name = "reflpp::detail::type_export";

					auto exported_fn_res = readable.find(export_fn_name);
					if(exported_fn_res != std::string::npos){
						auto sym_ptr = program_lib->get_symbol(sym);
						if(!sym_ptr){
							return nullptr;
						}

						auto type_exporter = reinterpret_cast<reflpp::detail::type_export_fn>(sym_ptr);
						auto t = type_exporter();
						if(t->name() == name) return t;
						else continue;
					}
				}

				fmt::print(stderr, "Failed to import reflected type '{}'\n", name);

				return nullptr;
			}

		private:
			std::vector<std::string> program_symbols;
	};

	struct void_info_helper: refl::detail::type_info_helper{
		std::string_view name() const noexcept override{ return "void"; }
		std::size_t size() const noexcept override{ return 0; }
		std::size_t alignment() const noexcept override{ return 0; }
		void destroy(void*) const noexcept override{}
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

refl::type_info refl::reflect(std::string_view name){
	static type_loader loader;
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
