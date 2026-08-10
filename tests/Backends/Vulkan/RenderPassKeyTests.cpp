#include <catch2/catch_test_macros.hpp>

#include <src/Backends/Vulkan/CommandList/VK_RenderPassKey.h>

static spall::vk::RenderPassKey colorKey()
{
	spall::vk::RenderPassKey key = {};
	key.ColorCount = 1;
	key.ColorFormats[0] = spall::Format::RGBA8;
	key.ColorLoadActions[0] = spall::LoadAction::Clear;
	key.ColorStoreActions[0] = spall::StoreAction::Store;

	return key;
}

static spall::vk::RenderPassKey depthKey()
{
	spall::vk::RenderPassKey key = colorKey();
	key.DepthFormat = spall::Format::Depth32Float;
	key.DepthLoadAction = spall::LoadAction::Clear;
	key.StencilLoadAction = spall::LoadAction::DontCare;

	return key;
}

TEST_CASE(
	"Identical render-pass keys are equal",
	"[renderpasskey]")
{
	CHECK(colorKey() == colorKey());
	CHECK(depthKey() == depthKey());
}

TEST_CASE(
	"A render-pass key distinguishes color formats",
	"[renderpasskey]")
{
	spall::vk::RenderPassKey other = colorKey();
	other.ColorFormats[0] = spall::Format::BGRA8;

	CHECK_FALSE(colorKey() == other);
}

TEST_CASE(
	"A render-pass key distinguishes clearing from loading",
	"[renderpasskey]")
{
	spall::vk::RenderPassKey other = colorKey();
	other.ColorLoadActions[0] = spall::LoadAction::Load;

	CHECK_FALSE(colorKey() == other);
}

TEST_CASE(
	"A render-pass key distinguishes store actions",
	"[renderpasskey]")
{
	spall::vk::RenderPassKey other = colorKey();
	other.ColorStoreActions[0] = spall::StoreAction::DontCare;

	CHECK_FALSE(colorKey() == other);
}

TEST_CASE(
	"A render-pass key distinguishes attachment counts",
	"[renderpasskey]")
{
	spall::vk::RenderPassKey other = colorKey();
	other.ColorCount = 2;
	other.ColorFormats[1] = spall::Format::RGBA8;

	CHECK_FALSE(colorKey() == other);
}

TEST_CASE(
	"A render-pass key distinguishes later color attachments",
	"[renderpasskey]")
{
	spall::vk::RenderPassKey key = colorKey();
	key.ColorCount = 2;
	key.ColorFormats[1] = spall::Format::RGBA8;
	key.ColorLoadActions[1] = spall::LoadAction::Clear;
	key.ColorStoreActions[1] = spall::StoreAction::Store;

	spall::vk::RenderPassKey formatDiffers = key;
	formatDiffers.ColorFormats[1] = spall::Format::BGRA8;

	spall::vk::RenderPassKey loadDiffers = key;
	loadDiffers.ColorLoadActions[1] = spall::LoadAction::Load;

	spall::vk::RenderPassKey storeDiffers = key;
	storeDiffers.ColorStoreActions[1] = spall::StoreAction::DontCare;

	CHECK_FALSE(key == formatDiffers);
	CHECK_FALSE(key == loadDiffers);
	CHECK_FALSE(key == storeDiffers);
}

TEST_CASE(
	"A render-pass key distinguishes depth formats",
	"[renderpasskey]")
{
	CHECK_FALSE(colorKey() == depthKey());

	spall::vk::RenderPassKey other = depthKey();
	other.DepthFormat = spall::Format::Depth24Stencil8;

	CHECK_FALSE(depthKey() == other);
}

TEST_CASE(
	"A render-pass key distinguishes depth and stencil load actions",
	"[renderpasskey]")
{
	spall::vk::RenderPassKey depthDiffers = depthKey();
	depthDiffers.DepthLoadAction = spall::LoadAction::Load;

	spall::vk::RenderPassKey stencilDiffers = depthKey();
	stencilDiffers.StencilLoadAction = spall::LoadAction::Clear;

	CHECK_FALSE(depthKey() == depthDiffers);
	CHECK_FALSE(depthKey() == stencilDiffers);
}

TEST_CASE(
	"A render-pass key distinguishes depth and stencil store actions",
	"[renderpasskey]")
{
	spall::vk::RenderPassKey depthDiffers = depthKey();
	depthDiffers.DepthStoreAction = spall::StoreAction::DontCare;

	spall::vk::RenderPassKey stencilDiffers = depthKey();
	stencilDiffers.StencilStoreAction = spall::StoreAction::DontCare;

	CHECK_FALSE(depthKey() == depthDiffers);
	CHECK_FALSE(depthKey() == stencilDiffers);
}
