# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

add_library(metacpp-plugin STATIC plugin.cpp)
add_library(metacpp::plugin ALIAS metacpp-plugin)
add_library(metapp::plugin ALIAS metacpp-plugin)

target_link_libraries(metacpp-plugin PRIVATE metacpp::refl Boost::system)

if(METACPP_IPO_SUPPORTED)
	set_target_properties(
		metacpp-plugin PROPERTIES INTERPROCEDURAL_OPTIMIZATION ON
	)
endif()

function(add_plugin tgt)
	add_library(${tgt} SHARED)

	target_link_libraries(${tgt} PRIVATE metacpp::attribs)

	set(DO_REFLECT OFF)
	set(CUR_MODE "SOURCES")

	foreach(arg IN LISTS ARGN)
		if("${arg}" STREQUAL "REFLECT")
			set(DO_REFLECT ON)
		elseif("${arg}" STREQUAL "INCLUDE_DIRS")
			set(CUR_MODE "INCLUDE_DIRS")
		elseif("${arg}" STREQUAL "SOURCES")
			set(CUR_MODE "SOURCES")
		elseif("${arg}" STREQUAL "LIBRARIES")
			set(CUR_MODE "LIBRARIES")
		elseif("${CUR_MODE}" STREQUAL "INCLUDE_DIRS")
			target_include_directories(${tgt} PRIVATE ${arg})
		elseif("${CUR_MODE}" STREQUAL "SOURCES")
			target_sources(${tgt} PRIVATE ${arg})
		elseif("${CUR_MODE}" STREQUAL "LIBRARIES")
			target_link_libraries(${tgt} PRIVATE ${arg})
		else()
			message(FATAL_ERROR "Unexpected argument to add_plugin: ${arg}")
		endif()
	endforeach()

	if(DO_REFLECT)
		target_reflect(${tgt})
	endif()
endfunction()

function(target_link_plugins tgt)
	target_link_libraries(${tgt} PRIVATE metacpp::headers metacpp::plugin)

	add_custom_command(
		TARGET ${tgt}
		POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_PROPERTY:${tgt},BINARY_DIR>
		VERBATIM
	)

	foreach(plugin IN LISTS ARGN)
		add_dependencies(${tgt} ${plugin})

		add_custom_command(
			TARGET ${tgt}
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${plugin}> $<TARGET_PROPERTY:${tgt},BINARY_DIR>
			VERBATIM
		)
	endforeach()
endfunction()
