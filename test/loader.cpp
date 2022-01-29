/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <cassert>

#include "fmt/format.h"

#include "metacpp/plugin.hpp"

#include "test/example.meta.h"

int main(int argc, char *argv[]){
	auto plugs = plugin::nearby_plugins();

	std::unordered_map<std::string_view, refl::class_info> classes;

	for(auto &&path : plugs){
		fmt::print("Plugin '{}'\n", path.c_str());

		auto plug = plugin::load(path);

		auto &&types = plug->exported_types();
		for(auto type : types){
			fmt::print("  Exports type '{}'\n", type->name());

			auto cls = dynamic_cast<refl::class_info>(type);
			if(cls){
				classes[cls->name()] = cls;
			}
		}

		auto &&fns = plug->exported_functions();
		for(auto fn : fns){
			fmt::print("  Exports fn '{}'\n", fn->name());
		}
	}

	auto derived_res = classes.find("TestDerived");
	assert(derived_res != classes.end());

	auto base_res = classes.find("TestBase");
	assert(base_res != classes.end());

	auto derived = derived_res->second;
	auto base = base_res->second;

	assert(refl::has_base(derived, base));

	return 0;
}
