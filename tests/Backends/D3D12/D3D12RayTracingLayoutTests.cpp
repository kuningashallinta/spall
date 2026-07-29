#include <catch2/catch_test_macros.hpp>

#include <spall/Common/Limits.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureAabb.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureInstance.h>
#include <tests/Support/Fakes.h>

#include <d3d12.h>

#include <cstddef>
#include <cstring>

TEST_CASE(
	"The acceleration-structure instance matches its native layout",
	"[d3d12][acceleration][layout]")
{
	STATIC_REQUIRE(sizeof(spall::AccelerationStructureInstance) == sizeof(D3D12_RAYTRACING_INSTANCE_DESC));
	STATIC_REQUIRE(alignof(spall::AccelerationStructureInstance) <= alignof(D3D12_RAYTRACING_INSTANCE_DESC));

	STATIC_REQUIRE(offsetof(spall::AccelerationStructureInstance, Transform) == offsetof(D3D12_RAYTRACING_INSTANCE_DESC, Transform));
	STATIC_REQUIRE(sizeof(spall::AccelerationStructureInstance::Transform) == sizeof(D3D12_RAYTRACING_INSTANCE_DESC::Transform));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureInstance, InstanceIdAndMask) == sizeof(float) * 12);
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureInstance, InstanceContributionAndFlags) == (sizeof(float) * 12) + 4);
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureInstance, AccelerationStructure) == offsetof(D3D12_RAYTRACING_INSTANCE_DESC, AccelerationStructure));

	STATIC_REQUIRE(spall::AccelerationStructureInstanceAlignment == D3D12_RAYTRACING_INSTANCE_DESCS_BYTE_ALIGNMENT);
	STATIC_REQUIRE(spall::MaxAccelerationStructureInstances == D3D12_RAYTRACING_MAX_INSTANCES_PER_TOP_LEVEL_ACCELERATION_STRUCTURE);
	STATIC_REQUIRE(spall::MaxAccelerationStructureGeometries == D3D12_RAYTRACING_MAX_GEOMETRIES_PER_BOTTOM_LEVEL_ACCELERATION_STRUCTURE);
}

TEST_CASE(
	"The acceleration-structure bounding box matches its native layout",
	"[d3d12][acceleration][layout]")
{
	STATIC_REQUIRE(sizeof(spall::AccelerationStructureAabb) == sizeof(D3D12_RAYTRACING_AABB));
	STATIC_REQUIRE(alignof(spall::AccelerationStructureAabb) <= alignof(D3D12_RAYTRACING_AABB));

	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MinX) == offsetof(D3D12_RAYTRACING_AABB, MinX));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MinY) == offsetof(D3D12_RAYTRACING_AABB, MinY));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MinZ) == offsetof(D3D12_RAYTRACING_AABB, MinZ));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MaxX) == offsetof(D3D12_RAYTRACING_AABB, MaxX));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MaxY) == offsetof(D3D12_RAYTRACING_AABB, MaxY));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MaxZ) == offsetof(D3D12_RAYTRACING_AABB, MaxZ));

	STATIC_REQUIRE(spall::AccelerationStructureAabbAlignment == D3D12_RAYTRACING_AABB_BYTE_ALIGNMENT);
}

TEST_CASE(
	"A built instance packs the fields its native bitfields read back",
	"[d3d12][acceleration][layout]")
{
	spall::tests::FakeAccelerationStructure structure(
		spall::tests::accelerationStructureInfo(spall::AccelerationStructureType::BottomLevel));

	const float transform[12] = {
		2.0f, 0.0f, 0.0f, 7.0f,
		0.0f, 3.0f, 0.0f, 8.0f,
		0.0f, 0.0f, 4.0f, 9.0f};

	const spall::AccelerationStructureInstance instance = spall::makeAccelerationStructureInstance(
		structure,
		transform,
		0x123456,
		0x7F,
		spall::AccelerationStructureInstanceFlags::TriangleCullDisable | spall::AccelerationStructureInstanceFlags::ForceOpaque,
		0x0ABCDE);

	D3D12_RAYTRACING_INSTANCE_DESC native = {};
	std::memcpy(&native, &instance, sizeof(native));

	CHECK(native.InstanceID == 0x123456u);
	CHECK(native.InstanceMask == 0x7Fu);
	CHECK(native.InstanceContributionToHitGroupIndex == 0x0ABCDEu);
	CHECK(native.Flags == static_cast<UINT>(D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE | D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE));
	CHECK(native.AccelerationStructure == structure.deviceAddress());

	CHECK(native.Transform[0][0] == 2.0f);
	CHECK(native.Transform[0][3] == 7.0f);
	CHECK(native.Transform[1][1] == 3.0f);
	CHECK(native.Transform[2][3] == 9.0f);
}

TEST_CASE(
	"The portable instance flags match their native values",
	"[d3d12][acceleration][layout]")
{
	STATIC_REQUIRE(static_cast<UINT>(spall::AccelerationStructureInstanceFlags::TriangleCullDisable) ==
		D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE);

	STATIC_REQUIRE(static_cast<UINT>(spall::AccelerationStructureInstanceFlags::TriangleFlipFacing) ==
		D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE);

	STATIC_REQUIRE(static_cast<UINT>(spall::AccelerationStructureInstanceFlags::ForceOpaque) ==
		D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE);

	STATIC_REQUIRE(static_cast<UINT>(spall::AccelerationStructureInstanceFlags::ForceNonOpaque) ==
		D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE);
}
