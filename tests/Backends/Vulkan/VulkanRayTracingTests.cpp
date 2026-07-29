#include <catch2/catch_test_macros.hpp>

#include <spall/CommandList/ICommandList.h>
#include <spall/Device/IDevice.h>
#include <spall/Pipeline/Binding/IResourceSet.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureInstance.h>
#include <spall/Resources/AccelerationStructure/IAccelerationStructure.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <tests/Shaders/RayQueryShaders.h>
#include <tests/Support/TestDevice.h>

#include <array>
#include <cstdint>
#include <cstring>
#include <span>

namespace
{
	constexpr std::uint32_t TestInstanceId = 7;

	spall::tests::TestDevice requireRayQueryDevice(
		void)
	{
		spall::tests::TestDevice testDevice = spall::tests::requireDevice(spall::RenderBackendType::Vulkan);

		if (not testDevice.Device->limits().SupportsInlineRayTracing)
		{
			SKIP("This adapter does not support Vulkan inline ray tracing.");
		}

		return testDevice;
	}

	struct Triangle
	{
		float Positions[9] = {};
	};

	Triangle triangleAtDepth(
		float depth)
	{
		return Triangle {{0.0f, 0.0f, depth,
			1.0f, 0.0f, depth,
			0.0f, 1.0f, depth}};
	}

	Triangle triangleAt(
		float depth,
		float offsetX)
	{
		return Triangle {{offsetX, 0.0f, depth,
			offsetX + 1.0f, 0.0f, depth,
			offsetX, 1.0f, depth}};
	}

	struct RayTracingScene
	{
		spall::Resource<spall::IBuffer> Vertices;
		spall::Resource<spall::IBuffer> Instances;
		spall::Resource<spall::IAccelerationStructure> BottomLevel;
		spall::Resource<spall::IAccelerationStructure> TopLevel;
	};

	spall::Resource<spall::IBuffer> createVertexBuffer(
		spall::IDevice& device,
		float depth)
	{
		spall::BufferCreateInfo info = {};
		info.Size = sizeof(Triangle);
		info.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
		info.CpuAccess = spall::MemoryAccess::Write;

		spall::Resource<spall::IBuffer> vertices;
		REQUIRE(device.resources().createBuffer(info, &vertices) == spall::SUCCESS);

		const Triangle triangle = triangleAtDepth(depth);
		REQUIRE(device.resources().writeBuffer(*vertices, triangle.Positions) == spall::SUCCESS);

		return vertices;
	}

	spall::Resource<spall::IBuffer> createVertexBufferAt(
		spall::IDevice& device,
		float depth,
		float offsetX)
	{
		spall::BufferCreateInfo info = {};
		info.Size = sizeof(Triangle);
		info.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
		info.CpuAccess = spall::MemoryAccess::Write;

		spall::Resource<spall::IBuffer> vertices;
		REQUIRE(device.resources().createBuffer(info, &vertices) == spall::SUCCESS);

		const Triangle triangle = triangleAt(depth, offsetX);
		REQUIRE(device.resources().writeBuffer(*vertices, triangle.Positions) == spall::SUCCESS);

		return vertices;
	}

	spall::AccelerationStructureGeometry triangleGeometry(
		spall::IBuffer& vertices)
	{
		spall::AccelerationStructureGeometry geometry = {};
		geometry.VertexBuffer = &vertices;
		geometry.VertexFormat = spall::Format::RGB32Float;
		geometry.VertexStride = sizeof(float) * 3;
		geometry.VertexCount = 3;

		return geometry;
	}

	RayTracingScene createScene(
		spall::IDevice& device,
		float depth,
		spall::AccelerationStructureBuildFlags flags = spall::AccelerationStructureBuildFlags::PreferFastTrace)
	{
		RayTracingScene scene = {};
		scene.Vertices = createVertexBuffer(device, depth);

		const spall::AccelerationStructureGeometry geometry = triangleGeometry(*scene.Vertices);

		spall::AccelerationStructureCreateInfo bottomLevelInfo = {};
		bottomLevelInfo.Type = spall::AccelerationStructureType::BottomLevel;
		bottomLevelInfo.Flags = flags;
		bottomLevelInfo.Geometries = std::span {&geometry, 1};

		REQUIRE(device.resources().createAccelerationStructure(bottomLevelInfo, &scene.BottomLevel) == spall::SUCCESS);

		spall::BufferCreateInfo instanceBufferInfo = {};
		instanceBufferInfo.Size = sizeof(spall::AccelerationStructureInstance);
		instanceBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
		instanceBufferInfo.CpuAccess = spall::MemoryAccess::Write;

		REQUIRE(device.resources().createBuffer(instanceBufferInfo, &scene.Instances) == spall::SUCCESS);

		constexpr float identity[12] = {
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f};

		const spall::AccelerationStructureInstance instance = spall::makeAccelerationStructureInstance(
			*scene.BottomLevel,
			identity,
			TestInstanceId);

		REQUIRE(device.resources().writeBuffer(*scene.Instances, std::span {&instance, 1}) == spall::SUCCESS);

		spall::AccelerationStructureCreateInfo topLevelInfo = {};
		topLevelInfo.Type = spall::AccelerationStructureType::TopLevel;
		topLevelInfo.Flags = flags;
		topLevelInfo.InstanceBuffer = scene.Instances.get();
		topLevelInfo.InstanceCount = 1;

		REQUIRE(device.resources().createAccelerationStructure(topLevelInfo, &scene.TopLevel) == spall::SUCCESS);

		return scene;
	}

	struct RayResult
	{
		std::uint32_t Hit = 0;
		std::uint32_t DistanceBits = 0;
		std::uint32_t InstanceId = 0;
		std::uint32_t PrimitiveIndex = 0;
	};

	float asFloat(
		std::uint32_t bits)
	{
		float value = 0.0f;
		static_assert(sizeof(value) == sizeof(bits));
		std::memcpy(&value, &bits, sizeof(value));

		return value;
	}

