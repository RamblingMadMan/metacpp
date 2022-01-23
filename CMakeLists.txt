# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(metacpp VERSION 0.0.1 LANGUAGES C CXX)

option(METACPP_BUILD_TESTS "Whether to build test targets" ${PROJECT_IS_TOP_LEVEL})

set(
	METACPP_INCLUDE_DIR
	${CMAKE_CURRENT_LIST_DIR}/include
)

set(
	METACPP_RESOURCES
	README.md
	LICENSE
)

configure_file(${METACPP_INCLUDE_DIR}/metacpp/config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/metacpp/config.hpp)

set(
	METACPP_INCLUDE_DIRS
	${CMAKE_CURRENT_LIST_DIR}/include
	${CMAKE_CURRENT_BINARY_DIR}/include
)

set(
	METACPP_INCLUDES
	${METACPP_INCLUDE_DIR}/metacpp/meta.hpp
	${METACPP_INCLUDE_DIR}/metacpp/refl.hpp
	${METACPP_INCLUDE_DIR}/metacpp/plugin.hpp
	${METACPP_INCLUDE_DIR}/metacpp/ast.hpp
	${CMAKE_CURRENT_BINARY_DIR}/include/metacpp/config.hpp
)

add_library(metacpp-headers INTERFACE ${METACPP_RESOURCES} ${METACPP_INCLUDES})
target_compile_features(metacpp-headers INTERFACE cxx_std_20)
target_include_directories(metacpp-headers INTERFACE ${METACPP_INCLUDE_DIRS})

add_library(metacpp::headers ALIAS metacpp-headers)

add_subdirectory(deps/fmt EXCLUDE_FROM_ALL)

add_subdirectory(ast)
add_subdirectory(plugin)
add_subdirectory(refl)

if(METACPP_BUILD_TESTS)
	add_subdirectory(test)
endif()
