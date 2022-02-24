/*
 * Meta C++ Tool and Library
 * Copyright (C) 2022  Keith Hammond
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include <iostream>

#include "example.meta.h"

int main(int argc, char *argv[]){
	// --------------
	// C++17 Features
	// --------------

	// iterate over all public class methods
	meta::for_all<meta::methods<example>>([](auto info_type){

		// retrieve the method info (not needed with c++20)
		using method = meta::get_t<decltype(info_type)>;

		// get method names
		std::cout << "Method " << method::name << "\n";

		// as well as information about the method
		std::cout << "\t"  "pointer type: " << meta::type_name<typename method::ptr_type> << "\n";

		// iterate over every parameter of a method
		meta::for_all_i<typename method::params>([](auto param, std::size_t idx){

			// 'param' is a meta::type<T>
			using info = meta::get_t<decltype(param)>;

			// get parameter types
			using param_type = typename info::type;

			// and names
			std::cout << "\t"  "parameter '" << info::name << "': " << meta::type_name<param_type> << "\n";

		});

		// and don't forget the result type
		std::cout << "\t"  "result type: " << meta::type_name<typename method::result> << "\n";
	});

	// iterate over all public class members
	meta::for_all<meta::members<example>>([](auto info_type){
		using member = meta::get_t<decltype(info_type)>;

		std::cout << "Member " << member::name << "\n";

		std::cout << "\t"	"pointer type: " << meta::type_name<typename member::ptr_type> << "\n";

		meta::for_all<typename member::attributes>([](auto attrib_info_type){
			using attrib = meta::get_t<decltype(attrib_info_type)>;

			if constexpr(attrib::scope.empty()){
				std::cout << "\t"	"attribute name: " << attrib::name << "\n";
			}
			else{
				std::cout << "\t"	"attribute name: " << attrib::scope << "::" << attrib::name << "\n";
			}

			if constexpr(attrib::args::size != 0){
				meta::for_all<typename attrib::args>([](auto arg_info_type){
					using arg = meta::get_t<decltype(arg_info_type)>;
					std::cout << "\t" "attribute arg: " << arg::value << "\n";
				});
			}
		});

	});

	// alias the info for the example class
	using example_info = meta::class_info<example>;

	// do some checks on the attributes
	static_assert(example_info::attributes::size == 1);

	// handy shortcut for getting the attributes of a type
	using my_attribs = meta::attributes<example>;

	// get the only attribute; by default 0 is passed as the second argument to get_t
	using my_attrib = meta::get_t<my_attribs>;

	// this can make the signal-to-noise ratio better
	using meta::get_t;
	using meta::for_all;
	using meta::for_all_i;

	// we can even inspect the arguments!
	using my_attrib_args = typename my_attrib::args;
	static_assert(my_attrib_args::size == 3);

	// do some checking on the attribute arguments
	static_assert(get_t<my_attrib_args, 0>::value == R"(1)");
	static_assert(get_t<my_attrib_args, 1>::value == R"("2")");
	static_assert(get_t<my_attrib_args, 2>::value == R"(3.0)");

	// iterate over all class attributes
	for_all<meta::attributes<example>>([](auto attrib){
		using info = get_t<decltype(attrib)>;

		// supports [[scoped::attributes]]
		if constexpr(!info::scope.empty()){
			std::cout << info::scope << "::";
		}

		// get attribute names
		std::cout << info::name;

		// also supports [[attributes(with, "args")]]
		if constexpr(!info::args::empty){
			std::cout << "(";

			// iterate over all attribute arguments
			for_all_i<typename info::args>([]<class Arg, std::size_t Idx>{ // template lambdas are a c++20 feature
				if constexpr(Idx != 0){
					std::cout << ", ";
				}

				std::cout << Arg::value;
			});

			std::cout << ")";
		}
	});

	// we can also reflect enums
	using enum_info = meta::enum_info<example_enum>;

	// get the name of an enum and if it is scoped
	std::cout << (enum_info::is_scoped ? "Scoped " : "") << "Enum " << enum_info::name << "\n";

	// iterate over all enum values
	for_all<typename enum_info::values>([](auto value){
		using info = get_t<decltype(value)>;

		// get the value name and unsigned integer representation
		std::cout << "\t"	"Enum value '" << info::name << "' == " << info::value << "\n";

	});

	// Get enum values from strings at compile time
	static_assert(meta::get_value<example_enum>("case_0") == static_cast<example_enum>(69));
	static_assert(meta::get_value<example_enum>("case_1") == static_cast<example_enum>(420));
	static_assert(meta::get_value<example_enum>("case_2") == static_cast<example_enum>(1337));

	// --------------
	// C++20 Features
	// --------------

	// do compile-time queries on class info
	static_assert(example_info::query_methods<"method1">::size == 2);

	// specify a signature
	static_assert(example_info::query_methods<"method1", void(std::string_view)>::size == 1);

	// or just part of one
	static_assert(example_info::query_methods<"method2", meta::ignore()>::size == 1);

	// also supports attribute queries
	static_assert(example_info::query_attributes<"my", "attrib">::size == 1);
}