	std::array<RayResult, 2> traceScene(
		spall::IDevice& device,
		RayTracingScene& scene,
		bool update = false,
		std::span<const std::uint32_t> bytecode = shaders::RayQueryComputeSpirv,
		bool buildBottomLevel = true)
	{
		spall::Resource<spall::IShader> shader = device.pipelines().createShader(
			spall::ShaderStage::Compute,
			bytecode);
		REQUIRE(shader);

		const spall::ResourceBindingInfo bindings[] = {
			{0, spall::ResourceBindingType::AccelerationStructure, spall::ShaderStageFlags::Compute},
			{1, spall::ResourceBindingType::StorageBuffer, spall::ShaderStageFlags::Compute}};

		spall::ResourceSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.Bindings = bindings;

		spall::Resource<spall::IResourceSetLayout> layout = device.pipelines().createResourceSetLayout(layoutInfo);
		REQUIRE(layout);

		spall::BufferCreateInfo resultsInfo = {};
		resultsInfo.Size = sizeof(RayResult) * 2;
		resultsInfo.Usage = spall::BufferUsageFlags::Storage | spall::BufferUsageFlags::TransferSource;

		spall::Resource<spall::IBuffer> results;
		REQUIRE(device.resources().createBuffer(resultsInfo, &results) == spall::SUCCESS);

		spall::ResourceWrite writes[2] = {};
		writes[0].Binding = 0;
		writes[0].Type = spall::ResourceBindingType::AccelerationStructure;
		writes[0].AccelerationStructure = scene.TopLevel.get();
		writes[1].Binding = 1;
		writes[1].Type = spall::ResourceBindingType::StorageBuffer;
		writes[1].Buffer = results.get();

		spall::ResourceSetCreateInfo setInfo = {};
		setInfo.Layout = layout.get();
		setInfo.Writes = writes;

		spall::Resource<spall::IResourceSet> resourceSet = device.pipelines().createResourceSet(setInfo);
		REQUIRE(resourceSet);

		const spall::IResourceSetLayout* const layouts[] = {layout.get()};

		spall::ComputePipelineCreateInfo pipelineInfo = {};
		pipelineInfo.ComputeShader.Module = shader.get();
		pipelineInfo.ComputeShader.Entry = "csMain";
		pipelineInfo.ResourceSetLayouts = layouts;

		spall::Resource<spall::IPipeline> pipeline = device.pipelines().createComputePipeline(pipelineInfo);
		REQUIRE(pipeline);

		spall::BufferCreateInfo readbackInfo = {};
		readbackInfo.Size = resultsInfo.Size;
		readbackInfo.Usage = spall::BufferUsageFlags::TransferDestination;
		readbackInfo.CpuAccess = spall::MemoryAccess::Read;

		spall::Resource<spall::IBuffer> readback;
		REQUIRE(device.resources().createBuffer(readbackInfo, &readback) == spall::SUCCESS);

		spall::Resource<spall::ICommandList> commands;
		REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);

		spall::AccelerationStructureBuildInfo buildInfo = {};
		buildInfo.Update = update;

		REQUIRE(commands->begin() == spall::SUCCESS);

		if (buildBottomLevel)
		{
			REQUIRE(commands->buildAccelerationStructure(*scene.BottomLevel, buildInfo) == spall::SUCCESS);
		}

		REQUIRE(commands->buildAccelerationStructure(*scene.TopLevel, buildInfo) == spall::SUCCESS);
		REQUIRE(commands->bindComputePipeline(*pipeline) == spall::SUCCESS);
		REQUIRE(commands->bindResourceSet(0, *resourceSet) == spall::SUCCESS);
		REQUIRE(commands->dispatch(1, 1, 1) == spall::SUCCESS);
		REQUIRE(commands->copyBuffer(*readback, 0, *results, 0, resultsInfo.Size) == spall::SUCCESS);
		REQUIRE(commands->end() == spall::SUCCESS);

		REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

		std::array<RayResult, 2> rayResults = {};
		REQUIRE(device.resources().readBuffer(*readback, std::span {rayResults}) == spall::SUCCESS);

		return rayResults;
	}

	std::array<RayResult, 2> traceScenePipeline(
		spall::IDevice& device,
		RayTracingScene& scene,
		const spall::RayTracingPipelineCreateInfo& pipelineTemplate)
	{
		const spall::ResourceBindingInfo bindings[] = {
			{0, spall::ResourceBindingType::AccelerationStructure, spall::ShaderStageFlags::RayGeneration},
			{1, spall::ResourceBindingType::StorageBuffer, spall::ShaderStageFlags::RayGeneration}};

		spall::ResourceSetLayoutCreateInfo layoutInfo = {};
		layoutInfo.Bindings = bindings;

		spall::Resource<spall::IResourceSetLayout> layout = device.pipelines().createResourceSetLayout(layoutInfo);
		REQUIRE(layout);

		spall::BufferCreateInfo resultsInfo = {};
		resultsInfo.Size = sizeof(RayResult) * 2;
		resultsInfo.Usage = spall::BufferUsageFlags::Storage | spall::BufferUsageFlags::TransferSource;

		spall::Resource<spall::IBuffer> results;
		REQUIRE(device.resources().createBuffer(resultsInfo, &results) == spall::SUCCESS);

		spall::ResourceWrite writes[2] = {};
		writes[0].Binding = 0;
		writes[0].Type = spall::ResourceBindingType::AccelerationStructure;
		writes[0].AccelerationStructure = scene.TopLevel.get();
		writes[1].Binding = 1;
		writes[1].Type = spall::ResourceBindingType::StorageBuffer;
		writes[1].Buffer = results.get();

		spall::ResourceSetCreateInfo setInfo = {};
		setInfo.Layout = layout.get();
		setInfo.Writes = writes;

		spall::Resource<spall::IResourceSet> resourceSet = device.pipelines().createResourceSet(setInfo);
		REQUIRE(resourceSet);

		const spall::IResourceSetLayout* const layouts[] = {layout.get()};

		spall::RayTracingPipelineCreateInfo pipelineInfo = pipelineTemplate;
		pipelineInfo.ResourceSetLayouts = layouts;

		spall::Resource<spall::IPipeline> pipeline = device.pipelines().createRayTracingPipeline(pipelineInfo);
		REQUIRE(pipeline);

		spall::BufferCreateInfo readbackInfo = {};
		readbackInfo.Size = resultsInfo.Size;
		readbackInfo.Usage = spall::BufferUsageFlags::TransferDestination;
		readbackInfo.CpuAccess = spall::MemoryAccess::Read;

		spall::Resource<spall::IBuffer> readback;
		REQUIRE(device.resources().createBuffer(readbackInfo, &readback) == spall::SUCCESS);

		spall::Resource<spall::ICommandList> commands;
		REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);

		REQUIRE(commands->begin() == spall::SUCCESS);
		REQUIRE(commands->buildAccelerationStructure(*scene.BottomLevel) == spall::SUCCESS);
		REQUIRE(commands->buildAccelerationStructure(*scene.TopLevel) == spall::SUCCESS);
		REQUIRE(commands->bindRayTracingPipeline(*pipeline) == spall::SUCCESS);
		REQUIRE(commands->bindResourceSet(0, *resourceSet) == spall::SUCCESS);
		REQUIRE(commands->dispatchRays(2, 1, 1) == spall::SUCCESS);
		REQUIRE(commands->copyBuffer(*readback, 0, *results, 0, resultsInfo.Size) == spall::SUCCESS);
		REQUIRE(commands->end() == spall::SUCCESS);

		REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

		std::array<RayResult, 2> rayResults = {};
		REQUIRE(device.resources().readBuffer(*readback, std::span {rayResults}) == spall::SUCCESS);

		return rayResults;
	}
} // namespace

