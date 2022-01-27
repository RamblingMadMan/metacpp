# Meta C++

This project contains a library for C++ AST parsing, metaprogramming and reflection.

Also included is a tool for generating the necessary meta and reflection information for use by each of the respective libraries.

## Dependencies

System dependencies:
- [CMake](https://cmake.org/) 3.21+
- A C++17 compiler
- [Libclang](https://clang.llvm.org/)

External dependencies (will be found or downloaded as required):
- [Boost](https://www.boost.org/) system
- [{fmt}](https://fmt.dev/)

## Including in a project

Although standalone compilation is supported and is important for bug-testing and development, the entire project is intended to be included as a subproject.

### Fetching the library

#### Using `FetchContent` (recommended)

From within your projects main `CMakeLists.txt` add the following before any targets that depend on the library:

```cmake
FetchContent_Declare(
    metapp
    GIT_REPOSITORY https://github.com/RamblingMadMan/metacpp.git
)

FetchContent_MakeAvailable(metapp)
```

#### As a git submodule

From the source directory of a project initialized with git run the following command:

```bash
git submodule add --depth 1 https://github.com/RamblingMadMan/metacpp.git deps/metacpp
```

Replace `deps/metacpp` with your own location if required.

Then from within your projects main `CMakeLists.txt` add the following before any targets that depend on the library:

```cmake
add_subdirectory(deps/metacpp)
```

### Generating information

For any targets that will use the `meta` or `refl` libraries, add the following to the correct `CMakeLists.txt`:

```cmake
target_reflect(<target>)
```

## Usage

### `CMakeLists.txt`:

```cmake
add_executable(example example.h example.cpp)

target_reflect(example)
```

### `example.h`:

```c++
#pragma once

#include <string_view>
#include <tuple>

class [[my::attrib(1, "2", 3.0)]] example{
	public:
		void method1(std::string_view s) const noexcept;
		void method1(const std::string &str) noexcept;

		std::tuple<int, float, char> method2();
};

enum class example_enum{
	case_0 = 69,
	case_1 = 420,
	case_2 = 1337
};

```

### `example.cpp`:

```c++
#include <iostream>

#include "example.meta.h"

int main(int argc, char *argv[]){
	// --------------
	// C++17 Features
	// --------------

	// iterate over all public class methods
	meta::for_all<meta::methods<example>>([]<class Method>{

		// get method names
		std::cout << "Method " << Method::name << "\n";

		// as well as information about the method
		std::cout << "\t"  "pointer type: " << meta::type_name<typename Method::ptr_type> << "\n";

		// iterate over every parameter of a method
		meta::for_all_i<typename Method::params>([](auto param, std::size_t idx){

			// 'param' is a meta::type<T>
			using info = meta::get_t<decltype(param)>;

			// get parameter types
			using param_type = typename info::type;

			// and names
			std::cout << "\t"  "parameter '" << info::name << "': " << meta::type_name<param_type> << "\n";

		});

		// and don't forget the result type
		std::cout << "\t"  "result type: " << meta::type_name<typename Method::result> << "\n";
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
			for_all_i<typename info::args>([]<class Arg, std::size_t Idx>{
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

```

## Bugs

Still in early development phase, so lots of bugs to be found.

If you do find any bugs; please, create an issue in this repo so I can prompty get them fixed.
