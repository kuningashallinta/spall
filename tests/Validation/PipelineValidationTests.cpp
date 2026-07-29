#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/PipelineValidation.h>
#include <tests/Support/Fakes.h>

#include <array>
#include <limits>
#include <span>

namespace
{
	using spall::tests::FakeResourceSetLayout;
	using spall::tests::FakeShader;

	FakeShader VertexShader;
	FakeShader FragmentShader;

	spall::PipelineCreateInfo validPipeline()
	{
		spall::PipelineCreateInfo info = {};
		info.VertexShader = spall::PipelineShaderStageInfo {&VertexShader, "vsMain"};
		info.FragmentShader = spall::PipelineShaderStageInfo {&FragmentShader, "psMain"};
		info.PrimitiveTopology = spall::PrimitiveTopology::TriangleStrip;
		info.CullMode = spall::CullMode::None;
		info.FrontFace = spall::FrontFace::Clockwise;
		info.ColorTargetFormats[0] = spall::Format::BGRA8;
		info.ColorTargetFormatCount = 1;

		return info;
	}
} // namespace

TEST_CASE(
	"A minimal graphics pipeline is valid",
	"[pipeline]")
{
	CHECK(spall::validatePipelineCreateInfo(validPipeline()) == spall::SUCCESS);
}

TEST_CASE(
	"A pipeline accepts an optional geometry shader",
	"[pipeline][stages]")
{
	spall::tests::FakeShader geometry;
	spall::PipelineCreateInfo info = validPipeline();
	info.GeometryShader = spall::PipelineShaderStageInfo {&geometry, "gsMain"};

	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"Tessellation requires both a control and an evaluation shader",
	"[pipeline][stages][tessellation]")
{
	spall::tests::FakeShader control;
	spall::tests::FakeShader evaluation;

	spall::PipelineCreateInfo onlyControl = validPipeline();
	onlyControl.PrimitiveTopology = spall::PrimitiveTopology::PatchList;
	onlyControl.PatchControlPoints = 3;
	onlyControl.TessellationControlShader = spall::PipelineShaderStageInfo {&control, "hsMain"};
	CHECK(spall::validatePipelineCreateInfo(onlyControl) == spall::ERR_INVALID_SHADER_STAGE);

	spall::PipelineCreateInfo both = validPipeline();
	both.PrimitiveTopology = spall::PrimitiveTopology::PatchList;
	both.PatchControlPoints = 3;
	both.TessellationControlShader = spall::PipelineShaderStageInfo {&control, "hsMain"};
	both.TessellationEvaluationShader = spall::PipelineShaderStageInfo {&evaluation, "dsMain"};
	CHECK(spall::validatePipelineCreateInfo(both) == spall::SUCCESS);
}

TEST_CASE(
	"Tessellation and patch-list topology require each other",
	"[pipeline][stages][tessellation]")
{
	spall::tests::FakeShader control;
	spall::tests::FakeShader evaluation;

	spall::PipelineCreateInfo tessWithoutPatch = validPipeline();
	tessWithoutPatch.PatchControlPoints = 3;
	tessWithoutPatch.TessellationControlShader = spall::PipelineShaderStageInfo {&control, "hsMain"};
	tessWithoutPatch.TessellationEvaluationShader = spall::PipelineShaderStageInfo {&evaluation, "dsMain"};
	CHECK(spall::validatePipelineCreateInfo(tessWithoutPatch) == spall::ERR_INVALID_SHADER_STAGE);

	spall::PipelineCreateInfo patchWithoutTess = validPipeline();
	patchWithoutTess.PrimitiveTopology = spall::PrimitiveTopology::PatchList;
	patchWithoutTess.PatchControlPoints = 3;
	CHECK(spall::validatePipelineCreateInfo(patchWithoutTess) == spall::ERR_INVALID_SHADER_STAGE);
}