TEST_CASE(
	"A Vulkan device builds a bottom-level acceleration structure",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::IBuffer> vertices = createVertexBuffer(device, 0.0f);
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(*vertices);

	spall::AccelerationStructureCreateInfo info = {};
	info.Type = spall::AccelerationStructureType::BottomLevel;
	info.Geometries = std::span {&geometry, 1};

	spall::Resource<spall::IAccelerationStructure> bottomLevel;
	REQUIRE(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::SUCCESS);

	CHECK(bottomLevel->info().Size != 0);
	CHECK(bottomLevel->info().BuildScratchSize != 0);
	CHECK(bottomLevel->info().GeometryCount == 1);
	CHECK(bottomLevel->deviceAddress() != 0);

	CHECK(bottomLevel->info().UpdateScratchSize == 0);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->buildAccelerationStructure(*bottomLevel) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);

	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);
}

TEST_CASE(
	"A Vulkan inline ray query hits a built triangle",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	RayTracingScene scene = createScene(device, 0.0f);
	const std::array<RayResult, 2> results = traceScene(device, scene);

	CHECK(results[0].Hit == 1);
	CHECK(results[0].InstanceId == TestInstanceId);
	CHECK(results[0].PrimitiveIndex == 0);
	CHECK(asFloat(results[0].DistanceBits) == 1.0f);

	CHECK(results[1].Hit == 0);
}

TEST_CASE(
	"A Vulkan ray-tracing pipeline traces a triangle",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	if (not device.limits().SupportsRayTracingPipeline)
	{
		SKIP("This adapter does not support ray-tracing pipelines.");
	}

	RayTracingScene scene = createScene(device, 0.0f);

	spall::Resource<spall::IShader> rayGenShader = device.pipelines().createShader(
		spall::ShaderStage::RayGeneration,
		shaders::RayTracingPipelineLibrarySpirv);
	spall::Resource<spall::IShader> missShader = device.pipelines().createShader(
		spall::ShaderStage::Miss,
		shaders::RayTracingPipelineLibrarySpirv);
	spall::Resource<spall::IShader> closestHitShader = device.pipelines().createShader(
		spall::ShaderStage::ClosestHit,
		shaders::RayTracingPipelineLibrarySpirv);
	REQUIRE(rayGenShader);
	REQUIRE(missShader);
	REQUIRE(closestHitShader);

	const spall::PipelineShaderStageInfo missShaders[] = {{missShader.get(), "missMain"}};
	const spall::RayTracingHitGroup hitGroups[] = {{{closestHitShader.get(), "closestHitMain"}, {}, {}}};

	spall::RayTracingPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.RayGenerationShader = {rayGenShader.get(), "rayGenMain"};
	pipelineInfo.MissShaders = missShaders;
	pipelineInfo.HitGroups = hitGroups;
	pipelineInfo.MaxPayloadSize = 16;

	const std::array<RayResult, 2> results = traceScenePipeline(device, scene, pipelineInfo);

	CHECK(results[0].Hit == 1);
	CHECK(results[0].InstanceId == TestInstanceId);
	CHECK(results[0].PrimitiveIndex == 0);
	CHECK(asFloat(results[0].DistanceBits) == 1.0f);

	CHECK(results[1].Hit == 0);
}

