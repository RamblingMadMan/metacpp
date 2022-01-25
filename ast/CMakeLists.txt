# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

find_package(Clang REQUIRED)

add_library(metacpp-ast clang.hpp compile_info.cpp parse.cpp)

target_compile_features(metacpp-ast PUBLIC cxx_std_17)

target_link_libraries(
	metacpp-ast PRIVATE libclang
)

target_link_libraries(
	metacpp-ast PUBLIC metacpp::headers fmt::fmt-header-only
)

target_include_directories(
	metacpp-ast PRIVATE ${CLANG_INCLUDE_DIRS}
)

add_library(metacpp::ast ALIAS metacpp-ast)
