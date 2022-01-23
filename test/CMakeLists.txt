# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

enable_testing()

add_executable(ast-test test.hpp test.cpp)

target_compile_features(ast-test PRIVATE cxx_std_20)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	message(WARNING "MSVC not fully supported")
else()
	target_compile_options(ast-test PUBLIC "-Wno-attributes")
endif()

target_link_libraries(ast-test PUBLIC metacpp::ast metacpp::refl)

target_reflect(ast-test)

add_test(
	NAME tool-test
	COMMAND
		reflpp -o ${CMAKE_BINARY_DIR}/test ${CMAKE_BINARY_DIR} test/test.hpp
)
