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
		plugin::load(path);
	}

	refl::class_info helped_cls;
	assert(helped_cls = refl::reflect_class("example_helped_instance"));

	auto instance = refl::value<TestTemplateClass<int>>(meta::type<example_helped_instance>{});

	assert(instance.as<TestTemplateClass<int>>());

	refl::class_info derived_test_cls;
	assert(derived_test_cls = refl::reflect_class("example_test_derived"));
	assert(derived_test_cls->size() == sizeof(example_test_derived));
	assert(derived_test_cls->alignment() == alignof(example_test_derived));

	{
		auto derived_instance = refl::value<TestTemplateClass<std::string_view>>(derived_test_cls, (int)1);
		assert(derived_instance.as<TestTemplateClass<std::string_view>>());
		assert(derived_instance->value() == "1");
	}

	refl::class_info derived_cls;
	assert(derived_cls = refl::reflect_class("TestDerived"));

	refl::class_info base_cls;
	assert(base_cls = refl::reflect_class("TestBase"));

	assert(refl::has_base(derived_cls, base_cls));

	return 0;
}
