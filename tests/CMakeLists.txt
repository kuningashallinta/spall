find_package(Catch2 CONFIG REQUIRED)

add_executable(spall_tests
	"Backend/BackendFactoryTests.cpp"
	"CommandList/BlockCompressedCopyTests.cpp"
	"CommandList/CommandListInterfaceTests.cpp"
	"CommandList/ParallelRecordingTests.cpp"
	"Common/Resource/ResourceTests.cpp"
	"Common/Resource/SharedObjectTests.cpp"
	"Device/ConvenienceApiTests.cpp"
	"Device/DeviceCapabilityTests.cpp"
	"Validation/AccelerationStructureValidationTests.cpp"
	"Validation/BufferValidationTests.cpp"
	"Validation/CommandStateValidationTests.cpp"
	"Validation/CommandValidationTests.cpp"
	"Validation/FlagValidationTests.cpp"
	"Validation/FormatValidationTests.cpp"
	"Validation/FramebufferValidationTests.cpp"
	"Validation/PipelineValidationTests.cpp"
	"Validation/QueryValidationTests.cpp"
	"Validation/RenderPassValidationTests.cpp"
	"Validation/ResourceSetValidationTests.cpp"
	"Validation/ResourceStateValidationTests.cpp"
	"Validation/SamplerValidationTests.cpp"
	"Validation/ShaderValidationTests.cpp"
	"Validation/SwapChainValidationTests.cpp"
	"Validation/TextureValidationTests.cpp")

if(SPALL_BUILD_RUNTIME)
	target_sources(spall_tests PRIVATE
		"Runtime/RendererFrameTests.cpp"
		"Runtime/RendererTests.cpp")

	target_link_libraries(spall_tests PRIVATE spall::runtime)
endif()

if(SPALL_ENABLE_D3D12)
	target_sources(spall_tests PRIVATE
		"Backends/D3D12/D3D12DeviceTests.cpp"
		"Backends/D3D12/D3D12IndirectCommandTests.cpp"
		"Backends/D3D12/D3D12PipelineMappingTests.cpp"
		"Backends/D3D12/D3D12RayTracingLayoutTests.cpp"
		"Backends/D3D12/D3D12RayTracingTests.cpp"
		"Backends/D3D12/DXGIDebugLabelTests.cpp")

	target_link_libraries(spall_tests PRIVATE d3dcompiler)
endif()

if(SPALL_ENABLE_VULKAN)
	target_sources(spall_tests PRIVATE
		"Backends/Vulkan/RenderPassKeyTests.cpp"
		"Backends/Vulkan/VulkanDebugMarkerTests.cpp"
		"Backends/Vulkan/VulkanFormatMappingTests.cpp"
		"Backends/Vulkan/VulkanIndirectCommandTests.cpp"
		"Backends/Vulkan/VulkanPipelineMappingTests.cpp"
		"Backends/Vulkan/VulkanRayTracingLayoutTests.cpp"
		"Backends/Vulkan/VulkanRayTracingTests.cpp")
endif()

spall_configure_target(spall_tests 20)

target_include_directories(spall_tests PRIVATE "${PROJECT_SOURCE_DIR}")

target_sources(spall_tests PRIVATE
	"Support/Fakes.h"
	"Support/TestDevice.h"
	"Support/TestWindow.h")

if(SPALL_ENABLE_D3D12 OR SPALL_ENABLE_VULKAN)
	target_sources(spall_tests PRIVATE
		"Shaders/RayQueryShaders.h"
		"Shaders/RayQuery.hlsl"
		"Shaders/RayQueryProcedural.hlsl"
		"Shaders/RayTracingPipeline.hlsl"
		"Shaders/RayTracingPipelineProcedural.hlsl")

	set_source_files_properties("Shaders/RayQuery.hlsl" PROPERTIES HEADER_FILE_ONLY ON)
	set_source_files_properties("Shaders/RayQueryProcedural.hlsl" PROPERTIES HEADER_FILE_ONLY ON)
	set_source_files_properties("Shaders/RayTracingPipeline.hlsl" PROPERTIES HEADER_FILE_ONLY ON)
	set_source_files_properties("Shaders/RayTracingPipelineProcedural.hlsl" PROPERTIES HEADER_FILE_ONLY ON)
endif()

if(SPALL_ENABLE_D3D12)
	target_sources(spall_tests PRIVATE "Support/RayTracingTestDevice.h")
endif()

target_link_libraries(spall_tests PRIVATE
	spall::spall
	spall::backends
	Catch2::Catch2WithMain)

if(SPALL_ENABLE_VULKAN)
	target_link_libraries(spall_tests PRIVATE Vulkan::Vulkan)
endif()

add_test(NAME spall_tests COMMAND spall_tests)
