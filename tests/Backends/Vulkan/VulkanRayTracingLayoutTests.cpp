#include <catch2/catch_test_macros.hpp>

#include <spall/Common/Limits.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureAabb.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureInstance.h>
#include <tests/Support/Fakes.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstring>

TEST_CASE(
	"The acceleration-structure instance matches its native layout",
	"[vulkan][acceleration][layout]")
{
	STATIC_REQUIRE(sizeof(spall::AccelerationStructureInstance) == sizeof(VkAccelerationStructureInstanceKHR));
	STATIC_REQUIRE(alignof(spall::AccelerationStructureInstance) <= alignof(VkAccelerationStructureInstanceKHR));

	STATIC_REQUIRE(offsetof(spall::AccelerationStructureInstance, Transform) == offsetof(VkAccelerationStructureInstanceKHR, transform));
	STATIC_REQUIRE(sizeof(spall::AccelerationStructureInstance::Transform) == sizeof(VkTransformMatrixKHR));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureInstance, InstanceIdAndMask) == sizeof(float) * 12);
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureInstance, InstanceContributionAndFlags) == (sizeof(float) * 12) + 4);
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureInstance, AccelerationStructure) == offsetof(VkAccelerationStructureInstanceKHR, accelerationStructureReference));

	STATIC_REQUIRE(spall::AccelerationStructureInstanceAlignment == 16);
}

TEST_CASE(
	"The acceleration-structure bounding box matches its native layout",
	"[vulkan][acceleration][layout]")
{
	STATIC_REQUIRE(sizeof(spall::AccelerationStructureAabb) == sizeof(VkAabbPositionsKHR));
	STATIC_REQUIRE(alignof(spall::AccelerationStructureAabb) <= alignof(VkAabbPositionsKHR));

	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MinX) == offsetof(VkAabbPositionsKHR, minX));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MinY) == offsetof(VkAabbPositionsKHR, minY));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MinZ) == offsetof(VkAabbPositionsKHR, minZ));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MaxX) == offsetof(VkAabbPositionsKHR, maxX));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MaxY) == offsetof(VkAabbPositionsKHR, maxY));
	STATIC_REQUIRE(offsetof(spall::AccelerationStructureAabb, MaxZ) == offsetof(VkAabbPositionsKHR, maxZ));

	STATIC_REQUIRE((sizeof(spall::AccelerationStructureAabb) % spall::AccelerationStructureAabbAlignment) == 0);
}

TEST_CASE(
	"A built instance packs the fields its native bitfields read back",
	"[vulkan][acceleration][layout]")
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

	VkAccelerationStructureInstanceKHR native = {};
	std::memcpy(&native, &instance, sizeof(native));

	CHECK(native.instanceCustomIndex == 0x123456u);
	CHECK(native.mask == 0x7Fu);
	CHECK(native.instanceShaderBindingTableRecordOffset == 0x0ABCDEu);
	CHECK(native.flags == static_cast<VkGeometryInstanceFlagsKHR>(VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR | VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR));
	CHECK(native.accelerationStructureReference == structure.deviceAddress());

	CHECK(native.transform.matrix[0][0] == 2.0f);
	CHECK(native.transform.matrix[0][3] == 7.0f);
	CHECK(native.transform.matrix[1][1] == 3.0f);
	CHECK(native.transform.matrix[2][3] == 9.0f);
}

TEST_CASE(
	"The portable instance flags match their native values",
	"[vulkan][acceleration][layout]")
{
	STATIC_REQUIRE(static_cast<VkGeometryInstanceFlagsKHR>(spall::AccelerationStructureInstanceFlags::TriangleCullDisable) ==
		VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR);
	STATIC_REQUIRE(static_cast<VkGeometryInstanceFlagsKHR>(spall::AccelerationStructureInstanceFlags::TriangleFlipFacing) ==
		VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR);
	STATIC_REQUIRE(static_cast<VkGeometryInstanceFlagsKHR>(spall::AccelerationStructureInstanceFlags::ForceOpaque) ==
		VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR);
	STATIC_REQUIRE(static_cast<VkGeometryInstanceFlagsKHR>(spall::AccelerationStructureInstanceFlags::ForceNonOpaque) ==
		VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR);
}
