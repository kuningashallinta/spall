include(CMakeParseArguments)

function(spall_configure_target target cxx_standard)
	target_compile_features(${target} PRIVATE cxx_std_${cxx_standard})
endfunction()

function(spall_static_library target cxx_standard)
	cmake_parse_arguments(ARGUMENTS "" "" "SOURCES" ${ARGN})

	if(ARGUMENTS_SOURCES)
		add_library(${target} STATIC ${ARGUMENTS_SOURCES})
	else()
		set(link_anchor "${CMAKE_CURRENT_BINARY_DIR}/${target}LinkAnchor.cpp")
		file(GENERATE
			OUTPUT "${link_anchor}"
			CONTENT "namespace spall::Detail\n{\n\tvoid ${target}LinkAnchor(void)\n\t{\n\t}\n}\n")

		add_library(${target} STATIC "${link_anchor}")
	endif()

	spall_configure_target(${target} ${cxx_standard})
endfunction()
