#include <catch2/catch_test_macros.hpp>

#include <src/Backends/Vulkan/Common/VK_PipelineMappings.h>

TEST_CASE(
	"Vulkan maps every fill mode",
	"[vulkan][pipeline][rasterizer]")
{
	CHECK(spall::vk::vulkanPolygonMode(spall::FillMode::Solid) == VK_POLYGON_MODE_FILL);
	CHECK(spall::vk::vulkanPolygonMode(spall::FillMode::Wireframe) == VK_POLYGON_MODE_LINE);
}

TEST_CASE(
	"Vulkan maps an attachment blend state",
	"[vulkan][pipeline][blend]")
{
	spall::BlendStateInfo blendState = {};
	blendState.EnableBlend = true;
	blendState.SourceColorFactor = spall::BlendFactor::SourceAlpha;
	blendState.DestinationColorFactor = spall::BlendFactor::OneMinusSourceAlpha;
	blendState.ColorBlendOp = spall::BlendOp::ReverseSubtract;
	blendState.SourceAlphaFactor = spall::BlendFactor::One;
	blendState.DestinationAlphaFactor = spall::BlendFactor::Zero;
	blendState.AlphaBlendOp = spall::BlendOp::Max;
	blendState.ColorWriteMask = spall::ColorComponentFlags::Red | spall::ColorComponentFlags::Alpha;

	const VkPipelineColorBlendAttachmentState state = spall::vk::vulkanColorBlendAttachmentState(blendState);

	CHECK(state.blendEnable == VK_TRUE);
	CHECK(state.srcColorBlendFactor == VK_BLEND_FACTOR_SRC_ALPHA);
	CHECK(state.dstColorBlendFactor == VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA);
	CHECK(state.colorBlendOp == VK_BLEND_OP_REVERSE_SUBTRACT);
	CHECK(state.srcAlphaBlendFactor == VK_BLEND_FACTOR_ONE);
	CHECK(state.dstAlphaBlendFactor == VK_BLEND_FACTOR_ZERO);
	CHECK(state.alphaBlendOp == VK_BLEND_OP_MAX);
	CHECK(state.colorWriteMask == (VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_A_BIT));
}

TEST_CASE(
	"Vulkan maps every stencil operation",
	"[vulkan][pipeline][stencil]")
{
	CHECK(spall::vk::vulkanStencilOp(spall::StencilOp::Keep) == VK_STENCIL_OP_KEEP);
	CHECK(spall::vk::vulkanStencilOp(spall::StencilOp::Zero) == VK_STENCIL_OP_ZERO);
	CHECK(spall::vk::vulkanStencilOp(spall::StencilOp::Replace) == VK_STENCIL_OP_REPLACE);
	CHECK(spall::vk::vulkanStencilOp(spall::StencilOp::IncrementClamp) == VK_STENCIL_OP_INCREMENT_AND_CLAMP);
	CHECK(spall::vk::vulkanStencilOp(spall::StencilOp::DecrementClamp) == VK_STENCIL_OP_DECREMENT_AND_CLAMP);
	CHECK(spall::vk::vulkanStencilOp(spall::StencilOp::Invert) == VK_STENCIL_OP_INVERT);
	CHECK(spall::vk::vulkanStencilOp(spall::StencilOp::IncrementWrap) == VK_STENCIL_OP_INCREMENT_AND_WRAP);
	CHECK(spall::vk::vulkanStencilOp(spall::StencilOp::DecrementWrap) == VK_STENCIL_OP_DECREMENT_AND_WRAP);
}
