# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

SET(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
SET(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)

find_package(Clang CONFIG REQUIRED)

add_library(metacpp-ast STATIC clang.hpp compile_info.cpp parse.cpp)
add_library(metacpp::ast ALIAS metacpp-ast)
add_library(metapp::ast ALIAS metacpp-ast)

target_compile_features(metacpp-ast PUBLIC cxx_std_17)

target_link_libraries(
	metacpp-ast PUBLIC metacpp::headers fmt::fmt-header-only
)

target_link_libraries(
	metacpp-ast PRIVATE libclang
)

target_include_directories(
	metacpp-ast PRIVATE ${CLANG_INCLUDE_DIRS}
)

if(METACPP_IPO_SUPPORTED)
	set_target_properties(
		metacpp-ast PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON
	)
endif()
