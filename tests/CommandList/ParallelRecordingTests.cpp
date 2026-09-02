#include <catch2/catch_test_macros.hpp>

#include <spall/CommandList/ICommandList.h>
#include <spall/Device/IDevice.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <spall/Resources/Texture/ITexture.h>
#include <tests/Support/TestDevice.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <thread>
#include <vector>

struct RecordingWork
{
	spall::Resource<spall::ITexture2D> Texture;
	spall::Resource<spall::IBuffer> Upload;
	spall::Resource<spall::ICommandList> Commands;
	bool Recorded = false;
};

static constexpr std::uint32_t Extent = 32;
static constexpr std::uint32_t MipLevels = 6;

static bool createWork(
	spall::IDevice& device,
	RecordingWork* work)
{
	spall::Texture2DCreateInfo textureInfo = {};
	textureInfo.Width = Extent;
	textureInfo.Height = Extent;
	textureInfo.MipLevels = MipLevels;
	textureInfo.Format = spall::Format::RGBA8;
	textureInfo.Usage =
		spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::TransferSource |
		spall::TextureUsageFlags::TransferDestination;

	if (device.resources().createTexture2D(textureInfo, &work->Texture) != spall::SUCCESS)
	{
		return false;
	}

	const std::uint32_t basePitch = Extent * 4;
	const std::vector<std::uint8_t> baseLevel(static_cast<std::size_t>(basePitch) * Extent, 200);

	spall::BufferCreateInfo uploadInfo = {};
	uploadInfo.Size = static_cast<std::uint32_t>(baseLevel.size());
	uploadInfo.Usage = spall::BufferUsageFlags::TransferSource;
	uploadInfo.CpuAccess = spall::MemoryAccess::Write;
	uploadInfo.InitialState = spall::ResourceStateFlags::CopySource;

	return device.resources()
			   .createBufferWithData(
				   uploadInfo,
				   std::span<const std::byte>(reinterpret_cast<const std::byte*>(baseLevel.data()), baseLevel.size()),
				   &work->Upload) == spall::SUCCESS;
}

static void recordWork(
	spall::IDevice& device,
	RecordingWork* work)
{
	if (device.createCommandList(&work->Commands) != spall::SUCCESS)
	{
		return;
	}

	spall::TextureRegion baseRegion = {};

	const bool recorded = work->Commands->begin() == spall::SUCCESS and
		work->Commands->copyBufferToTexture(*work->Texture, baseRegion, *work->Upload, 0, Extent * 4) == spall::SUCCESS and
		work->Commands->generateMips(*work->Texture) == spall::SUCCESS and work->Commands->end() == spall::SUCCESS;

	work->Recorded = recorded;
}

static void runParallelRecording(
	spall::RenderBackendType backendType)
{
	const TestDevice testDevice = requireDevice(backendType);
	spall::IDevice& device = *testDevice.Device;

	const std::uint32_t threadCount = std::clamp(std::thread::hardware_concurrency(), 4u, 8u);

	for (std::uint32_t iteration = 0; iteration < 4; ++iteration)
	{
		std::vector<RecordingWork> work(threadCount);

		for (RecordingWork& item : work)
		{
			REQUIRE(createWork(device, &item));
		}

		std::vector<std::thread> threads;

		for (RecordingWork& item : work)
		{
			threads.emplace_back(recordWork, std::ref(device), &item);
		}

		for (std::thread& thread : threads)
		{
			thread.join();
		}

		for (RecordingWork& item : work)
		{
			REQUIRE(item.Recorded);
			REQUIRE(device.graphicsQueue().submit(*item.Commands) == spall::SUCCESS);
		}

		REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);
	}
}

TEST_CASE(
	"Command lists record in parallel against one device",
	"[recording][GPU][threading]")
{
#if SPALL_HAS_D3D12
	SECTION("on D3D12")
	{
		runParallelRecording(spall::RenderBackendType::D3D12);
	}
#endif

#if SPALL_HAS_VULKAN
	SECTION("on Vulkan")
	{
		runParallelRecording(spall::RenderBackendType::Vulkan);
	}
#endif
}
