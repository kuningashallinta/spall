#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/SwapChainValidation.h>

static int WindowStorage = 0;

static spall::SwapChainCreateInfo swapChainCreateInfo()
{
	spall::SwapChainCreateInfo info = {};
	info.Window.Type = spall::WindowHandleType::Win32;
	info.Window.Value = &WindowStorage;
	info.Width = 1280;
	info.Height = 720;
	info.Format = spall::Format::BGRA8;
	info.PresentMode = spall::PresentMode::VSync;

	return info;
}

TEST_CASE(
	"A well-formed swap chain is valid",
	"[swapchain]")
{
	CHECK(spall::validateSwapChainCreateInfo(swapChainCreateInfo()) == spall::SUCCESS);
}

TEST_CASE(
	"A swap chain accepts sRGB color formats",
	"[swapchain][srgb]")
{
	spall::SwapChainCreateInfo rgba = swapChainCreateInfo();
	rgba.Format = spall::Format::RGBA8Srgb;

	spall::SwapChainCreateInfo bgra = swapChainCreateInfo();
	bgra.Format = spall::Format::BGRA8Srgb;

	CHECK(spall::validateSwapChainCreateInfo(rgba) == spall::SUCCESS);
	CHECK(spall::validateSwapChainCreateInfo(bgra) == spall::SUCCESS);
}

TEST_CASE(
	"A swap chain requires a non-null window handle",
	"[swapchain]")
{
	spall::SwapChainCreateInfo info = swapChainCreateInfo();
	info.Window.Value = nullptr;

	CHECK(spall::validateSwapChainCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A swap chain rejects a non-Win32 window handle",
	"[swapchain]")
{
	spall::SwapChainCreateInfo info = swapChainCreateInfo();
	info.Window.Type = static_cast<spall::WindowHandleType>(99);

	CHECK(spall::validateSwapChainCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A swap chain rejects zero dimensions",
	"[swapchain]")
{
	spall::SwapChainCreateInfo zeroWidth = swapChainCreateInfo();
	zeroWidth.Width = 0;
	CHECK(spall::validateSwapChainCreateInfo(zeroWidth) != spall::SUCCESS);

	spall::SwapChainCreateInfo zeroHeight = swapChainCreateInfo();
	zeroHeight.Height = 0;
	CHECK(spall::validateSwapChainCreateInfo(zeroHeight) != spall::SUCCESS);
}

TEST_CASE(
	"A swap chain requires an explicit format",
	"[swapchain]")
{
	spall::SwapChainCreateInfo info = swapChainCreateInfo();
	info.Format = spall::Format::Unknown;

	CHECK(spall::validateSwapChainCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A swap chain requires a color format",
	"[swapchain]")
{
	spall::SwapChainCreateInfo info = swapChainCreateInfo();
	info.Format = spall::Format::Depth32Float;

	CHECK(spall::validateSwapChainCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A swap chain rejects a block-compressed format",
	"[swapchain][compressed]")
{
	spall::SwapChainCreateInfo info = swapChainCreateInfo();
	info.Format = spall::Format::BC1RGBAUnorm;

	CHECK(spall::validateSwapChainCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A swap chain accepts each supported present mode",
	"[swapchain]")
{
	spall::SwapChainCreateInfo immediate = swapChainCreateInfo();
	immediate.PresentMode = spall::PresentMode::Immediate;
	CHECK(spall::validateSwapChainCreateInfo(immediate) == spall::SUCCESS);

	spall::SwapChainCreateInfo vsync = swapChainCreateInfo();
	vsync.PresentMode = spall::PresentMode::VSync;
	CHECK(spall::validateSwapChainCreateInfo(vsync) == spall::SUCCESS);
}

TEST_CASE(
	"A swap chain rejects an invalid present mode",
	"[swapchain]")
{
	spall::SwapChainCreateInfo info = swapChainCreateInfo();
	info.PresentMode = static_cast<spall::PresentMode>(99);

	CHECK(spall::validateSwapChainCreateInfo(info) != spall::SUCCESS);
}
