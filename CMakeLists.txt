cmake_minimum_required(VERSION 4.2)

project(spall
	DESCRIPTION "Spall RHI"
	LANGUAGES CXX)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(WIN32)
	option(SPALL_ENABLE_D3D12 "Build the D3D12 backend" ON)
else()
	option(SPALL_ENABLE_D3D12 "Build the D3D12 backend" OFF)
endif()

option(SPALL_ENABLE_VULKAN "Build the Vulkan backend" ON)
option(SPALL_BUILD_RUNTIME "Build the optional Spall RHI runtime layer" ON)
option(SPALL_BUILD_EXAMPLES "Build the Spall RHI examples" ${PROJECT_IS_TOP_LEVEL})
option(SPALL_BUILD_TESTS "Build the Spall RHI tests" ${PROJECT_IS_TOP_LEVEL})

if(NOT SPALL_ENABLE_D3D12 AND NOT SPALL_ENABLE_VULKAN)
	message(FATAL_ERROR "Spall RHI requires at least one enabled rendering backend.")
endif()

if(SPALL_ENABLE_D3D12 AND NOT WIN32)
	message(FATAL_ERROR "The D3D12 backend requires Windows. Set SPALL_ENABLE_D3D12=OFF to build the Vulkan backend on this platform.")
endif()

if(SPALL_ENABLE_VULKAN AND NOT WIN32)
	message(FATAL_ERROR "The Vulkan backend currently creates its presentation surface through the Win32 WSI extension and requires Windows. Cross-platform surface support is not yet implemented.")
endif()

if(SPALL_ENABLE_VULKAN)
	find_package(Vulkan REQUIRED)
	find_package(VulkanMemoryAllocator CONFIG REQUIRED)
endif()

include(cmake/SpallTargets.cmake)

add_subdirectory(src)

if(SPALL_ENABLE_VULKAN AND (SPALL_BUILD_EXAMPLES OR SPALL_BUILD_TESTS))
	set(SPALL_LAYER_SETTINGS_OUTPUT "${PROJECT_BINARY_DIR}/$<CONFIG>/Output/vk_layer_settings.txt")

	add_custom_target(spall_layer_settings ALL
		COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/$<CONFIG>/Output"
		COMMAND ${CMAKE_COMMAND} -E copy_if_different
			"${CMAKE_CURRENT_SOURCE_DIR}/vk_layer_settings.txt"
			"${SPALL_LAYER_SETTINGS_OUTPUT}"
		BYPRODUCTS "${SPALL_LAYER_SETTINGS_OUTPUT}"
		VERBATIM)
endif()

if(SPALL_BUILD_EXAMPLES AND SPALL_BUILD_RUNTIME AND SPALL_ENABLE_D3D12 AND SPALL_ENABLE_VULKAN)
	add_subdirectory(examples/spinning_cube)
	add_subdirectory(examples/compute_image)
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT spinning_cube)
endif()

if(SPALL_BUILD_TESTS)
	enable_testing()
	add_subdirectory(tests)
endif()

set(SPALL_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/spall")

configure_package_config_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake/spallConfig.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/spallConfig.cmake"
	INSTALL_DESTINATION "${SPALL_INSTALL_CMAKEDIR}")

install(TARGETS ${SPALL_INSTALL_TARGETS}
	EXPORT spallTargets
	ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
	RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
	FILE_SET public_headers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(EXPORT spallTargets
	FILE spallTargets.cmake
	NAMESPACE spall::
	DESTINATION "${SPALL_INSTALL_CMAKEDIR}")

install(FILES
	"${CMAKE_CURRENT_BINARY_DIR}/spallConfig.cmake"
	DESTINATION "${SPALL_INSTALL_CMAKEDIR}")
