#include <catch2/catch_test_macros.hpp>

#include <spall/CommandList/IndirectCommands.h>

#include <vulkan/vulkan.h>

#include <cstddef>

TEST_CASE(
	"Vulkan indirect command structs match their native layout",
	"[vulkan][commandlist][indirect]")
{
	STATIC_REQUIRE(sizeof(spall::DrawIndirectCommand) == sizeof(VkDrawIndirectCommand));
	STATIC_REQUIRE(sizeof(spall::DrawIndexedIndirectCommand) == sizeof(VkDrawIndexedIndirectCommand));
	STATIC_REQUIRE(sizeof(spall::DispatchIndirectCommand) == sizeof(VkDispatchIndirectCommand));
	STATIC_REQUIRE(offsetof(spall::DrawIndirectCommand, StartInstance) == offsetof(VkDrawIndirectCommand, firstInstance));
	STATIC_REQUIRE(offsetof(spall::DrawIndexedIndirectCommand, VertexOffset) == offsetof(VkDrawIndexedIndirectCommand, vertexOffset));
	STATIC_REQUIRE(offsetof(spall::DrawIndexedIndirectCommand, StartInstance) == offsetof(VkDrawIndexedIndirectCommand, firstInstance));
}
