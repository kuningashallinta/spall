#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/AccelerationStructureValidation.h>
#include <tests/Support/Fakes.h>

#include <span>

static constexpr spall::BufferUsageFlags InputUsage = spall::BufferUsageFlags::AccelerationStructureInput;

static FakeBuffer inputBuffer(
	std::uint32_t size = 4096)
{
	return FakeBuffer(spall::BufferInfo {.Size = size, .Usage = InputUsage});
}

static spall::AccelerationStructureGeometry triangleGeometry(
	spall::IBuffer& vertexBuffer)
{
	spall::AccelerationStructureGeometry geometry = {};
	geometry.VertexBuffer = &vertexBuffer;
	geometry.VertexFormat = spall::Format::RGB32Float;
	geometry.VertexStride = 12;
	geometry.VertexCount = 3;

	return geometry;
}

static spall::AccelerationStructureGeometry aabbGeometry(
	spall::IBuffer& aabbBuffer)
{
	spall::AccelerationStructureGeometry geometry = {};
	geometry.Type = spall::AccelerationStructureGeometryType::Aabbs;
	geometry.AabbBuffer = &aabbBuffer;
	geometry.AabbCount = 1;

	return geometry;
}

static spall::AccelerationStructureCreateInfo bottomLevelInfo(
	std::span<const spall::AccelerationStructureGeometry> geometries)
{
	spall::AccelerationStructureCreateInfo info = {};
	info.Type = spall::AccelerationStructureType::BottomLevel;
	info.Geometries = geometries;

	return info;
}

static spall::AccelerationStructureCreateInfo topLevelInfo(
	spall::IBuffer& instanceBuffer,
	std::uint32_t instanceCount = 1)
{
	spall::AccelerationStructureCreateInfo info = {};
	info.Type = spall::AccelerationStructureType::TopLevel;
	info.InstanceBuffer = &instanceBuffer;
	info.InstanceCount = instanceCount;

	return info;
}

