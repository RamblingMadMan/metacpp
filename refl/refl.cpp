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