TEST_CASE(
	"Tessellation rejects an out-of-range patch control-point count",
	"[pipeline][stages][tessellation]")
{
	spall::tests::FakeShader control;
	spall::tests::FakeShader evaluation;

	spall::PipelineCreateInfo info = validPipeline();
	info.PrimitiveTopology = spall::PrimitiveTopology::PatchList;
	info.TessellationControlShader = spall::PipelineShaderStageInfo {&control, "hsMain"};
	info.TessellationEvaluationShader = spall::PipelineShaderStageInfo {&evaluation, "dsMain"};

	info.PatchControlPoints = 0;
	CHECK(spall::validatePipelineCreateInfo(info) == spall::ERR_INVALID_SIZE);

	info.PatchControlPoints = 33;
	CHECK(spall::validatePipelineCreateInfo(info) == spall::ERR_INVALID_SIZE);

	info.PatchControlPoints = 32;
	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects an invalid primitive topology",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.PrimitiveTopology = static_cast<spall::PrimitiveTopology>(99);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects an invalid cull mode and front face",
	"[pipeline]")
{
	spall::PipelineCreateInfo badCull = validPipeline();
	badCull.CullMode = static_cast<spall::CullMode>(99);
	CHECK(spall::validatePipelineCreateInfo(badCull) != spall::SUCCESS);

	spall::PipelineCreateInfo badFace = validPipeline();
	badFace.FrontFace = static_cast<spall::FrontFace>(99);
	CHECK(spall::validatePipelineCreateInfo(badFace) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline validates rasterizer fill mode",
	"[pipeline][rasterizer]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.FillMode = spall::FillMode::Wireframe;
	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);

	info.FillMode = static_cast<spall::FillMode>(99);
	CHECK(spall::validatePipelineCreateInfo(info) == spall::ERR_INVALID_ARGUMENT);
}

TEST_CASE(
	"A pipeline validates finite depth bias values",
	"[pipeline][rasterizer]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.DepthBias = -8;
	info.DepthBiasClamp = 1.5f;
	info.SlopeScaledDepthBias = -2.0f;
	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);

	info.DepthBiasClamp = std::numeric_limits<float>::infinity();
	CHECK(spall::validatePipelineCreateInfo(info) == spall::ERR_INVALID_RANGE);

	info.DepthBiasClamp = 0.0f;
	info.SlopeScaledDepthBias = std::numeric_limits<float>::quiet_NaN();
	CHECK(spall::validatePipelineCreateInfo(info) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"A pipeline requires a finite positive line width",
	"[pipeline][rasterizer]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.LineWidth = 2.0f;
	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);

	info.LineWidth = 0.0f;
	CHECK(spall::validatePipelineCreateInfo(info) == spall::ERR_INVALID_RANGE);

	info.LineWidth = std::numeric_limits<float>::quiet_NaN();
	CHECK(spall::validatePipelineCreateInfo(info) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"A pipeline requires a vertex shader",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.VertexShader.Module = nullptr;

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline with color targets requires a fragment shader",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.FragmentShader.Module = nullptr;

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A depth-only pipeline needs no fragment shader",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.FragmentShader.Module = nullptr;
	info.ColorTargetFormatCount = 0;
	info.ColorTargetFormats[0] = spall::Format::Unknown;
	info.DepthStencilFormat = spall::Format::Depth32Float;

	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A pipeline requires at least one target",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.ColorTargetFormatCount = 0;
	info.ColorTargetFormats[0] = spall::Format::Unknown;

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects too many color targets",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.ColorTargetFormatCount = spall::MaxColorAttachments + 1;

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline color target must be a color format",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.ColorTargetFormats[0] = spall::Format::Depth32Float;

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects a block-compressed color target",
	"[pipeline][compressed]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.ColorTargetFormats[0] = spall::Format::BC7RGBAUnorm;

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects a color format as its depth-stencil target",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.DepthStencilFormat = spall::Format::RGBA8;

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Depth testing requires a depth-stencil format",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.EnableDepthTest = true;

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Depth writing requires depth testing",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.DepthStencilFormat = spall::Format::Depth32Float;
	info.EnableDepthWrite = true;
	info.EnableDepthTest = false;

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Depth testing and writing together are valid with a depth format",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.DepthStencilFormat = spall::Format::Depth32Float;
	info.EnableDepthTest = true;
	info.EnableDepthWrite = true;

	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects an invalid depth compare operation",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.DepthCompareOp = static_cast<spall::CompareOp>(99);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Stencil testing requires a stencil-capable format",
	"[pipeline][stencil]")
{
	spall::PipelineCreateInfo missingFormat = validPipeline();
	missingFormat.EnableStencilTest = true;

	spall::PipelineCreateInfo depthOnly = validPipeline();
	depthOnly.DepthStencilFormat = spall::Format::Depth32Float;
	depthOnly.EnableStencilTest = true;

	spall::PipelineCreateInfo combined = validPipeline();
	combined.DepthStencilFormat = spall::Format::Depth32FloatStencil8;
	combined.EnableStencilTest = true;

	CHECK(spall::validatePipelineCreateInfo(missingFormat) == spall::ERR_INVALID_FORMAT);
	CHECK(spall::validatePipelineCreateInfo(depthOnly) == spall::ERR_INVALID_FORMAT);
	CHECK(spall::validatePipelineCreateInfo(combined) == spall::SUCCESS);
}

