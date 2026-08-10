#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/ShaderValidation.h>

#include <cstddef>
#include <span>

static const std::byte Bytecode[8] = {};

static spall::ShaderCreateInfo shaderCreateInfo(
	spall::ShaderStage stage)
{
	spall::ShaderCreateInfo info = {};
	info.Stage = stage;
	info.Bytecode = std::span(Bytecode);

	return info;
}

TEST_CASE(
	"Every supported shader stage is accepted",
	"[shader]")
{
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::Vertex)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::Fragment)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::Compute)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::Geometry)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::TessellationControl)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::TessellationEvaluation)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::RayGeneration)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::Miss)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::ClosestHit)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::AnyHit)) == spall::SUCCESS);
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(spall::ShaderStage::Intersection)) == spall::SUCCESS);
}

TEST_CASE(
	"A shader rejects an unsupported stage",
	"[shader]")
{
	CHECK(spall::validateShaderCreateInfo(shaderCreateInfo(static_cast<spall::ShaderStage>(99))) != spall::SUCCESS);
}

TEST_CASE(
	"A shader rejects empty bytecode",
	"[shader]")
{
	spall::ShaderCreateInfo info = shaderCreateInfo(spall::ShaderStage::Vertex);
	info.Bytecode = {};

	CHECK(spall::validateShaderCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"Empty bytecode is rejected before the stage is considered",
	"[shader]")
{
	spall::ShaderCreateInfo info = shaderCreateInfo(static_cast<spall::ShaderStage>(99));
	info.Bytecode = {};

	CHECK(spall::validateShaderCreateInfo(info) == spall::ERR_INVALID_SHADER_BYTECODE);
}

TEST_CASE(
	"A single byte of bytecode is enough to pass validation",
	"[shader]")
{
	spall::ShaderCreateInfo info = shaderCreateInfo(spall::ShaderStage::Vertex);
	info.Bytecode = std::span(Bytecode, 1);

	CHECK(spall::validateShaderCreateInfo(info) == spall::SUCCESS);
}