TEST_CASE(
	"A Vulkan ray-tracing pipeline intersects a procedural bounding box",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	if (not device.limits().SupportsRayTracingPipeline)
	{
		SKIP("This adapter does not support ray-tracing pipelines.");
	}

	RayTracingScene scene = {};

	spall::BufferCreateInfo aabbBufferInfo = {};
	aabbBufferInfo.Size = sizeof(spall::AccelerationStructureAabb);
	aabbBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
	aabbBufferInfo.CpuAccess = spall::MemoryAccess::Write;

	REQUIRE(device.resources().createBuffer(aabbBufferInfo, &scene.Vertices) == spall::SUCCESS);

	const spall::AccelerationStructureAabb box = {0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.5f};
	REQUIRE(device.resources().writeBuffer(*scene.Vertices, std::span {&box, 1}) == spall::SUCCESS);

	spall::AccelerationStructureGeometry geometry = {};
	geometry.Type = spall::AccelerationStructureGeometryType::Aabbs;
	geometry.AabbBuffer = scene.Vertices.get();
	geometry.AabbCount = 1;

	spall::AccelerationStructureCreateInfo bottomLevelInfo = {};
	bottomLevelInfo.Type = spall::AccelerationStructureType::BottomLevel;
	bottomLevelInfo.Geometries = std::span {&geometry, 1};

	REQUIRE(device.resources().createAccelerationStructure(bottomLevelInfo, &scene.BottomLevel) == spall::SUCCESS);

	spall::BufferCreateInfo instanceBufferInfo = {};
	instanceBufferInfo.Size = sizeof(spall::AccelerationStructureInstance);
	instanceBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
	instanceBufferInfo.CpuAccess = spall::MemoryAccess::Write;

	REQUIRE(device.resources().createBuffer(instanceBufferInfo, &scene.Instances) == spall::SUCCESS);

	constexpr float identity[12] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f};

	const spall::AccelerationStructureInstance instance = spall::makeAccelerationStructureInstance(
		*scene.BottomLevel,
		identity,
		TestInstanceId);

	REQUIRE(device.resources().writeBuffer(*scene.Instances, std::span {&instance, 1}) == spall::SUCCESS);

	spall::AccelerationStructureCreateInfo topLevelInfo = {};
	topLevelInfo.Type = spall::AccelerationStructureType::TopLevel;
	topLevelInfo.InstanceBuffer = scene.Instances.get();
	topLevelInfo.InstanceCount = 1;

	REQUIRE(device.resources().createAccelerationStructure(topLevelInfo, &scene.TopLevel) == spall::SUCCESS);

	spall::Resource<spall::IShader> rayGenShader = device.pipelines().createShader(
		spall::ShaderStage::RayGeneration,
		shaders::RayTracingPipelineProceduralLibrarySpirv);
	spall::Resource<spall::IShader> missShader = device.pipelines().createShader(
		spall::ShaderStage::Miss,
		shaders::RayTracingPipelineProceduralLibrarySpirv);
	spall::Resource<spall::IShader> closestHitShader = device.pipelines().createShader(
		spall::ShaderStage::ClosestHit,
		shaders::RayTracingPipelineProceduralLibrarySpirv);
	spall::Resource<spall::IShader> anyHitShader = device.pipelines().createShader(
		spall::ShaderStage::AnyHit,
		shaders::RayTracingPipelineProceduralLibrarySpirv);
	spall::Resource<spall::IShader> intersectionShader = device.pipelines().createShader(
		spall::ShaderStage::Intersection,
		shaders::RayTracingPipelineProceduralLibrarySpirv);

	const spall::PipelineShaderStageInfo missShaders[] = {{missShader.get(), "missMain"}};
	const spall::RayTracingHitGroup hitGroups[] = {{{closestHitShader.get(), "closestHitMain"},
		{anyHitShader.get(), "anyHitMain"},
		{intersectionShader.get(), "intersectionMain"}}};

	spall::RayTracingPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.RayGenerationShader = {rayGenShader.get(), "rayGenMain"};
	pipelineInfo.MissShaders = missShaders;
	pipelineInfo.HitGroups = hitGroups;
	pipelineInfo.MaxPayloadSize = 16;
	pipelineInfo.MaxAttributeSize = 12;

	const std::array<RayResult, 2> results = traceScenePipeline(device, scene, pipelineInfo);

	CHECK(results[0].Hit == 1);
	CHECK(asFloat(results[0].DistanceBits) == 1.5f);
	CHECK(results[0].InstanceId == TestInstanceId);
	CHECK(results[1].Hit == 0);
}

TEST_CASE(
	"A Vulkan ray-tracing pipeline selects a hit group per instance",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	if (not device.limits().SupportsRayTracingPipeline)
	{
		SKIP("This adapter does not support ray-tracing pipelines.");
	}

	RayTracingScene scene = {};
	scene.Vertices = createVertexBuffer(device, 0.0f);

	const spall::AccelerationStructureGeometry geometry = triangleGeometry(*scene.Vertices);

	spall::AccelerationStructureCreateInfo bottomLevelInfo = {};
	bottomLevelInfo.Type = spall::AccelerationStructureType::BottomLevel;
	bottomLevelInfo.Geometries = std::span {&geometry, 1};

	REQUIRE(device.resources().createAccelerationStructure(bottomLevelInfo, &scene.BottomLevel) == spall::SUCCESS);

	spall::BufferCreateInfo instanceBufferInfo = {};
	instanceBufferInfo.Size = sizeof(spall::AccelerationStructureInstance) * 2;
	instanceBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
	instanceBufferInfo.CpuAccess = spall::MemoryAccess::Write;

	REQUIRE(device.resources().createBuffer(instanceBufferInfo, &scene.Instances) == spall::SUCCESS);

	constexpr float identity[12] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f};

	constexpr float translated[12] = {
		1.0f, 0.0f, 0.0f, 4.75f,
		0.0f, 1.0f, 0.0f, 4.75f,
		0.0f, 0.0f, 1.0f, 0.0f};

	const spall::AccelerationStructureInstance instances[2] = {
		spall::makeAccelerationStructureInstance(*scene.BottomLevel, identity, 7, 0xFF, spall::AccelerationStructureInstanceFlags::None, 0),
		spall::makeAccelerationStructureInstance(*scene.BottomLevel, translated, 9, 0xFF, spall::AccelerationStructureInstanceFlags::None, 1)};

	REQUIRE(device.resources().writeBuffer(*scene.Instances, instances) == spall::SUCCESS);

	spall::AccelerationStructureCreateInfo topLevelInfo = {};
	topLevelInfo.Type = spall::AccelerationStructureType::TopLevel;
	topLevelInfo.InstanceBuffer = scene.Instances.get();
	topLevelInfo.InstanceCount = 2;

	REQUIRE(device.resources().createAccelerationStructure(topLevelInfo, &scene.TopLevel) == spall::SUCCESS);

	spall::Resource<spall::IShader> rayGenShader = device.pipelines().createShader(
		spall::ShaderStage::RayGeneration,
		shaders::RayTracingPipelineLibrarySpirv);
	spall::Resource<spall::IShader> missShader = device.pipelines().createShader(
		spall::ShaderStage::Miss,
		shaders::RayTracingPipelineLibrarySpirv);
	spall::Resource<spall::IShader> closestHitShader = device.pipelines().createShader(
		spall::ShaderStage::ClosestHit,
		shaders::RayTracingPipelineLibrarySpirv);

	const spall::PipelineShaderStageInfo missShaders[] = {{missShader.get(), "missMain"}};
	const spall::RayTracingHitGroup hitGroups[] = {
		{{closestHitShader.get(), "closestHitMain"}, {}, {}},
		{{closestHitShader.get(), "closestHitSecondMain"}, {}, {}}};

	spall::RayTracingPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.RayGenerationShader = {rayGenShader.get(), "rayGenMain"};
	pipelineInfo.MissShaders = missShaders;
	pipelineInfo.HitGroups = hitGroups;
	pipelineInfo.MaxPayloadSize = 16;

	const std::array<RayResult, 2> results = traceScenePipeline(device, scene, pipelineInfo);

	CHECK(results[0].Hit == 1);
	CHECK(results[0].InstanceId == 7);
	CHECK(results[1].Hit == 2);
	CHECK(results[1].InstanceId == 9);
}

