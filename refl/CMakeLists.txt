# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

find_package(Boost REQUIRED COMPONENTS system)

add_library(metacpp-refl STATIC make_meta.hpp refl.cpp)

target_compile_features(metacpp-refl PUBLIC cxx_std_17)

target_link_libraries(metacpp-refl PUBLIC metacpp::headers fmt::fmt-header-only)
target_link_libraries(metacpp-refl PRIVATE Boost::system metacpp::plugin ${CMAKE_DL_LIBS})

add_library(metacpp::refl ALIAS metacpp-refl)

add_executable(reflpp tool.cpp)

target_compile_features(reflpp PRIVATE cxx_std_17)

target_link_libraries(reflpp Boost::system fmt::fmt-header-only metacpp::ast)

function(target_reflect tgt)
	message("-- Generating reflection information for ${tgt}")

	get_target_property(TGT_TYPE ${tgt} TYPE)

	if(${TGT_TYPE} STREQUAL "INTERFACE_LIBRARY")
		target_link_libraries(${tgt} INTERFACE metacpp::headers)
	else()
		target_link_libraries(${tgt} PRIVATE metacpp::headers)
	endif()

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		message(WARNING "MSVC not fully supported")
	else()
		if(${TGT_TYPE} STREQUAL "INTERFACE_LIBRARY")
			target_compile_options(${tgt} INTERFACE "-Wno-attributes")
		else()
			target_compile_options(${tgt} PUBLIC "-Wno-attributes")
		endif()
	endif()

	get_target_property(TGT_BINARY_DIR ${tgt} BINARY_DIR)
	get_target_property(TGT_INCLUDE_DIRS ${tgt} INCLUDE_DIRECTORIES)
	get_target_property(TGT_SOURCE_DIR ${tgt} SOURCE_DIR)

	if(${TGT_TYPE} STREQUAL "INTERFACE_LIBRARY")
		get_target_property(TGT_SOURCES ${tgt} INTERFACE_SOURCES)
	else()
		get_target_property(TGT_SOURCES ${tgt} SOURCES)
	endif()

	set_target_properties(
		${tgt} PROPERTIES
		EXPORT_COMPILE_COMMANDS ON
		ENABLE_EXPORTS ON
	)

	if(${TGT_TYPE} STREQUAL "INTERFACE_LIBRARY")
		target_compile_features(${tgt} INTERFACE cxx_std_17)
	else()
		target_compile_features(${tgt} PRIVATE cxx_std_17)
	endif()

	get_target_property(TGT_CXX_STANDARD ${tgt} CXX_STANDARD)

	if(TGT_CXX_STANDARD LESS 17)
		message("-- Changing CXX_STANDARD for ${tgt} from ${TGT_CXX_STANDARD} to 17")
		set_target_properties(
			${tgt} PROPERTIES
			CXX_STANDARD 17
			CXX_STANDARD_REQUIRED ON
		)
	endif()

	set(INPUT_HEADERS "")

	set(OUTPUT_DIR "${TGT_BINARY_DIR}/reflect")
	set(OUTPUT_HEADERS "")
	set(OUTPUT_SOURCES "")

	foreach(SRC IN LISTS TGT_SOURCES)
		cmake_path(GET SRC EXTENSION TGT_SRC_EXT)
		if(TGT_SRC_EXT MATCHES "(\\.hpp)|(\\.h)")
			list(APPEND INPUT_HEADERS "${SRC}")

			cmake_path(REPLACE_EXTENSION SRC ".meta.hpp" OUTPUT_VARIABLE TGT_HEADER_OUT)

			set(TGT_HEADER_OUTPUT "${OUTPUT_DIR}/${TGT_HEADER_OUT}")
			set(TGT_SOURCE_OUTPUT "${OUTPUT_DIR}/${SRC}.refl.cpp")

			cmake_path(ABSOLUTE_PATH TGT_SOURCE_OUTPUT OUTPUT_VARIABLE TGT_SOURCE_OUTPUT)

			list(APPEND OUTPUT_HEADERS "${OUTPUT_DIR}/${TGT_HEADER_OUT}")
			list(APPEND OUTPUT_SOURCES "${TGT_SOURCE_OUTPUT}")

			if(${TGT_TYPE} STREQUAL "INTERFACE_LIBRARY")
				target_sources(${tgt} INTERFACE "${OUTPUT_DIR}/${TGT_HEADER_OUT}")
				target_sources(${tgt} INTERFACE "${TGT_SOURCE_OUTPUT}")
			else()
				target_sources(${tgt} PUBLIC "${OUTPUT_DIR}/${TGT_HEADER_OUT}")
				target_sources(${tgt} PRIVATE "${TGT_SOURCE_OUTPUT}")
			endif()

			#set_source_files_properties("${TGT_SOURCE_OUTPUT}" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
			#set_source_files_properties("${OUTPUT_DIR}/${TGT_HEADER_OUT}" PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
		endif()
	endforeach()

	if(NOT OUTPUT_HEADERS STREQUAL "")
		add_custom_command(
			OUTPUT ${OUTPUT_SOURCES} ${OUTPUT_HEADERS}
			DEPENDS reflpp ${INPUT_HEADERS}
			COMMAND reflpp -o "${OUTPUT_DIR}" "${PROJECT_BINARY_DIR}" ${INPUT_HEADERS}
			WORKING_DIRECTORY ${TGT_SOURCE_DIR}
			VERBATIM
		)

		#target_sources(${tgt} PRIVATE ${OUTPUT_SOURCES})
	endif()

	if(${TGT_TYPE} STREQUAL "INTERFACE_LIBRARY")
		target_include_directories(${tgt} INTERFACE ${OUTPUT_DIR} ${OUTPUT_DIR}/include)
	else()
		target_include_directories(${tgt} PRIVATE ${OUTPUT_DIR} ${OUTPUT_DIR}/include)
	endif()
endfunction()
