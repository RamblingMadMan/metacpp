# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(metacpp VERSION 0.0.1 LANGUAGES C CXX)

include(FetchContent)

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
	${METACPP_INCLUDE_DIR}/metacpp/serial.hpp
	${CMAKE_CURRENT_BINARY_DIR}/include/metacpp/config.hpp
)

add_library(metacpp-headers INTERFACE ${METACPP_RESOURCES} ${METACPP_INCLUDES})
target_compile_features(metacpp-headers INTERFACE cxx_std_17)
target_include_directories(metacpp-headers INTERFACE ${METACPP_INCLUDE_DIRS})

add_library(metacpp::headers ALIAS metacpp-headers)

find_package(Boost CONFIG COMPONENTS system dll)

if(NOT Boost_system_FOUND)
	FetchContent_Declare(
		boostlib
		GIT_REPOSITORY https://github.com/boostorg/boost.git
		GIT_TAG        a07c63c9e56f4d3b5bc904e25a7ad6f900728dbf # boost-1.78.0
	)

	FetchContent_MakeAvailable(boostlib)
elseif(NOT Boost_dll_FOUND)
	message(WARNING "Could not find Boost::dll, trying to continue anyway")
endif()

find_package(fmt CONFIG)

if(NOT fmt_FOUND)
	FetchContent_Declare(
		fmtlib
		GIT_REPOSITORY https://github.com/fmtlib/fmt.git
		GIT_TAG        b6f4ceaed0a0a24ccf575fab6c56dd50ccf6f1a9 # 8.1.1
	)

	FetchContent_MakeAvailable(fmtlib)
endif()

add_subdirectory(ast)
add_subdirectory(plugin)
add_subdirectory(refl)

if(METACPP_BUILD_TESTS)
	add_subdirectory(test)
endif()
