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

	for(auto &&path : plugs){
		fmt::print("Plugin '{}'\n", path.c_str());
		plugin::load(path);
	}

	refl::class_info helped_cls;
	assert(helped_cls = refl::reflect_class("example_helped_instance"));

	auto instance = refl::value<TestTemplateClass<int>>(meta::type<example_helped_instance>{});

	assert(instance.as<TestTemplateClass<int>>());

	refl::class_info derived_cls;
	assert(derived_cls = refl::reflect_class("TestDerived"));

	refl::class_info base_cls;
	assert(base_cls = refl::reflect_class("TestBase"));

	assert(refl::has_base(derived_cls, base_cls));

	return 0;
}
