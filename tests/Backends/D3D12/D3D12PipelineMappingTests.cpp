#include <catch2/catch_test_macros.hpp>

#include <src/Backends/D3D12/Common/Mappings/D3D12_PipelineMappings.h>

TEST_CASE(
	"D3D12 maps patch-list topology to the patch pipeline type",
	"[d3d12][pipeline][tessellation]")
{
	CHECK(spall::d3d12::primitiveTopologyType(spall::PrimitiveTopology::PatchList) == D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH);
	CHECK(spall::d3d12::primitiveTopologyType(spall::PrimitiveTopology::TriangleList) == D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
}

TEST_CASE(
	"D3D12 maps a patch-list to its control-point count",
	"[d3d12][pipeline][tessellation]")
{
	CHECK(spall::d3d12::primitiveTopology(spall::PrimitiveTopology::PatchList, 1) == D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST);
	CHECK(spall::d3d12::primitiveTopology(spall::PrimitiveTopology::PatchList, 3) == D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
	CHECK(spall::d3d12::primitiveTopology(spall::PrimitiveTopology::PatchList, 32) == D3D_PRIMITIVE_TOPOLOGY_32_CONTROL_POINT_PATCHLIST);
	CHECK(spall::d3d12::primitiveTopology(spall::PrimitiveTopology::TriangleList, 0) == D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
}
