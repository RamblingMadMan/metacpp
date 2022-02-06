# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

add_library(metacpp-refl SHARED make_meta.hpp refl.cpp)
add_library(metacpp::refl ALIAS metacpp-refl)
add_library(metapp::refl ALIAS metacpp-refl)

target_compile_features(metacpp-refl PUBLIC cxx_std_17)

target_link_libraries(metacpp-refl PUBLIC metacpp::headers fmt::fmt-header-only)
target_link_libraries(metacpp-refl PRIVATE Boost::system)

set_target_properties(
	metacpp-refl PROPERTIES POSITION_INDEPENDENT_CODE ON
)

add_executable(reflpp tool.cpp)
add_executable(metacpp::refl-tool ALIAS reflpp)
add_executable(metapp::refl-tool ALIAS reflpp)

target_compile_features(reflpp PRIVATE cxx_std_17)

target_link_libraries(reflpp Boost::system fmt::fmt-header-only metacpp::ast)

if(METACPP_IPO_SUPPORTED)
	set_target_properties(
		reflpp PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON
	)
endif()

function(target_reflect tgt)
	message(STATUS "Generating reflection information for ${tgt}")

	get_target_property(TGT_TYPE ${tgt} TYPE)

	if(${TGT_TYPE} STREQUAL "INTERFACE_LIBRARY")
		target_link_libraries(${tgt} INTERFACE metacpp::headers metacpp::refl)
	else()
		target_link_libraries(${tgt} PRIVATE metacpp::headers metacpp::refl)
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
	get_target_property(TGT_INTERFACE_INCLUDE_DIRS ${tgt} INTERFACE_INCLUDE_DIRECTORIES)
	get_target_property(TGT_SOURCE_DIR ${tgt} SOURCE_DIR)

	list(APPEND TGT_INCLUDE_DIRS "${TGT_INTERFACE_INCLUDE_DIRS}")

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

	if(${TGT_TYPE} STREQUAL "EXECUTABLE")
		set_target_properties(
			${tgt} PROPERTIES
			POSITION_INDEPENDENT_CODE OFF
			ENABLE_EXPORTS ON
		)
	endif()

	set(INPUT_HEADERS "")

	string(MD5 TGT_HASH "${tgt}")

	set(OUTPUT_DIR "${TGT_BINARY_DIR}/reflect/${TGT_HASH}")
	set(OUTPUT_HEADERS "")
	set(OUTPUT_SOURCES "")

	file(MAKE_DIRECTORY ${OUTPUT_DIR})

	if(${TGT_TYPE} STREQUAL "INTERFACE_LIBRARY")
		target_include_directories(${tgt} INTERFACE ${OUTPUT_DIR})
	else()
		target_include_directories(${tgt} PUBLIC ${OUTPUT_DIR})
	endif()

	foreach(SRC IN LISTS TGT_SOURCES)
		cmake_path(GET SRC EXTENSION TGT_SRC_EXT)
		if(TGT_SRC_EXT MATCHES "(\\.hpp)|(\\.h)")
			cmake_path(ABSOLUTE_PATH SRC OUTPUT_VARIABLE SRC_ABSOLUTE)

			message(DEBUG "header path:       ${SRC_ABSOLUTE}")

			set(TGT_OUTPUT_DIR "${OUTPUT_DIR}")

			foreach(dir IN LISTS TGT_INCLUDE_DIRS)
				cmake_path(ABSOLUTE_PATH dir OUTPUT_VARIABLE DIR_ABSOLUTE)

				string(LENGTH "${DIR_ABSOLUTE}/" DIR_LENGTH)
				string(SUBSTRING "${SRC_ABSOLUTE}" 0 ${DIR_LENGTH} PATH_INITIAL)

				message(DEBUG "include dir:       ${DIR_ABSOLUTE}")

				if(PATH_INITIAL STREQUAL "${DIR_ABSOLUTE}/")
					cmake_path(GET SRC_ABSOLUTE PARENT_PATH PATH_DIR)
					string(SUBSTRING "${PATH_DIR}" ${DIR_LENGTH} -1 PATH_RELATIVE)
					set(TGT_OUTPUT_DIR "${OUTPUT_DIR}/${PATH_RELATIVE}")

					message(DEBUG "absolute path:     ${SRC_ABSOLUTE}")
					message(DEBUG "path initial:      ${PATH_INITIAL}")
					message(DEBUG "path parent:       ${PATH_DIR}")
					message(DEBUG "path relative:     ${PATH_RELATIVE}")

					break()
				endif()
			endforeach()

			list(APPEND INPUT_HEADERS "${SRC}")

			cmake_path(GET SRC EXTENSION TGT_EXTENSION)
			cmake_path(GET SRC FILENAME TGT_HEADER)
			cmake_path(GET SRC FILENAME TGT_HEADER_OUT)
			cmake_path(REPLACE_EXTENSION TGT_HEADER_OUT ".meta${TGT_EXTENSION}" OUTPUT_VARIABLE TGT_HEADER_OUT)

			set(TGT_HEADER_OUTPUT "${TGT_OUTPUT_DIR}/${TGT_HEADER_OUT}")
			set(TGT_SOURCE_OUTPUT "${TGT_OUTPUT_DIR}/${TGT_HEADER}.refl.cpp")

			message(DEBUG "Meta information for '${SRC}' output to ${TGT_HEADER_OUTPUT}")

			list(APPEND OUTPUT_HEADERS "${TGT_HEADER_OUTPUT}")
			list(APPEND OUTPUT_SOURCES "${TGT_SOURCE_OUTPUT}")

			if(${TGT_TYPE} STREQUAL "INTERFACE_LIBRARY")
				target_sources(${tgt} INTERFACE "${TGT_HEADER_OUTPUT}")
				target_sources(${tgt} INTERFACE "${TGT_SOURCE_OUTPUT}")
			else()
				target_sources(${tgt} PUBLIC "${TGT_HEADER_OUTPUT}")
				target_sources(${tgt} PRIVATE "${TGT_SOURCE_OUTPUT}")
			endif()
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
		target_include_directories(${tgt} INTERFACE ${OUTPUT_DIR})
	else()
		target_include_directories(${tgt} PUBLIC ${OUTPUT_DIR})
	endif()
endfunction()