TEST_CASE(
	"A Vulkan inline ray query hits the second geometry of a bottom-level structure",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::IBuffer> offsetVertices = createVertexBufferAt(device, 0.0f, 4.0f);
	spall::Resource<spall::IBuffer> tracedVertices = createVertexBufferAt(device, 0.5f, 0.0f);

	const spall::AccelerationStructureGeometry geometries[2] = {
		triangleGeometry(*offsetVertices),
		triangleGeometry(*tracedVertices)};

	RayTracingScene scene = {};

	spall::AccelerationStructureCreateInfo bottomLevelInfo = {};
	bottomLevelInfo.Type = spall::AccelerationStructureType::BottomLevel;
	bottomLevelInfo.Geometries = geometries;

	REQUIRE(device.resources().createAccelerationStructure(bottomLevelInfo, &scene.BottomLevel) == spall::SUCCESS);
	CHECK(scene.BottomLevel->info().GeometryCount == 2);

	spall::BufferCreateInfo instanceBufferInfo = {};
	instanceBufferInfo.Size = sizeof(spall::AccelerationStructureInstance);
	instanceBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
	instanceBufferInfo.CpuAccess = spall::MemoryAccess::Write;

	REQUIRE(device.resources().createBuffer(instanceBufferInfo, &scene.Instances) == spall::SUCCESS);

	constexpr float identity[12] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f};

	const spall::AccelerationStructureInstance instance = spall::makeAccelerationStructureInstance(
		*scene.BottomLevel,
		identity,
		TestInstanceId);

	REQUIRE(device.resources().writeBuffer(*scene.Instances, std::span {&instance, 1}) == spall::SUCCESS);

	spall::AccelerationStructureCreateInfo topLevelInfo = {};
	topLevelInfo.Type = spall::AccelerationStructureType::TopLevel;
	topLevelInfo.InstanceBuffer = scene.Instances.get();
	topLevelInfo.InstanceCount = 1;

	REQUIRE(device.resources().createAccelerationStructure(topLevelInfo, &scene.TopLevel) == spall::SUCCESS);

	const std::array<RayResult, 2> results = traceScene(device, scene);

	CHECK(results[0].Hit == 1);
	CHECK(asFloat(results[0].DistanceBits) == 1.5f);
	CHECK(results[0].InstanceId == TestInstanceId);
	CHECK(results[1].Hit == 0);
}

TEST_CASE(
	"A Vulkan inline ray query hits the second instance of a top-level structure",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	RayTracingScene scene = {};
	scene.Vertices = createVertexBufferAt(device, 0.5f, 0.0f);

	const spall::AccelerationStructureGeometry geometry = triangleGeometry(*scene.Vertices);

	spall::AccelerationStructureCreateInfo bottomLevelInfo = {};
	bottomLevelInfo.Type = spall::AccelerationStructureType::BottomLevel;
	bottomLevelInfo.Geometries = std::span {&geometry, 1};

	REQUIRE(device.resources().createAccelerationStructure(bottomLevelInfo, &scene.BottomLevel) == spall::SUCCESS);

	spall::BufferCreateInfo instanceBufferInfo = {};
	instanceBufferInfo.Size = sizeof(spall::AccelerationStructureInstance) * 2;
	instanceBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
	instanceBufferInfo.CpuAccess = spall::MemoryAccess::Write;

	REQUIRE(device.resources().createBuffer(instanceBufferInfo, &scene.Instances) == spall::SUCCESS);

	constexpr float translated[12] = {
		1.0f, 0.0f, 0.0f, 4.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f};

	constexpr float identity[12] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f};

	const spall::AccelerationStructureInstance instances[2] = {
		spall::makeAccelerationStructureInstance(*scene.BottomLevel, translated, 3),
		spall::makeAccelerationStructureInstance(*scene.BottomLevel, identity, TestInstanceId)};

	REQUIRE(device.resources().writeBuffer(*scene.Instances, instances) == spall::SUCCESS);

	spall::AccelerationStructureCreateInfo topLevelInfo = {};
	topLevelInfo.Type = spall::AccelerationStructureType::TopLevel;
	topLevelInfo.InstanceBuffer = scene.Instances.get();
	topLevelInfo.InstanceCount = 2;

	REQUIRE(device.resources().createAccelerationStructure(topLevelInfo, &scene.TopLevel) == spall::SUCCESS);

	const std::array<RayResult, 2> results = traceScene(device, scene);

	CHECK(results[0].Hit == 1);
	CHECK(results[0].InstanceId == TestInstanceId);
	CHECK(asFloat(results[0].DistanceBits) == 1.5f);
	CHECK(results[1].Hit == 0);
}

TEST_CASE(
	"A Vulkan inline ray query intersects a procedural bounding box",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	RayTracingScene scene = {};

	spall::BufferCreateInfo aabbBufferInfo = {};
	aabbBufferInfo.Size = sizeof(spall::AccelerationStructureAabb);
	aabbBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
	aabbBufferInfo.CpuAccess = spall::MemoryAccess::Write;

	REQUIRE(device.resources().createBuffer(aabbBufferInfo, &scene.Vertices) == spall::SUCCESS);

	const spall::AccelerationStructureAabb box = {0.0f, 0.0f, 0.5f, 1.0f, 1.0f, 1.5f};
	REQUIRE(device.resources().writeBuffer(*scene.Vertices, std::span {&box, 1}) == spall::SUCCESS);

	spall::AccelerationStructureGeometry geometry = {};
	geometry.Type = spall::AccelerationStructureGeometryType::Aabbs;
	geometry.AabbBuffer = scene.Vertices.get();
	geometry.AabbCount = 1;

	spall::AccelerationStructureCreateInfo bottomLevelInfo = {};
	bottomLevelInfo.Type = spall::AccelerationStructureType::BottomLevel;
	bottomLevelInfo.Geometries = std::span {&geometry, 1};

	REQUIRE(device.resources().createAccelerationStructure(bottomLevelInfo, &scene.BottomLevel) == spall::SUCCESS);
	CHECK(scene.BottomLevel->info().GeometryCount == 1);

	spall::BufferCreateInfo instanceBufferInfo = {};
	instanceBufferInfo.Size = sizeof(spall::AccelerationStructureInstance);
	instanceBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
	instanceBufferInfo.CpuAccess = spall::MemoryAccess::Write;

	REQUIRE(device.resources().createBuffer(instanceBufferInfo, &scene.Instances) == spall::SUCCESS);

	constexpr float identity[12] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f};

	const spall::AccelerationStructureInstance instance = spall::makeAccelerationStructureInstance(
		*scene.BottomLevel,
		identity,
		TestInstanceId);

	REQUIRE(device.resources().writeBuffer(*scene.Instances, std::span {&instance, 1}) == spall::SUCCESS);

	spall::AccelerationStructureCreateInfo topLevelInfo = {};
	topLevelInfo.Type = spall::AccelerationStructureType::TopLevel;
	topLevelInfo.InstanceBuffer = scene.Instances.get();
	topLevelInfo.InstanceCount = 1;

	REQUIRE(device.resources().createAccelerationStructure(topLevelInfo, &scene.TopLevel) == spall::SUCCESS);

	const std::array<RayResult, 2> results = traceScene(
		device,
		scene,
		false,
		shaders::RayQueryProceduralComputeSpirv);

	CHECK(results[0].Hit == 1);
	CHECK(asFloat(results[0].DistanceBits) == 1.5f);
	CHECK(results[0].InstanceId == TestInstanceId);
	CHECK(results[0].PrimitiveIndex == 0);
	CHECK(results[1].Hit == 0);
}

