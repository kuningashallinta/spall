# ---------------------------------------------------------------------------
# spall::spall — backend-agnostic public interface
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SPALL_CORE_HEADERS CONFIGURE_DEPENDS
	"${PROJECT_SOURCE_DIR}/include/*.h"
	"${PROJECT_SOURCE_DIR}/include/*.inl"
	"${PROJECT_SOURCE_DIR}/include/*.inc")

list(FILTER SPALL_CORE_HEADERS EXCLUDE REGEX "/include/spall/Backends/")
list(FILTER SPALL_CORE_HEADERS EXCLUDE REGEX "/include/spall/Backend/BackendFactory\\.h$")
list(FILTER SPALL_CORE_HEADERS EXCLUDE REGEX "/include/spall/Runtime/")

spall_static_library(spall 20)
add_library(spall::spall ALIAS spall)
set_target_properties(spall PROPERTIES EXPORT_NAME spall)

target_sources(spall
	PUBLIC
		FILE_SET public_headers
		TYPE HEADERS
		BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
		FILES ${SPALL_CORE_HEADERS})

target_compile_features(spall PUBLIC cxx_std_20)

target_compile_definitions(spall PUBLIC
	SPALL_HAS_D3D12=$<BOOL:${SPALL_ENABLE_D3D12}>
	SPALL_HAS_VULKAN=$<BOOL:${SPALL_ENABLE_VULKAN}>)

target_include_directories(spall
	PUBLIC
		$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
		$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set(SPALL_INSTALL_TARGETS spall)

# Internal shared components pulled in by every backend.
file(GLOB_RECURSE SPALL_INTERNAL_SOURCES CONFIGURE_DEPENDS
	"${CMAKE_CURRENT_SOURCE_DIR}/Common/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Common/*.inl"
	"${CMAKE_CURRENT_SOURCE_DIR}/Common/*.inc"
	"${CMAKE_CURRENT_SOURCE_DIR}/Validation/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Common/*.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Common/*.inl"
	"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Common/*.inc")

# ---------------------------------------------------------------------------
# spall::d3d12 — Direct3D 12 backend (Windows only)
# ---------------------------------------------------------------------------
if(SPALL_ENABLE_D3D12)
	file(GLOB_RECURSE SPALL_D3D12_SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/D3D12/*.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/D3D12/*.inl"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/D3D12/*.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/D3D12/*.inc"
		"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Backends/D3D12/*.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Backends/D3D12/*.inl"
		"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Backends/D3D12/*.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Backends/D3D12/*.inc")

	spall_static_library(spall_d3d12 20
		SOURCES ${SPALL_D3D12_SOURCES} ${SPALL_INTERNAL_SOURCES})
	add_library(spall::d3d12 ALIAS spall_d3d12)
	set_target_properties(spall_d3d12 PROPERTIES EXPORT_NAME d3d12)

	target_sources(spall_d3d12
		PUBLIC
			FILE_SET public_headers
			TYPE HEADERS
			BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
			FILES "${PROJECT_SOURCE_DIR}/include/spall/Backends/D3D12/D3D12_Backend.h")

	target_include_directories(spall_d3d12 PRIVATE "${PROJECT_SOURCE_DIR}")

	target_compile_definitions(spall_d3d12 PRIVATE
		WIN32
		_WINDOWS)

	target_link_libraries(spall_d3d12 PUBLIC
		spall::spall
		d3d12
		dcomp
		dxgi
		dxguid)

	list(APPEND SPALL_INSTALL_TARGETS spall_d3d12)
endif()

# ---------------------------------------------------------------------------
# spall::vulkan — Vulkan backend (portable)
# ---------------------------------------------------------------------------
if(SPALL_ENABLE_VULKAN)
	file(GLOB_RECURSE SPALL_VULKAN_SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/Vulkan/*.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/Vulkan/*.inl"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/Vulkan/*.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Backends/Vulkan/*.inc"
		"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Backends/Vulkan/*.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Backends/Vulkan/*.inl"
		"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Backends/Vulkan/*.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/Validation/Backends/Vulkan/*.inc")

	spall_static_library(spall_vulkan 20
		SOURCES ${SPALL_VULKAN_SOURCES} ${SPALL_INTERNAL_SOURCES})
	add_library(spall::vulkan ALIAS spall_vulkan)
	set_target_properties(spall_vulkan PROPERTIES EXPORT_NAME vulkan)

	target_sources(spall_vulkan
		PUBLIC
			FILE_SET public_headers
			TYPE HEADERS
			BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
			FILES "${PROJECT_SOURCE_DIR}/include/spall/Backends/Vulkan/VK_Backend.h")

	target_include_directories(spall_vulkan PRIVATE "${PROJECT_SOURCE_DIR}")

	target_link_libraries(spall_vulkan PUBLIC
		spall::spall
		Vulkan::Vulkan)

	target_link_libraries(spall_vulkan PRIVATE
		GPUOpen::VulkanMemoryAllocator)

	list(APPEND SPALL_INSTALL_TARGETS spall_vulkan)
endif()

# ---------------------------------------------------------------------------
# spall::backends — backend instantiation for the enabled backends
# ---------------------------------------------------------------------------
spall_static_library(spall_backends 20
	SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/Backend/BackendFactory.cpp")
add_library(spall::backends ALIAS spall_backends)
set_target_properties(spall_backends PROPERTIES EXPORT_NAME backends)

target_sources(spall_backends
	PUBLIC
		FILE_SET public_headers
		TYPE HEADERS
		BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
		FILES "${PROJECT_SOURCE_DIR}/include/spall/Backend/BackendFactory.h")

target_link_libraries(spall_backends PUBLIC spall::spall)

if(SPALL_ENABLE_D3D12)
	target_link_libraries(spall_backends PRIVATE spall::d3d12)
endif()

if(SPALL_ENABLE_VULKAN)
	target_link_libraries(spall_backends PRIVATE spall::vulkan)
endif()

list(APPEND SPALL_INSTALL_TARGETS spall_backends)

# ---------------------------------------------------------------------------
# spall::runtime — optional managed renderer and frame loop
# ---------------------------------------------------------------------------
if(SPALL_BUILD_RUNTIME)
	file(GLOB_RECURSE SPALL_RUNTIME_SOURCES CONFIGURE_DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/Runtime/*.h"
		"${CMAKE_CURRENT_SOURCE_DIR}/Runtime/*.cpp")

	file(GLOB_RECURSE SPALL_RUNTIME_HEADERS CONFIGURE_DEPENDS
		"${PROJECT_SOURCE_DIR}/include/spall/Runtime/*.h")

	spall_static_library(spall_runtime 20
		SOURCES ${SPALL_RUNTIME_SOURCES})
	add_library(spall::runtime ALIAS spall_runtime)
	set_target_properties(spall_runtime PROPERTIES EXPORT_NAME runtime)

	target_sources(spall_runtime
		PUBLIC
			FILE_SET public_headers
			TYPE HEADERS
			BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
			FILES ${SPALL_RUNTIME_HEADERS})

	target_include_directories(spall_runtime PRIVATE "${PROJECT_SOURCE_DIR}")
	target_link_libraries(spall_runtime PUBLIC spall::backends)

	list(APPEND SPALL_INSTALL_TARGETS spall_runtime)
endif()

set(SPALL_INSTALL_TARGETS ${SPALL_INSTALL_TARGETS} PARENT_SCOPE)
