# Meta C++

This project contains a library for C++ AST parsing, metaprogramming and reflection.

Also included is a tool for generating the necessary meta and reflection information for use by each of the respective libraries.

## Dependencies

- A C++17 compiler
- Libclang

## Including in a project

Although standalone compilation is supported and is important for bug-testing and development, the entire project is intended to be included as a subproject.

### Fetching the library

#### As a git submodule (recommended)

From the source directory of a project initialized with git run the following command:

```bash
git submodule add --depth 1 https://github.com/RamblingMadMan/metacpp.git deps/metacpp
```

Replace `deps/metacpp` with your own location if required.

Then from within your projects main `CMakeLists.txt` add the following before any targets that depend on the library:

```cmake
add_subdirectory(deps/metacpp)
```

#### Using `FetchContent`

From within your projects main `CMakeLists.txt` add the following before any targets that depend on the library:

```cmake
FetchContent_Declare(
    metapp
    GIT_REPOSITORY https://github.com/RamblingMadMan/metacpp.git
)

FetchContent_MakeAvailable(metapp)
```

### Generating information

For any targets that will use the `meta` or `refl` libraries, add the following to the correct `CMakeLists.txt`:

```cmake
target_reflect(<target>)
```

## Usage

### `example.h`:

```c++
#include <string_view>
#include <tuple>

class example{
    public:
        void method1(std::string_view s) const noexcept;
        void method1(const std::string &str) noexcept;

        std::tuple<int, float, char> method2();
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
        std::cout << "\t"  "pointer type: " << meta::type_name<Method::ptr_type> << "\n";

        // iterate over every parameter of a method
        meta::for_all<typename Method::params>([]<class Param>{

            // get parameter types
            using param_type = typename Param::type;

            // and names
            std::cout << "\t"  "parameter '" << Param::name << "': " << meta::type_name<param_type> << "\n";

        });

        // and don't forget the result type
        std::cout << "\t"  "result type: " << meta::type_name<typename Method::result> << "\n";
    });

    // alias the info for the example class
    using example_info = meta::class_info<example>;

    // --------------
    // C++20 Features
    // --------------

    // do compile-time queries on class info
    static_assert(example_info::query_method<"method1">::size == 2);

    // specify a signature
    static_assert(example_info::query_method<"method1", void(std::string_view)>::size == 1);

    // or just part of one
    static_assert(example_info::query_method<"method2", meta::ignore()>::size == 1);
}
```
