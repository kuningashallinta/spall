#include <catch2/catch_test_macros.hpp>

#include <spall/CommandList/ICommandList.h>

#include <concepts>
#include <span>

TEST_CASE(
	"Draw methods support default and explicit instance parameters",
	"[commandlist][instancing]")
{
	STATIC_REQUIRE(requires(spall::ICommandList& commandList) {
		{ commandList.draw(3, 0) } -> std::same_as<spall::Status>;
		{ commandList.draw(3, 0, 4, 2) } -> std::same_as<spall::Status>;
		{ commandList.drawIndexed(6, 0, 0) } -> std::same_as<spall::Status>;
		{ commandList.drawIndexed(6, 0, 0, 4, 2) } -> std::same_as<spall::Status>;
	});
}

TEST_CASE(
	"Command lists expose debug groups and markers",
	"[commandlist][debugmarker]")
{
	STATIC_REQUIRE(requires(spall::ICommandList& commandList) {
		{ commandList.pushDebugGroup("Frame") } -> std::same_as<spall::Status>;
		{ commandList.pushDebugGroup("Shadow pass", spall::Color {1.0f, 0.5f, 0.25f, 1.0f}) } -> std::same_as<spall::Status>;
		{ commandList.popDebugGroup() } -> std::same_as<spall::Status>;
		{ commandList.insertDebugMarker("Upload") } -> std::same_as<spall::Status>;
		{ commandList.insertDebugMarker("Draw", spall::Color {0.0f, 1.0f, 0.0f, 1.0f}) } -> std::same_as<spall::Status>;
	});
}

TEST_CASE(
	"Command lists expose a dynamic stencil reference",
	"[commandlist][stencil]")
{
	STATIC_REQUIRE(requires(spall::ICommandList& commandList) {
		{ commandList.setStencilReference(42) } -> std::same_as<spall::Status>;
	});
}

TEST_CASE(
	"Command lists expose texture readback copies",
	"[commandlist][copy]")
{
	STATIC_REQUIRE(requires(
		spall::ICommandList& commandList,
		spall::IBuffer& buffer,
		spall::ITexture& texture,
		const spall::TextureRegion& region) {
		{ commandList.copyTextureToBuffer(buffer, 16, 256, texture, region) } -> std::same_as<spall::Status>;
	});
}

TEST_CASE(
	"Command lists expose automatic mipmap generation",
	"[commandlist][mips]")
{
	STATIC_REQUIRE(requires(
		spall::ICommandList& commandList,
		spall::ITexture& texture) {
		{ commandList.generateMips(texture) } -> std::same_as<spall::Status>;
	});
}

TEST_CASE(
	"Command lists expose push-constant updates",
	"[commandlist][pushconstants]")
{
	STATIC_REQUIRE(requires(
		spall::ICommandList& commandList,
		std::span<const std::byte> data) {
		{ commandList.setPushConstants(spall::ShaderStageFlags::Vertex, 0, data) } -> std::same_as<spall::Status>;
	});
}
