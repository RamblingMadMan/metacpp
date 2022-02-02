# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

enable_testing()

add_plugin(
	plugin-test

	INCLUDE_DIRS
	include

	HEADERS
	test.hpp
)

add_plugin(
	plugin-test-other

	INCLUDE_DIRS
	include

	HEADERS
	include/test/example.h

	LIBRARIES
	plugin-test
)

add_executable(loader-test include/test/example.h loader.cpp)

add_executable(ast-test test.hpp test.cpp)
add_executable(meta-example example.h example.cpp)

target_compile_features(ast-test PRIVATE cxx_std_20)
target_compile_features(meta-example PRIVATE cxx_std_20)
target_compile_features(loader-test PRIVATE cxx_std_20)

set_target_properties(
	ast-test meta-example loader-test PROPERTIES
	CXX_STANDARD 20
	CXX_STANDARD_REQUIRED ON
)

target_include_directories(loader-test PRIVATE include)

target_link_libraries(ast-test PUBLIC metacpp::ast metacpp::refl)
target_link_libraries(loader-test PRIVATE fmt::fmt-header-only)
target_link_plugins(loader-test plugin-test plugin-test-other)

if(METACPP_IPO_SUPPORTED)
	set_target_properties(
		ast-test meta-example loader-test PROPERTIES
		INTERPROCEDURAL_OPTIMIZATION ON
	)
endif()

target_reflect(plugin-test)
target_reflect(plugin-test-other)
target_reflect(loader-test)
target_reflect(ast-test)
target_reflect(meta-example)

add_test(
	NAME tool-test
	COMMAND
		reflpp -o ${CMAKE_BINARY_DIR}/test ${CMAKE_BINARY_DIR} test/test.hpp
)
