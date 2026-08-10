#include <catch2/catch_test_macros.hpp>

#include <spall/CommandList/ICommandList.h>
#include <spall/Device/IDevice.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <spall/Resources/Texture/ITexture.h>
#include <src/Validation/Common/FormatValidation.h>
#include <src/Validation/Common/TextureValidation.h>
#include <tests/Support/TestDevice.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

static void runBlockCompressedMipRoundTrip(
	spall::RenderBackendType backendType)
{
	const TestDevice testDevice = requireDevice(backendType);
	spall::IDevice& device = *testDevice.Device;

	constexpr spall::Format Format = spall::Format::BC1RGBAUnorm;
	constexpr std::uint32_t Extent = 8;
	constexpr std::uint32_t MipLevels = 4;

	spall::TextureCreateInfo textureInfo = {};
	textureInfo.Width = Extent;
	textureInfo.Height = Extent;
	textureInfo.MipLevels = MipLevels;
	textureInfo.Format = Format;
	textureInfo.Usage =
		spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::TransferSource |
		spall::TextureUsageFlags::TransferDestination;

	spall::Resource<spall::ITexture> texture;
	REQUIRE(device.resources().createTexture(textureInfo, &texture) == spall::SUCCESS);

	const std::uint32_t blockWidth = spall::formatBlockWidth(Format);
	const std::uint32_t blockHeight = spall::formatBlockHeight(Format);

	for (std::uint32_t mipLevel = 0; mipLevel < MipLevels; ++mipLevel)
	{
		const std::uint32_t mipWidth = spall::mipLevelExtent(Extent, mipLevel);
		const std::uint32_t mipHeight = spall::mipLevelExtent(Extent, mipLevel);

		INFO("mip level " << mipLevel << " covering " << mipWidth << "x" << mipHeight << " texels");

		const std::uint32_t rowPitch = spall::formatBlockCount(mipWidth, blockWidth) * spall::formatBytesPerBlock(Format);
		const std::uint32_t rowCount = spall::formatBlockCount(mipHeight, blockHeight);

		std::vector<std::uint8_t> data(static_cast<std::size_t>(rowPitch) * rowCount);

		for (std::size_t index = 0; index < data.size(); ++index)
		{
			data[index] = static_cast<std::uint8_t>((index * 13) + (mipLevel * 61) + 7);
		}

		spall::BufferCreateInfo uploadInfo = {};
		uploadInfo.Size = static_cast<std::uint32_t>(data.size());
		uploadInfo.Usage = spall::BufferUsageFlags::TransferSource;
		uploadInfo.CpuAccess = spall::MemoryAccess::Write;

		spall::Resource<spall::IBuffer> upload;
		REQUIRE(device.resources()
					.createBufferWithData(
						uploadInfo,
						std::span<const std::byte>(reinterpret_cast<const std::byte*>(data.data()), data.size()),
						&upload) == spall::SUCCESS);

		spall::BufferCreateInfo readbackInfo = {};
		readbackInfo.Size = uploadInfo.Size;
		readbackInfo.Usage = spall::BufferUsageFlags::TransferDestination;
		readbackInfo.CpuAccess = spall::MemoryAccess::Read;
		readbackInfo.InitialState = spall::ResourceStateFlags::CopyDest;

		spall::Resource<spall::IBuffer> readback;
		REQUIRE(device.resources().createBuffer(readbackInfo, &readback) == spall::SUCCESS);

		spall::TextureRegion region = {};
		region.MipLevel = mipLevel;

		spall::Resource<spall::ICommandList> commands;
		REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

		REQUIRE(commands->begin() == spall::SUCCESS);
		REQUIRE(commands->copyBufferToTexture(*texture, region, *upload, 0, rowPitch) == spall::SUCCESS);
		REQUIRE(commands->copyTextureToBuffer(*readback, 0, rowPitch, *texture, region) == spall::SUCCESS);
		REQUIRE(commands->end() == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

		std::vector<std::uint8_t> observed(data.size());

		REQUIRE(device.resources()
					.readBuffer(
						*readback,
						std::span<std::byte>(reinterpret_cast<std::byte*>(observed.data()), observed.size()),
						0) == spall::SUCCESS);

		CHECK(observed == data);
	}
}

TEST_CASE(
	"A block-compressed texture round-trips every mip level",
	"[copy][GPU]")
{
#if SPALL_HAS_D3D12
	SECTION("on D3D12")
	{
		runBlockCompressedMipRoundTrip(spall::RenderBackendType::D3D12);
	}
#endif

#if SPALL_HAS_VULKAN
	SECTION("on Vulkan")
	{
		runBlockCompressedMipRoundTrip(spall::RenderBackendType::Vulkan);
	}
#endif
}
