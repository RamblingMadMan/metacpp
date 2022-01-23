# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(metacpp VERSION 0.0.1 LANGUAGES C CXX)

set(
	METACPP_INCLUDE_DIR
	${CMAKE_CURRENT_LIST_DIR}/include
)

configure_file(${METACPP_INCLUDE_DIR}/metacpp/config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/metacpp/config.hpp)

set(
	METACPP_INCLUDE_DIRS
	${CMAKE_CURRENT_LIST_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
)

set(
	METACPP_INCLUDES
	${METACPP_INCLUDE_DIR}/metacpp/ast.hpp
	${METACPP_INCLUDE_DIR}/metacpp/refl.hpp
	${METACPP_INCLUDE_DIR}/metacpp/meta.hpp
	${CMAKE_CURRENT_BINARY_DIR}/include/metacpp/config.hpp
)

add_library(metacpp-headers INTERFACE ${METACPP_INCLUDES})
target_compile_features(metacpp-headers INTERFACE cxx_std_20)
target_include_directories(metacpp-headers INTERFACE ${METACPP_INCLUDE_DIRS})

add_library(metacpp::headers ALIAS metacpp-headers)

add_subdirectory(deps/fmt)

add_subdirectory(astlib)
add_subdirectory(refl)
add_subdirectory(test)