TEST_CASE(
	"A Vulkan acceleration structure shrinks when compacted and still traces",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	RayTracingScene scene = {};
	scene.Vertices = createVertexBuffer(device, 0.0f);

	const spall::AccelerationStructureGeometry geometry = triangleGeometry(*scene.Vertices);

	spall::AccelerationStructureCreateInfo bottomLevelInfo = {};
	bottomLevelInfo.Type = spall::AccelerationStructureType::BottomLevel;
	bottomLevelInfo.Flags = spall::AccelerationStructureBuildFlags::PreferFastTrace |
		spall::AccelerationStructureBuildFlags::AllowCompaction;
	bottomLevelInfo.Geometries = std::span {&geometry, 1};

	REQUIRE(device.resources().createAccelerationStructure(bottomLevelInfo, &scene.BottomLevel) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->buildAccelerationStructure(*scene.BottomLevel) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	const std::uint64_t sizeBeforeCompaction = scene.BottomLevel->info().Size;
	const std::uint64_t addressBeforeCompaction = scene.BottomLevel->deviceAddress();

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->compactAccelerationStructure(*scene.BottomLevel) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	CHECK(scene.BottomLevel->info().Size <= sizeBeforeCompaction);
	CHECK(scene.BottomLevel->deviceAddress() != addressBeforeCompaction);

	spall::BufferCreateInfo instanceBufferInfo = {};
	instanceBufferInfo.Size = sizeof(spall::AccelerationStructureInstance);
	instanceBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
	instanceBufferInfo.CpuAccess = spall::MemoryAccess::Write;

	REQUIRE(device.resources().createBuffer(instanceBufferInfo, &scene.Instances) == spall::SUCCESS);

	constexpr float identity[12] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f};

	const spall::AccelerationStructureInstance instance = spall::makeAccelerationStructureInstance(
		*scene.BottomLevel,
		identity,
		TestInstanceId);

	REQUIRE(device.resources().writeBuffer(*scene.Instances, std::span {&instance, 1}) == spall::SUCCESS);

	spall::AccelerationStructureCreateInfo topLevelInfo = {};
	topLevelInfo.Type = spall::AccelerationStructureType::TopLevel;
	topLevelInfo.InstanceBuffer = scene.Instances.get();
	topLevelInfo.InstanceCount = 1;

	REQUIRE(device.resources().createAccelerationStructure(topLevelInfo, &scene.TopLevel) == spall::SUCCESS);

	const std::array<RayResult, 2> results = traceScene(
		device,
		scene,
		false,
		shaders::RayQueryComputeSpirv,
		false);

	CHECK(results[0].Hit == 1);
	CHECK(results[0].InstanceId == TestInstanceId);
	CHECK(asFloat(results[0].DistanceBits) == 1.0f);
	CHECK(results[1].Hit == 0);
}

TEST_CASE(
	"A Vulkan device rejects invalid acceleration-structure compaction",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::IBuffer> vertices = createVertexBuffer(device, 0.0f);
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(*vertices);

	SECTION("Compaction without the flag is rejected")
	{
		spall::AccelerationStructureCreateInfo info = {};
		info.Type = spall::AccelerationStructureType::BottomLevel;
		info.Geometries = std::span {&geometry, 1};

		spall::Resource<spall::IAccelerationStructure> bottomLevel;
		REQUIRE(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::SUCCESS);

		spall::Resource<spall::ICommandList> commands;
		REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);
		REQUIRE(commands->begin() == spall::SUCCESS);

		CHECK(commands->compactAccelerationStructure(*bottomLevel) == spall::ERR_INVALID_STATE);
	}

	SECTION("Compaction before the first build is rejected")
	{
		spall::AccelerationStructureCreateInfo info = {};
		info.Type = spall::AccelerationStructureType::BottomLevel;
		info.Flags = spall::AccelerationStructureBuildFlags::AllowCompaction;
		info.Geometries = std::span {&geometry, 1};

		spall::Resource<spall::IAccelerationStructure> bottomLevel;
		REQUIRE(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::SUCCESS);

		spall::Resource<spall::ICommandList> commands;
		REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);
		REQUIRE(commands->begin() == spall::SUCCESS);

		CHECK(commands->compactAccelerationStructure(*bottomLevel) == spall::ERR_INVALID_STATE);
	}

	SECTION("Compacting twice and rebuilding afterwards are rejected")
	{
		spall::AccelerationStructureCreateInfo info = {};
		info.Type = spall::AccelerationStructureType::BottomLevel;
		info.Flags = spall::AccelerationStructureBuildFlags::AllowCompaction;
		info.Geometries = std::span {&geometry, 1};

		spall::Resource<spall::IAccelerationStructure> bottomLevel;
		REQUIRE(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::SUCCESS);

		spall::Resource<spall::ICommandList> commands;
		REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);

		REQUIRE(commands->begin() == spall::SUCCESS);
		REQUIRE(commands->buildAccelerationStructure(*bottomLevel) == spall::SUCCESS);
		REQUIRE(commands->end() == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

		REQUIRE(commands->begin() == spall::SUCCESS);
		REQUIRE(commands->compactAccelerationStructure(*bottomLevel) == spall::SUCCESS);

		CHECK(commands->compactAccelerationStructure(*bottomLevel) == spall::ERR_INVALID_STATE);
		CHECK(commands->buildAccelerationStructure(*bottomLevel) == spall::ERR_INVALID_STATE);

		REQUIRE(commands->end() == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);
	}
}