TEST_CASE(
	"A minimal unindexed bottom-level acceleration structure is accepted",
	"[acceleration][create]")
{
	FakeBuffer vertices = inputBuffer();
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	const spall::AccelerationStructureCreateInfo info = bottomLevelInfo(std::span {&geometry, 1});

	CHECK(spall::validateAccelerationStructureCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A minimal indexed bottom-level acceleration structure is accepted",
	"[acceleration][create]")
{
	FakeBuffer vertices = inputBuffer();
	FakeBuffer indices = inputBuffer();

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.VertexCount = 4;
	geometry.IndexBuffer = &indices;
	geometry.IndexFormat = spall::IndexFormat::UInt16;
	geometry.IndexCount = 6;

	const spall::AccelerationStructureCreateInfo info = bottomLevelInfo(std::span {&geometry, 1});

	CHECK(spall::validateAccelerationStructureCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A minimal top-level acceleration structure is accepted",
	"[acceleration][create]")
{
	FakeBuffer instances = inputBuffer();
	const spall::AccelerationStructureCreateInfo info = topLevelInfo(instances);

	CHECK(spall::validateAccelerationStructureCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"An acceleration structure rejects unknown build flags",
	"[acceleration][create]")
{
	FakeBuffer vertices = inputBuffer();
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);

	spall::AccelerationStructureCreateInfo info = bottomLevelInfo(std::span {&geometry, 1});
	info.Flags = static_cast<spall::AccelerationStructureBuildFlags>(1u << 20);

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"An acceleration structure rejects preferring both fast tracing and fast building",
	"[acceleration][create]")
{
	FakeBuffer vertices = inputBuffer();
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);

	spall::AccelerationStructureCreateInfo info = bottomLevelInfo(std::span {&geometry, 1});
	info.Flags = spall::AccelerationStructureBuildFlags::PreferFastTrace | spall::AccelerationStructureBuildFlags::PreferFastBuild;

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A bottom-level acceleration structure requires geometry",
	"[acceleration][create]")
{
	const spall::AccelerationStructureCreateInfo info = bottomLevelInfo({});

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A bottom-level acceleration structure rejects instances",
	"[acceleration][create]")
{
	FakeBuffer vertices = inputBuffer();
	FakeBuffer instances = inputBuffer();
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);

	spall::AccelerationStructureCreateInfo info = bottomLevelInfo(std::span {&geometry, 1});
	info.InstanceBuffer = &instances;

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A top-level acceleration structure rejects geometry",
	"[acceleration][create]")
{
	FakeBuffer vertices = inputBuffer();
	FakeBuffer instances = inputBuffer();
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);

	spall::AccelerationStructureCreateInfo info = topLevelInfo(instances);
	info.Geometries = std::span {&geometry, 1};

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A top-level acceleration structure requires an instance buffer",
	"[acceleration][create]")
{
	spall::AccelerationStructureCreateInfo info = {};
	info.Type = spall::AccelerationStructureType::TopLevel;
	info.InstanceCount = 1;

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A top-level acceleration structure requires an acceleration-structure-input instance buffer",
	"[acceleration][create]")
{
	FakeBuffer instances(spall::BufferInfo {.Size = 4096, .Usage = spall::BufferUsageFlags::Storage});
	const spall::AccelerationStructureCreateInfo info = topLevelInfo(instances);

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A top-level acceleration structure requires at least one instance",
	"[acceleration][create]")
{
	FakeBuffer instances = inputBuffer();
	const spall::AccelerationStructureCreateInfo info = topLevelInfo(instances, 0);

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A top-level acceleration structure rejects an unaligned instance offset",
	"[acceleration][create]")
{
	FakeBuffer instances = inputBuffer();

	spall::AccelerationStructureCreateInfo info = topLevelInfo(instances);
	info.InstanceBufferOffset = 8;

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A top-level acceleration structure rejects an instance range past the end of its buffer",
	"[acceleration][create]")
{
	FakeBuffer instances = inputBuffer(64);
	const spall::AccelerationStructureCreateInfo info = topLevelInfo(instances, 2);

	CHECK(spall::validateAccelerationStructureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A geometry requires a vertex buffer",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.VertexBuffer = nullptr;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A geometry requires an acceleration-structure-input vertex buffer",
	"[acceleration][geometry]")
{
	FakeBuffer vertices(spall::BufferInfo {.Size = 4096, .Usage = spall::BufferUsageFlags::Vertex});
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A geometry rejects a zero or misaligned vertex stride",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();
	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);

	geometry.VertexStride = 0;
	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);

	geometry.VertexStride = 10;
	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A geometry rejects a misaligned vertex offset",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.VertexOffset = 2;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A geometry requires at least three vertices",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.VertexCount = 2;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"An unindexed geometry requires whole triangles",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.VertexCount = 4;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A geometry rejects a vertex range past the end of its buffer",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer(16);
	const spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A geometry rejects an index count without an index buffer",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.IndexCount = 3;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"An indexed geometry requires whole triangles",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();
	FakeBuffer indices = inputBuffer();

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.IndexBuffer = &indices;
	geometry.IndexCount = 4;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"An indexed geometry rejects a misaligned index offset",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();
	FakeBuffer indices = inputBuffer();

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.IndexBuffer = &indices;
	geometry.IndexCount = 3;
	geometry.IndexOffset = 2;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"An indexed geometry rejects an index range past the end of its buffer",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();
	FakeBuffer indices = inputBuffer(8);

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.IndexBuffer = &indices;
	geometry.IndexCount = 3;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A geometry rejects a misaligned transform offset",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();
	FakeBuffer transforms = inputBuffer();

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.TransformBuffer = &transforms;
	geometry.TransformOffset = 8;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A geometry rejects a transform range past the end of its buffer",
	"[acceleration][geometry]")
{
	FakeBuffer vertices = inputBuffer();
	FakeBuffer transforms = inputBuffer(32);

	spall::AccelerationStructureGeometry geometry = triangleGeometry(vertices);
	geometry.TransformBuffer = &transforms;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"An update requires an acceleration structure created with update support",
	"[acceleration][build]")
{
	const spall::AccelerationStructureInfo info = {
		.Type = spall::AccelerationStructureType::BottomLevel,
		.Flags = spall::AccelerationStructureBuildFlags::PreferFastTrace,
		.Size = 1024,
		.BuildScratchSize = 512,
		.GeometryCount = 1};

	spall::AccelerationStructureBuildInfo buildInfo = {};
	buildInfo.Update = true;

	CHECK(spall::validateAccelerationStructureBuildInfo(info, buildInfo) != spall::SUCCESS);

	const spall::AccelerationStructureInfo updatable = {
		.Type = spall::AccelerationStructureType::BottomLevel,
		.Flags = spall::AccelerationStructureBuildFlags::AllowUpdate,
		.Size = 1024,
		.BuildScratchSize = 512,
		.GeometryCount = 1};

	CHECK(spall::validateAccelerationStructureBuildInfo(updatable, buildInfo) == spall::SUCCESS);
}

TEST_CASE(
	"A build instance count applies only to a top-level acceleration structure",
	"[acceleration][build]")
{
	const spall::AccelerationStructureInfo info = {
		.Type = spall::AccelerationStructureType::BottomLevel,
		.Flags = spall::AccelerationStructureBuildFlags::PreferFastTrace,
		.Size = 1024,
		.BuildScratchSize = 512,
		.GeometryCount = 1};

	spall::AccelerationStructureBuildInfo buildInfo = {};
	buildInfo.InstanceCount = 1;

	CHECK(spall::validateAccelerationStructureBuildInfo(info, buildInfo) != spall::SUCCESS);
}