TEST_CASE(
	"Stencil state defaults to a disabled no-op",
	"[pipeline][stencil]")
{
	const spall::PipelineCreateInfo info = {};

	CHECK_FALSE(info.EnableStencilTest);
	CHECK(info.FrontStencilState.FailOp == spall::StencilOp::Keep);
	CHECK(info.FrontStencilState.DepthFailOp == spall::StencilOp::Keep);
	CHECK(info.FrontStencilState.PassOp == spall::StencilOp::Keep);
	CHECK(info.FrontStencilState.Compare == spall::CompareOp::Always);
	CHECK(info.BackStencilState.FailOp == spall::StencilOp::Keep);
	CHECK(info.BackStencilState.DepthFailOp == spall::StencilOp::Keep);
	CHECK(info.BackStencilState.PassOp == spall::StencilOp::Keep);
	CHECK(info.BackStencilState.Compare == spall::CompareOp::Always);
	CHECK(info.StencilReadMask == 0xff);
	CHECK(info.StencilWriteMask == 0xff);
	CHECK(info.StencilReference == 0);
}

TEST_CASE(
	"A pipeline accepts separate front and back stencil states",
	"[pipeline][stencil]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.DepthStencilFormat = spall::Format::Depth24Stencil8;
	info.EnableStencilTest = true;
	info.FrontStencilState.FailOp = spall::StencilOp::Replace;
	info.FrontStencilState.DepthFailOp = spall::StencilOp::IncrementClamp;
	info.FrontStencilState.PassOp = spall::StencilOp::IncrementWrap;
	info.FrontStencilState.Compare = spall::CompareOp::Equal;
	info.BackStencilState.FailOp = spall::StencilOp::Zero;
	info.BackStencilState.DepthFailOp = spall::StencilOp::DecrementClamp;
	info.BackStencilState.PassOp = spall::StencilOp::DecrementWrap;
	info.BackStencilState.Compare = spall::CompareOp::NotEqual;
	info.StencilReadMask = 0x7f;
	info.StencilWriteMask = 0x3f;
	info.StencilReference = 42;

	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects invalid front and back stencil comparisons",
	"[pipeline][stencil]")
{
	spall::PipelineCreateInfo invalidFront = validPipeline();
	invalidFront.FrontStencilState.Compare = static_cast<spall::CompareOp>(99);

	spall::PipelineCreateInfo invalidBack = validPipeline();
	invalidBack.BackStencilState.Compare = static_cast<spall::CompareOp>(99);

	CHECK(spall::validatePipelineCreateInfo(invalidFront) == spall::ERR_INVALID_ARGUMENT);
	CHECK(spall::validatePipelineCreateInfo(invalidBack) == spall::ERR_INVALID_ARGUMENT);
}