TEST_CASE(
	"A Vulkan acceleration structure refits after an update",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	RayTracingScene scene = createScene(
		device,
		0.0f,
		spall::AccelerationStructureBuildFlags::AllowUpdate | spall::AccelerationStructureBuildFlags::PreferFastBuild);

	const std::array<RayResult, 2> before = traceScene(device, scene);
	REQUIRE(before[0].Hit == 1);
	CHECK(asFloat(before[0].DistanceBits) == 1.0f);

	const Triangle moved = triangleAtDepth(0.5f);
	REQUIRE(device.resources().writeBuffer(*scene.Vertices, moved.Positions) == spall::SUCCESS);

	const std::array<RayResult, 2> after = traceScene(device, scene, true);

	CHECK(after[0].Hit == 1);
	CHECK(asFloat(after[0].DistanceBits) == 1.5f);
	CHECK(after[1].Hit == 0);
}

TEST_CASE(
	"A Vulkan device rejects invalid acceleration-structure work",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::IBuffer> vertices = createVertexBuffer(device, 0.0f);

	SECTION("An empty geometry list is rejected")
	{
		spall::AccelerationStructureCreateInfo info = {};
		info.Type = spall::AccelerationStructureType::BottomLevel;

		spall::Resource<spall::IAccelerationStructure> bottomLevel;
		CHECK(device.resources().createAccelerationStructure(info, &bottomLevel) != spall::SUCCESS);
	}

	SECTION("A vertex buffer without acceleration-structure-input usage is rejected")
	{
		spall::BufferCreateInfo bufferInfo = {};
		bufferInfo.Size = sizeof(Triangle);
		bufferInfo.Usage = spall::BufferUsageFlags::Vertex;

		spall::Resource<spall::IBuffer> plainVertices;
		REQUIRE(device.resources().createBuffer(bufferInfo, &plainVertices) == spall::SUCCESS);

		const spall::AccelerationStructureGeometry geometry = triangleGeometry(*plainVertices);

		spall::AccelerationStructureCreateInfo info = {};
		info.Type = spall::AccelerationStructureType::BottomLevel;
		info.Geometries = std::span {&geometry, 1};

		spall::Resource<spall::IAccelerationStructure> bottomLevel;
		CHECK(device.resources().createAccelerationStructure(info, &bottomLevel) != spall::SUCCESS);
	}

	SECTION("An untraceable vertex format is rejected")
	{
		spall::AccelerationStructureGeometry geometry = triangleGeometry(*vertices);
		geometry.VertexFormat = spall::Format::RGBA8;

		spall::AccelerationStructureCreateInfo info = {};
		info.Type = spall::AccelerationStructureType::BottomLevel;
		info.Geometries = std::span {&geometry, 1};

		spall::Resource<spall::IAccelerationStructure> bottomLevel;
		CHECK(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::ERR_UNSUPPORTED_FORMAT);
	}

	SECTION("Building outside a recording is rejected")
	{
		const spall::AccelerationStructureGeometry geometry = triangleGeometry(*vertices);

		spall::AccelerationStructureCreateInfo info = {};
		info.Type = spall::AccelerationStructureType::BottomLevel;
		info.Geometries = std::span {&geometry, 1};

		spall::Resource<spall::IAccelerationStructure> bottomLevel;
		REQUIRE(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::SUCCESS);

		spall::Resource<spall::ICommandList> commands;
		REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);

		CHECK(commands->buildAccelerationStructure(*bottomLevel) != spall::SUCCESS);
	}

	SECTION("An update without update support is rejected")
	{
		const spall::AccelerationStructureGeometry geometry = triangleGeometry(*vertices);

		spall::AccelerationStructureCreateInfo info = {};
		info.Type = spall::AccelerationStructureType::BottomLevel;
		info.Geometries = std::span {&geometry, 1};

		spall::Resource<spall::IAccelerationStructure> bottomLevel;
		REQUIRE(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::SUCCESS);

		spall::Resource<spall::ICommandList> commands;
		REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);
		REQUIRE(commands->begin() == spall::SUCCESS);

		spall::AccelerationStructureBuildInfo buildInfo = {};
		buildInfo.Update = true;

		CHECK(commands->buildAccelerationStructure(*bottomLevel, buildInfo) != spall::SUCCESS);
	}

	SECTION("An update before the first build is rejected")
	{
		const spall::AccelerationStructureGeometry geometry = triangleGeometry(*vertices);

		spall::AccelerationStructureCreateInfo info = {};
		info.Type = spall::AccelerationStructureType::BottomLevel;
		info.Flags = spall::AccelerationStructureBuildFlags::AllowUpdate;
		info.Geometries = std::span {&geometry, 1};

		spall::Resource<spall::IAccelerationStructure> bottomLevel;
		REQUIRE(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::SUCCESS);

		spall::Resource<spall::ICommandList> commands;
		REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);
		REQUIRE(commands->begin() == spall::SUCCESS);

		spall::AccelerationStructureBuildInfo buildInfo = {};
		buildInfo.Update = true;

		CHECK(commands->buildAccelerationStructure(*bottomLevel, buildInfo) == spall::ERR_INVALID_STATE);
	}
}

TEST_CASE(
	"A Vulkan acceleration-structure build transitions a device-local input",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	const Triangle triangle = triangleAtDepth(0.0f);

	spall::BufferCreateInfo bufferInfo = {};
	bufferInfo.Size = sizeof(Triangle);
	bufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput | spall::BufferUsageFlags::TransferDestination;

	spall::Resource<spall::IBuffer> vertices = device.resources().createBufferWithData(bufferInfo, triangle.Positions);
	REQUIRE(vertices);

	const spall::AccelerationStructureGeometry geometry = triangleGeometry(*vertices);

	spall::AccelerationStructureCreateInfo info = {};
	info.Type = spall::AccelerationStructureType::BottomLevel;
	info.Geometries = std::span {&geometry, 1};

	spall::Resource<spall::IAccelerationStructure> bottomLevel;
	REQUIRE(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->buildAccelerationStructure(*bottomLevel) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);

	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);
}

