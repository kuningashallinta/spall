#include <catch2/catch_test_macros.hpp>

#include <spall/CommandList/ICommandList.h>
#include <spall/Device/IDevice.h>
#include <spall/Runtime/Renderer.h>
#include <spall/SwapChain/ISwapChain.h>
#include <tests/Support/TestWindow.h>

#include <algorithm>
#include <cstdint>
#include <vector>

static spall::Renderer createRenderer(
	spall::RenderBackendType backendType,
	HWND window)
{
	spall::RendererCreateInfo info = {};
	info.Backend = backendType;
	info.SwapChain.Window.Type = spall::WindowHandleType::Win32;
	info.SwapChain.Window.Value = window;
	info.SwapChain.Width = 256;
	info.SwapChain.Height = 256;
	info.SwapChain.Format = spall::Format::BGRA8Srgb;

	return spall::Renderer::create(info);
}

static void runFrameLifecycle(
	spall::RenderBackendType backendType)
{
	const HiddenWindow window;

	if (window.handle() == nullptr)
	{
		SKIP("The test window could not be created.");
	}

	spall::Renderer renderer = createRenderer(backendType, window.handle());

	if (not renderer)
	{
		SKIP("The backend is unavailable on this machine.");
	}

	const std::uint32_t frameCount = renderer.swapChain().frameCount();
	REQUIRE(frameCount > 0);

	std::vector<spall::ICommandList*> observedCommandLists;

	for (std::uint32_t frameIndex = 0; frameIndex < (frameCount * 4); ++frameIndex)
	{
		spall::Frame frame = renderer.beginFrame();
		REQUIRE(static_cast<bool>(frame));

		observedCommandLists.push_back(&frame.commands());

		REQUIRE(frame.end() == spall::SUCCESS);
		CHECK(not frame);
	}

	std::vector<spall::ICommandList*> distinctCommandLists = observedCommandLists;
	std::sort(distinctCommandLists.begin(), distinctCommandLists.end());
	distinctCommandLists.erase(
		std::unique(distinctCommandLists.begin(), distinctCommandLists.end()),
		distinctCommandLists.end());

	CHECK(distinctCommandLists.size() <= frameCount);

	{
		spall::Frame cancelled = renderer.beginFrame();
		REQUIRE(static_cast<bool>(cancelled));
	}

	spall::Frame resumed = renderer.beginFrame();
	REQUIRE(static_cast<bool>(resumed));
	REQUIRE(resumed.end() == spall::SUCCESS);

	CHECK(renderer.resize(320, 240) == spall::SUCCESS);
	CHECK(renderer.width() == 320);
	CHECK(renderer.height() == 240);

	spall::Frame afterResize = renderer.beginFrame();
	REQUIRE(static_cast<bool>(afterResize));
	REQUIRE(afterResize.end() == spall::SUCCESS);
}

TEST_CASE(
	"A managed renderer reuses one command list per frame in flight",
	"[runtime][GPU]")
{
#if SPALL_HAS_D3D12
	SECTION("on D3D12")
	{
		runFrameLifecycle(spall::RenderBackendType::D3D12);
	}
#endif

#if SPALL_HAS_VULKAN
	SECTION("on Vulkan")
	{
		runFrameLifecycle(spall::RenderBackendType::Vulkan);
	}
#endif
}
