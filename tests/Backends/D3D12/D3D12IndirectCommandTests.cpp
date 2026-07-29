#include <catch2/catch_test_macros.hpp>

#include <spall/CommandList/IndirectCommands.h>

#include <d3d12.h>

#include <cstddef>

TEST_CASE(
	"D3D12 indirect command structs match their native layout",
	"[d3d12][commandlist][indirect]")
{
	STATIC_REQUIRE(sizeof(spall::DrawIndirectCommand) == sizeof(D3D12_DRAW_ARGUMENTS));
	STATIC_REQUIRE(sizeof(spall::DrawIndexedIndirectCommand) == sizeof(D3D12_DRAW_INDEXED_ARGUMENTS));
	STATIC_REQUIRE(sizeof(spall::DispatchIndirectCommand) == sizeof(D3D12_DISPATCH_ARGUMENTS));
	STATIC_REQUIRE(offsetof(spall::DrawIndirectCommand, StartInstance) == offsetof(D3D12_DRAW_ARGUMENTS, StartInstanceLocation));
	STATIC_REQUIRE(offsetof(spall::DrawIndexedIndirectCommand, VertexOffset) == offsetof(D3D12_DRAW_INDEXED_ARGUMENTS, BaseVertexLocation));
	STATIC_REQUIRE(offsetof(spall::DrawIndexedIndirectCommand, StartInstance) == offsetof(D3D12_DRAW_INDEXED_ARGUMENTS, StartInstanceLocation));
}
