# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(metacpp VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES ${CMAKE_CXX_IMPLICIT_INCLUDE_DIRECTORIES})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

###################
# Project Options #
###################

option(METACPP_BUILD_TESTS "Whether to build test targets" ${PROJECT_IS_TOP_LEVEL})
option(METACPP_BUILD_DOCS "Whether to build source documentation" ${PROJECT_IS_TOP_LEVEL})
set(METACPP_SMALL_SIZE 16 CACHE STRING "Size considered 'small'")

#########################
# Project Configuration #
#########################

set(METACPP_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(METACPP_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(METACPP_VERSION_PATCH ${PROJECT_VERSION_PATCH})

execute_process(
	COMMAND git rev-parse --short HEAD
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	OUTPUT_VARIABLE METACPP_REPO_HASH
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
	COMMAND git rev-parse --abbrev-ref HEAD
	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
	OUTPUT_VARIABLE METACPP_REPO_BRANCH
	OUTPUT_STRIP_TRAILING_WHITESPACE
)

include(CheckIPOSupported)
check_ipo_supported(RESULT METACPP_IPO_SUPPORTED OUTPUT METACPP_IPO_ERROR)

set(
	METACPP_INCLUDE_DIR
	${CMAKE_CURRENT_LIST_DIR}/include
)

set(
	METACPP_RESOURCES
	README.md
	LICENSE
)

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

configure_file(include/metacpp/config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/include/metacpp/config.hpp)

################
# Dependencies #
################

include(FetchContent)

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

if(NOT TARGET fmt)
	find_package(fmt CONFIG)
endif()

if(NOT TARGET fmt AND NOT fmt_FOUND)
	FetchContent_Declare(
		fmtlib
		GIT_REPOSITORY https://github.com/fmtlib/fmt.git
		GIT_TAG        b6f4ceaed0a0a24ccf575fab6c56dd50ccf6f1a9 # 8.1.1
	)

	FetchContent_MakeAvailable(fmtlib)
endif()

###################
# Project Targets #
###################

add_library(metacpp-headers INTERFACE ${METACPP_RESOURCES} ${METACPP_INCLUDES})
target_compile_features(metacpp-headers INTERFACE cxx_std_17)
target_include_directories(metacpp-headers INTERFACE ${METACPP_INCLUDE_DIRS})

add_library(metacpp::headers ALIAS metacpp-headers)

add_library(metacpp-attribs INTERFACE)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	message(WARNING "MSVC not fully supported")
else()
	target_compile_options(metacpp-attribs INTERFACE "-Wno-attributes")
endif()

add_library(metacpp::attribs ALIAS metacpp-attribs)
add_library(metapp::attribs ALIAS metacpp-attribs)

add_subdirectory(ast)
add_subdirectory(plugin)
add_subdirectory(refl)

if(METACPP_BUILD_TESTS)
	add_subdirectory(test)
endif()

#########################
# Project Documentation #
#########################

if(METACPP_BUILD_DOCS)
	find_package(Doxygen)

	if(DOXYGEN_FOUND)

	endif()
endif()

########################
# Project Installation #
########################

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(METACPP_INCLUDE_INSTALL_DIR ${CMAKE_INSTALL_INCLUDEDIR} CACHE STRING "Where to install header files")
set(METACPP_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR} CACHE STRING "Where to install libraries")
set(METACPP_INSTALL_SYSCONFDIR ${CMAKE_INSTALL_SYSCONFDIR} CACHE STRING "Where to install configuration files")

configure_package_config_file(
	${CMAKE_CURRENT_LIST_DIR}/cmake/metacpp-config.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/metacpp-config.cmake
	INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/metacpp/cmake
	PATH_VARS METACPP_INCLUDE_INSTALL_DIR METACPP_INSTALL_SYSCONFDIR
)

write_basic_package_version_file(
	${CMAKE_CURRENT_BINARY_DIR}/metacpp-config-version.cmake
	VERSION "${metacpp_VERSION_MAJOR}.${metacpp_VERSION_MINOR}.${metacpp_VERSION_PATCH}"
	COMPATIBILITY SameMajorVersion
)

install(TARGETS reflpp metacpp-ast metacpp-refl metacpp-plugin)

install(
	DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/include/metacpp"
	DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
	FILES_MATCHING PATTERN "*.hpp"
)

install(
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/metacpp-config.cmake
		${CMAKE_CURRENT_BINARY_DIR}/metacpp-config-version.cmake
	DESTINATION ${CMAKE_INSTALL_LIBDIR}/metacpp/cmake
)