TEST_CASE(
	"A Vulkan acceleration-structure build requires a graphics command list",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::IBuffer> vertices = createVertexBuffer(device, 0.0f);
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(*vertices);

	spall::AccelerationStructureCreateInfo info = {};
	info.Type = spall::AccelerationStructureType::BottomLevel;
	info.Geometries = std::span {&geometry, 1};

	spall::Resource<spall::IAccelerationStructure> bottomLevel;
	REQUIRE(device.resources().createAccelerationStructure(info, &bottomLevel) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> compute;
	REQUIRE(device.createCommandList(spall::QueueType::Compute, &compute) == spall::SUCCESS);
	REQUIRE(compute->begin() == spall::SUCCESS);

	CHECK(compute->buildAccelerationStructure(*bottomLevel) == spall::ERR_UNSUPPORTED);
}

TEST_CASE(
	"A Vulkan acceleration-structure update must refit the instance count its build used",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	constexpr spall::AccelerationStructureBuildFlags updatable =
		spall::AccelerationStructureBuildFlags::AllowUpdate |
		spall::AccelerationStructureBuildFlags::PreferFastBuild;

	spall::Resource<spall::IBuffer> vertices = createVertexBuffer(device, 0.0f);
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(*vertices);

	spall::AccelerationStructureCreateInfo bottomLevelInfo = {};
	bottomLevelInfo.Type = spall::AccelerationStructureType::BottomLevel;
	bottomLevelInfo.Flags = updatable;
	bottomLevelInfo.Geometries = std::span {&geometry, 1};

	spall::Resource<spall::IAccelerationStructure> bottomLevel;
	REQUIRE(device.resources().createAccelerationStructure(bottomLevelInfo, &bottomLevel) == spall::SUCCESS);

	spall::BufferCreateInfo instanceBufferInfo = {};
	instanceBufferInfo.Size = sizeof(spall::AccelerationStructureInstance) * 2;
	instanceBufferInfo.Usage = spall::BufferUsageFlags::AccelerationStructureInput;
	instanceBufferInfo.CpuAccess = spall::MemoryAccess::Write;

	spall::Resource<spall::IBuffer> instances;
	REQUIRE(device.resources().createBuffer(instanceBufferInfo, &instances) == spall::SUCCESS);

	constexpr float identity[12] = {
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f};

	const spall::AccelerationStructureInstance written[2] = {
		spall::makeAccelerationStructureInstance(*bottomLevel, identity, 0),
		spall::makeAccelerationStructureInstance(*bottomLevel, identity, 1)};

	REQUIRE(device.resources().writeBuffer(*instances, written) == spall::SUCCESS);

	spall::AccelerationStructureCreateInfo topLevelInfo = {};
	topLevelInfo.Type = spall::AccelerationStructureType::TopLevel;
	topLevelInfo.Flags = updatable;
	topLevelInfo.InstanceBuffer = instances.get();
	topLevelInfo.InstanceCount = 2;

	spall::Resource<spall::IAccelerationStructure> topLevel;
	REQUIRE(device.resources().createAccelerationStructure(topLevelInfo, &topLevel) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);
	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->buildAccelerationStructure(*bottomLevel) == spall::SUCCESS);

	spall::AccelerationStructureBuildInfo partial = {};
	partial.InstanceCount = 1;

	REQUIRE(commands->buildAccelerationStructure(*topLevel, partial) == spall::SUCCESS);

	spall::AccelerationStructureBuildInfo mismatched = {};
	mismatched.Update = true;

	CHECK(commands->buildAccelerationStructure(*topLevel, mismatched) == spall::ERR_INVALID_RANGE);

	spall::AccelerationStructureBuildInfo matched = {};
	matched.Update = true;
	matched.InstanceCount = 1;

	CHECK(commands->buildAccelerationStructure(*topLevel, matched) == spall::SUCCESS);

	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);
}

TEST_CASE(
	"A Vulkan dispatch rejects an acceleration structure that was never built",
	"[vulkan][GPU][raytracing]")
{
	const spall::tests::TestDevice testDevice = requireRayQueryDevice();
	spall::IDevice& device = *testDevice.Device;

	RayTracingScene scene = createScene(device, 0.0f);

	spall::Resource<spall::IShader> shader = device.pipelines().createShader(
		spall::ShaderStage::Compute,
		shaders::RayQueryComputeSpirv);
	REQUIRE(shader);

	const spall::ResourceBindingInfo bindings[] = {
		{0, spall::ResourceBindingType::AccelerationStructure, spall::ShaderStageFlags::Compute},
		{1, spall::ResourceBindingType::StorageBuffer, spall::ShaderStageFlags::Compute}};

	spall::ResourceSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.Bindings = bindings;

	spall::Resource<spall::IResourceSetLayout> layout = device.pipelines().createResourceSetLayout(layoutInfo);
	REQUIRE(layout);

	spall::BufferCreateInfo resultsInfo = {};
	resultsInfo.Size = sizeof(RayResult) * 2;
	resultsInfo.Usage = spall::BufferUsageFlags::Storage;

	spall::Resource<spall::IBuffer> results;
	REQUIRE(device.resources().createBuffer(resultsInfo, &results) == spall::SUCCESS);

	spall::ResourceWrite writes[2] = {};
	writes[0].Binding = 0;
	writes[0].Type = spall::ResourceBindingType::AccelerationStructure;
	writes[0].AccelerationStructure = scene.TopLevel.get();
	writes[1].Binding = 1;
	writes[1].Type = spall::ResourceBindingType::StorageBuffer;
	writes[1].Buffer = results.get();

	spall::ResourceSetCreateInfo setInfo = {};
	setInfo.Layout = layout.get();
	setInfo.Writes = writes;

	spall::Resource<spall::IResourceSet> resourceSet = device.pipelines().createResourceSet(setInfo);
	REQUIRE(resourceSet);

	const spall::IResourceSetLayout* const layouts[] = {layout.get()};

	spall::ComputePipelineCreateInfo pipelineInfo = {};
	pipelineInfo.ComputeShader.Module = shader.get();
	pipelineInfo.ComputeShader.Entry = "csMain";
	pipelineInfo.ResourceSetLayouts = layouts;

	spall::Resource<spall::IPipeline> pipeline = device.pipelines().createComputePipeline(pipelineInfo);
	REQUIRE(pipeline);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(spall::QueueType::Graphics, &commands) == spall::SUCCESS);

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->bindComputePipeline(*pipeline) == spall::SUCCESS);
	REQUIRE(commands->bindResourceSet(0, *resourceSet) == spall::SUCCESS);

	CHECK(commands->dispatch(1, 1, 1) == spall::ERR_INVALID_STATE);
}