TEST_CASE(
	"A build instance count cannot exceed the declared maximum",
	"[acceleration][build]")
{
	const spall::AccelerationStructureInfo info = {
		.Type = spall::AccelerationStructureType::TopLevel,
		.Flags = spall::AccelerationStructureBuildFlags::PreferFastTrace,
		.Size = 1024,
		.BuildScratchSize = 512,
		.InstanceCount = 4};

	spall::AccelerationStructureBuildInfo buildInfo = {};
	buildInfo.InstanceCount = 5;

	CHECK(spall::validateAccelerationStructureBuildInfo(info, buildInfo) != spall::SUCCESS);

	buildInfo.InstanceCount = 4;

	CHECK(spall::validateAccelerationStructureBuildInfo(info, buildInfo) == spall::SUCCESS);
}

TEST_CASE(
	"A default build of a top-level acceleration structure is accepted",
	"[acceleration][build]")
{
	const spall::AccelerationStructureInfo info = {
		.Type = spall::AccelerationStructureType::TopLevel,
		.Flags = spall::AccelerationStructureBuildFlags::PreferFastTrace,
		.Size = 1024,
		.BuildScratchSize = 512,
		.InstanceCount = 4};

	CHECK(spall::validateAccelerationStructureBuildInfo(info, {}) == spall::SUCCESS);
}

TEST_CASE(
	"Compaction requires its build flag",
	"[acceleration][build]")
{
	const spall::AccelerationStructureInfo without = {
		.Type = spall::AccelerationStructureType::BottomLevel,
		.Flags = spall::AccelerationStructureBuildFlags::PreferFastTrace,
		.Size = 1024,
		.BuildScratchSize = 512,
		.GeometryCount = 1};

	CHECK(spall::validateAccelerationStructureCompaction(without) == spall::ERR_INVALID_STATE);

	const spall::AccelerationStructureInfo with = {
		.Type = spall::AccelerationStructureType::BottomLevel,
		.Flags = spall::AccelerationStructureBuildFlags::AllowCompaction,
		.Size = 1024,
		.BuildScratchSize = 512,
		.GeometryCount = 1};

	CHECK(spall::validateAccelerationStructureCompaction(with) == spall::SUCCESS);
}

TEST_CASE(
	"A bounding-box geometry is accepted",
	"[acceleration][geometry]")
{
	FakeBuffer boxes = inputBuffer();

	CHECK(spall::validateAccelerationStructureGeometry(aabbGeometry(boxes)) == spall::SUCCESS);
}

TEST_CASE(
	"A bounding-box geometry ignores the triangle fields",
	"[acceleration][geometry]")
{
	FakeBuffer boxes = inputBuffer();

	spall::AccelerationStructureGeometry geometry = aabbGeometry(boxes);
	geometry.VertexFormat = spall::Format::RGBA8;
	geometry.VertexCount = 1;
	geometry.VertexStride = 3;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) == spall::SUCCESS);
}

TEST_CASE(
	"A bounding-box geometry requires a bounding-box buffer",
	"[acceleration][geometry]")
{
	FakeBuffer boxes = inputBuffer();

	spall::AccelerationStructureGeometry geometry = aabbGeometry(boxes);
	geometry.AabbBuffer = nullptr;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A bounding-box geometry requires an acceleration-structure-input buffer",
	"[acceleration][geometry]")
{
	FakeBuffer boxes(spall::BufferInfo {.Size = 4096, .Usage = spall::BufferUsageFlags::Storage});

	CHECK(spall::validateAccelerationStructureGeometry(aabbGeometry(boxes)) != spall::SUCCESS);
}

TEST_CASE(
	"A bounding-box geometry rejects a zero count",
	"[acceleration][geometry]")
{
	FakeBuffer boxes = inputBuffer();

	spall::AccelerationStructureGeometry geometry = aabbGeometry(boxes);
	geometry.AabbCount = 0;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A bounding-box geometry rejects a misaligned stride",
	"[acceleration][geometry]")
{
	FakeBuffer boxes = inputBuffer();

	spall::AccelerationStructureGeometry geometry = aabbGeometry(boxes);
	geometry.AabbStride = 28;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);

	geometry.AabbStride = 16;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);

	geometry.AabbStride = 32;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) == spall::SUCCESS);
}

TEST_CASE(
	"A bounding-box geometry rejects a misaligned offset",
	"[acceleration][geometry]")
{
	FakeBuffer boxes = inputBuffer();

	spall::AccelerationStructureGeometry geometry = aabbGeometry(boxes);
	geometry.AabbOffset = 4;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}

TEST_CASE(
	"A bounding-box geometry rejects a range past its buffer",
	"[acceleration][geometry]")
{
	FakeBuffer boxes = inputBuffer(48);

	spall::AccelerationStructureGeometry geometry = aabbGeometry(boxes);
	geometry.AabbCount = 2;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) == spall::SUCCESS);

	geometry.AabbCount = 3;

	CHECK(spall::validateAccelerationStructureGeometry(geometry) != spall::SUCCESS);
}