TEST_CASE(
	"A pipeline rejects invalid stencil operations",
	"[pipeline][stencil]")
{
	spall::PipelineCreateInfo invalidFail = validPipeline();
	invalidFail.FrontStencilState.FailOp = static_cast<spall::StencilOp>(99);

	spall::PipelineCreateInfo invalidDepthFail = validPipeline();
	invalidDepthFail.BackStencilState.DepthFailOp = static_cast<spall::StencilOp>(99);

	spall::PipelineCreateInfo invalidPass = validPipeline();
	invalidPass.FrontStencilState.PassOp = static_cast<spall::StencilOp>(99);

	CHECK(spall::validatePipelineCreateInfo(invalidFail) == spall::ERR_INVALID_ARGUMENT);
	CHECK(spall::validatePipelineCreateInfo(invalidDepthFail) == spall::ERR_INVALID_ARGUMENT);
	CHECK(spall::validatePipelineCreateInfo(invalidPass) == spall::ERR_INVALID_ARGUMENT);
}

TEST_CASE(
	"A pipeline rejects invalid blend factors and operations",
	"[pipeline]")
{
	spall::PipelineCreateInfo badFactor = validPipeline();
	badFactor.BlendStates[0].DestinationAlphaFactor = static_cast<spall::BlendFactor>(99);
	CHECK(spall::validatePipelineCreateInfo(badFactor) != spall::SUCCESS);

	spall::PipelineCreateInfo badOp = validPipeline();
	badOp.BlendStates[0].ColorBlendOp = static_cast<spall::BlendOp>(99);
	CHECK(spall::validatePipelineCreateInfo(badOp) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline validates every active attachment blend state",
	"[pipeline][blend]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.ColorTargetFormats[1] = spall::Format::RGBA8;
	info.ColorTargetFormatCount = 2;
	info.BlendStates[0].EnableBlend = true;
	info.BlendStates[0].DestinationColorFactor = spall::BlendFactor::OneMinusSourceAlpha;
	info.BlendStates[1].ColorWriteMask = spall::ColorComponentFlags::Red;
	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);

	info.BlendStates[1].AlphaBlendOp = static_cast<spall::BlendOp>(99);
	CHECK(spall::validatePipelineCreateInfo(info) == spall::ERR_INVALID_ARGUMENT);
}

