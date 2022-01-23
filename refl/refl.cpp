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

#include <dlfcn.h>

#include <optional>

#define BOOST_DLL_USE_STD_FS
#include "boost/core/demangle.hpp"
#include "boost/dll.hpp"

#include "fmt/format.h"

#include "metacpp/refl.hpp"

namespace dll = boost::dll;

namespace {
	class type_loader{
		public:
			type_loader()
				: program_handle(dlopen(nullptr, RTLD_LAZY))
				, program_symbols(dll::library_info(dll::program_location()).symbols())
			{}

			~type_loader(){
				dlclose(program_handle);
			}

			refl::type_info load(std::string_view name){
				auto program_lib = dll::shared_library(dll::program_location());

				for(auto &&sym : program_symbols){
					auto readable = boost::core::demangle(sym.c_str());

					constexpr std::string_view export_fn_name = "reflpp::detail::type_export";

					auto exported_fn_res = readable.find(export_fn_name);
					if(exported_fn_res != std::string::npos){
						auto sym_ptr = dlsym(program_handle, sym.c_str());
						if(!sym_ptr){
							std::fprintf(stderr, "Error in dlsym(%s): %s\n", sym.c_str(), dlerror());
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
			void *program_handle;
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