TEST_CASE(
	"A pipeline rejects an unknown color write mask",
	"[pipeline]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.BlendStates[0].ColorWriteMask = static_cast<spall::ColorComponentFlags>(1u << 20);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects more resource-set layouts than the limit",
	"[pipeline]")
{
	const spall::IResourceSetLayout* layouts[spall::MaxResourceSets + 1] = {};

	spall::PipelineCreateInfo info = validPipeline();
	info.ResourceSetLayouts = std::span(layouts);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects a null resource-set layout",
	"[pipeline]")
{
	FakeResourceSetLayout realLayout;
	const spall::IResourceSetLayout* layouts[] = {&realLayout, nullptr};

	spall::PipelineCreateInfo info = validPipeline();
	info.ResourceSetLayouts = std::span(layouts);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Vertex bindings and attributes must both be present or both absent",
	"[pipeline]")
{
	const spall::VertexBindingInfo bindings[] = {{0, 16}};

	spall::PipelineCreateInfo info = validPipeline();
	info.VertexBindings = std::span(bindings);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects a zero vertex stride",
	"[pipeline]")
{
	const spall::VertexBindingInfo bindings[] = {{0, 0}};
	const spall::VertexAttributeInfo attributes[] = {{0, 0, spall::Format::RG32Float, 0}};

	spall::PipelineCreateInfo info = validPipeline();
	info.VertexBindings = std::span(bindings);
	info.VertexAttributes = std::span(attributes);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects duplicate vertex bindings",
	"[pipeline]")
{
	const spall::VertexBindingInfo bindings[] = {{0, 16}, {0, 16}};
	const spall::VertexAttributeInfo attributes[] = {{0, 0, spall::Format::RG32Float, 0}};

	spall::PipelineCreateInfo info = validPipeline();
	info.VertexBindings = std::span(bindings);
	info.VertexAttributes = std::span(attributes);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A vertex attribute must reference an existing binding",
	"[pipeline]")
{
	const spall::VertexBindingInfo bindings[] = {{0, 16}};
	const spall::VertexAttributeInfo attributes[] = {{0, 7, spall::Format::RG32Float, 0}};

	spall::PipelineCreateInfo info = validPipeline();
	info.VertexBindings = std::span(bindings);
	info.VertexAttributes = std::span(attributes);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A vertex attribute must fit within its binding stride",
	"[pipeline]")
{
	const spall::VertexBindingInfo bindings[] = {{0, 8}};
	const spall::VertexAttributeInfo overrun[] = {{0, 0, spall::Format::RG32Float, 4}};
	const spall::VertexAttributeInfo fits[] = {{0, 0, spall::Format::RG32Float, 0}};

	spall::PipelineCreateInfo info = validPipeline();
	info.VertexBindings = std::span(bindings);

	info.VertexAttributes = std::span(overrun);
	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);

	info.VertexAttributes = std::span(fits);
	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects a non-vertex attribute format",
	"[pipeline]")
{
	const spall::VertexBindingInfo bindings[] = {{0, 16}};
	const spall::VertexAttributeInfo attributes[] = {{0, 0, spall::Format::Depth32Float, 0}};

	spall::PipelineCreateInfo info = validPipeline();
	info.VertexBindings = std::span(bindings);
	info.VertexAttributes = std::span(attributes);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A pipeline rejects duplicate vertex-attribute locations",
	"[pipeline]")
{
	const spall::VertexBindingInfo bindings[] = {{0, 32}};
	const spall::VertexAttributeInfo attributes[] = {
		{0, 0, spall::Format::RG32Float, 0},
		{0, 0, spall::Format::RG32Float, 8}};

	spall::PipelineCreateInfo info = validPipeline();
	info.VertexBindings = std::span(bindings);
	info.VertexAttributes = std::span(attributes);

	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A well-formed vertex layout is accepted",
	"[pipeline]")
{
	const spall::VertexBindingInfo bindings[] = {{0, 24}};
	const spall::VertexAttributeInfo attributes[] = {
		{0, 0, spall::Format::RG32Float, 0},
		{1, 0, spall::Format::RGBA32Float, 8}};

	spall::PipelineCreateInfo info = validPipeline();
	info.VertexBindings = std::span(bindings);
	info.VertexAttributes = std::span(attributes);

	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A compute pipeline requires a compute shader",
	"[pipeline][compute]")
{
	spall::ComputePipelineCreateInfo info = {};

	CHECK(spall::validateComputePipelineCreateInfo(info) != spall::SUCCESS);

	FakeShader computeShader;
	info.ComputeShader = spall::PipelineShaderStageInfo {&computeShader, "csMain"};

	CHECK(spall::validateComputePipelineCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A compute pipeline rejects too many resource-set layouts",
	"[pipeline][compute]")
{
	FakeShader computeShader;
	const spall::IResourceSetLayout* layouts[spall::MaxResourceSets + 1] = {};

	spall::ComputePipelineCreateInfo info = {};
	info.ComputeShader = spall::PipelineShaderStageInfo {&computeShader, "csMain"};
	info.ResourceSetLayouts = std::span(layouts);

	CHECK(spall::validateComputePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Graphics push constants require aligned size and graphics stages",
	"[pipeline][pushconstants]")
{
	spall::PipelineCreateInfo info = validPipeline();
	info.PushConstants = {spall::ShaderStageFlags::Vertex | spall::ShaderStageFlags::Fragment, 128};
	CHECK(spall::validatePipelineCreateInfo(info) == spall::SUCCESS);

	info.PushConstants.Size = 129;
	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);

	info.PushConstants = {spall::ShaderStageFlags::Compute, 16};
	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);

	info.PushConstants = {spall::ShaderStageFlags::None, 16};
	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);

	info = validPipeline();
	info.FragmentShader = {};
	info.ColorTargetFormatCount = 0;
	info.DepthStencilFormat = spall::Format::Depth32Float;
	info.PushConstants = {spall::ShaderStageFlags::Fragment, 16};
	CHECK(spall::validatePipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Compute push constants require the compute stage",
	"[pipeline][compute][pushconstants]")
{
	FakeShader computeShader;
	spall::ComputePipelineCreateInfo info = {};
	info.ComputeShader = spall::PipelineShaderStageInfo {&computeShader, "csMain"};
	info.PushConstants = {spall::ShaderStageFlags::Compute, 32};
	CHECK(spall::validateComputePipelineCreateInfo(info) == spall::SUCCESS);

	info.PushConstants.Stages = spall::ShaderStageFlags::Vertex;
	CHECK(spall::validateComputePipelineCreateInfo(info) != spall::SUCCESS);
}

namespace
{
	FakeShader RayGenShader;
	FakeShader MissShader;
	FakeShader ClosestHitShader;

	spall::RayTracingPipelineCreateInfo validRayTracingPipeline(
		std::span<const spall::PipelineShaderStageInfo> missShaders,
		std::span<const spall::RayTracingHitGroup> hitGroups)
	{
		spall::RayTracingPipelineCreateInfo info = {};
		info.RayGenerationShader = spall::PipelineShaderStageInfo {&RayGenShader, "rayGenMain"};
		info.MissShaders = missShaders;
		info.HitGroups = hitGroups;
		info.MaxPayloadSize = 16;

		return info;
	}
} // namespace

TEST_CASE(
	"A minimal ray-tracing pipeline is valid",
	"[pipeline][raytracing]")
{
	const spall::PipelineShaderStageInfo missShaders[] = {{&MissShader, "missMain"}};
	const spall::RayTracingHitGroup hitGroups[] = {{{&ClosestHitShader, "closestHitMain"}, {}, {}}};

	CHECK(spall::validateRayTracingPipelineCreateInfo(validRayTracingPipeline(missShaders, hitGroups)) == spall::SUCCESS);
}

TEST_CASE(
	"A ray-tracing pipeline requires a ray-generation shader",
	"[pipeline][raytracing]")
{
	spall::RayTracingPipelineCreateInfo info = validRayTracingPipeline({}, {});
	info.RayGenerationShader = {};

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A ray-tracing stage requires an explicit entry point",
	"[pipeline][raytracing]")
{
	spall::RayTracingPipelineCreateInfo info = validRayTracingPipeline({}, {});
	info.RayGenerationShader.Entry = nullptr;

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);

	info.RayGenerationShader.Entry = "";

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A hit group requires at least one shader",
	"[pipeline][raytracing]")
{
	const spall::RayTracingHitGroup hitGroups[] = {{}};
	const spall::RayTracingPipelineCreateInfo info = validRayTracingPipeline({}, hitGroups);

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A triangle hit group requires the built-in attribute size",
	"[pipeline][raytracing]")
{
	const spall::RayTracingHitGroup hitGroups[] = {{{&ClosestHitShader, "closestHitMain"}, {}, {}}};

	spall::RayTracingPipelineCreateInfo info = validRayTracingPipeline({}, hitGroups);
	info.MaxAttributeSize = 4;

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);

	info.MaxAttributeSize = 8;

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A ray-tracing pipeline bounds its declared sizes",
	"[pipeline][raytracing]")
{
	spall::RayTracingPipelineCreateInfo info = validRayTracingPipeline({}, {});
	info.MaxPayloadSize = 0;

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);

	info = validRayTracingPipeline({}, {});
	info.MaxPayloadSize = 6;

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);

	info = validRayTracingPipeline({}, {});
	info.MaxAttributeSize = spall::MaxRayTracingAttributeSize + 4;

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);

	info = validRayTracingPipeline({}, {});
	info.MaxRecursionDepth = 0;

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);

	info.MaxRecursionDepth = spall::MaxRayRecursionDepth + 1;

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Ray-tracing push constants require ray-tracing stages",
	"[pipeline][raytracing][pushconstants]")
{
	spall::RayTracingPipelineCreateInfo info = validRayTracingPipeline({}, {});
	info.PushConstants = {spall::ShaderStageFlags::RayGeneration | spall::ShaderStageFlags::ClosestHit, 32};

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) == spall::SUCCESS);

	info.PushConstants.Stages = spall::ShaderStageFlags::Compute;

	CHECK(spall::validateRayTracingPipelineCreateInfo(info) != spall::SUCCESS);
}
