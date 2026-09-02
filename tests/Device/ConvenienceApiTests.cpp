#include <catch2/catch_test_macros.hpp>

#include <spall/Backend/IBackend.h>
#include <spall/CommandList/ICommandList.h>
#include <spall/Device/IDevice.h>
#include <spall/Device/IPipelineFactory.h>
#include <spall/Device/IPresentationFactory.h>
#include <spall/Device/IResourceFactory.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <tests/Support/Fakes.h>

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

class FakeResourceFactory final : public spall::IResourceFactory
{
public:
	FakeResourceFactory(
		void)
		: m_Buffer(spall::BufferInfo {.Size = 256, .Usage = spall::BufferUsageFlags::Vertex})
	{
	}

	spall::Status createTexture1D(
		const spall::Texture1DCreateInfo&,
		spall::Resource<spall::ITexture1D>*) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::Status createTexture2D(
		const spall::Texture2DCreateInfo&,
		spall::Resource<spall::ITexture2D>*) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::Status createTexture3D(
		const spall::Texture3DCreateInfo&,
		spall::Resource<spall::ITexture3D>*) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::Status createTextureView(
		const spall::TextureViewCreateInfo&,
		spall::Resource<spall::ITextureView>*) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::Status createFramebuffer(
		const spall::FramebufferCreateInfo&,
		spall::Resource<spall::IFramebuffer>*) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::Status createBuffer(
		const spall::BufferCreateInfo&,
		spall::Resource<spall::IBuffer>*) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::Status createBufferWithData(
		const spall::BufferCreateInfo& createInfo,
		std::span<const std::byte> data,
		spall::Resource<spall::IBuffer>* buffer) override
	{
		LastCreateInfo = createInfo;
		LastDataSize = data.size_bytes();

		if (FailCreation)
		{
			return spall::ERR_INVALID_ARGUMENT;
		}

		buffer->reset(&m_Buffer);
		return {};
	}

	spall::Status writeBuffer(
		spall::IBuffer&,
		std::span<const std::byte> data,
		std::uint32_t offset) override
	{
		LastDataSize = data.size_bytes();
		LastOffset = offset;
		return {};
	}

	spall::Status readBuffer(
		spall::IBuffer&,
		std::span<std::byte> data,
		std::uint32_t offset) override
	{
		LastDataSize = data.size_bytes();
		LastOffset = offset;
		return {};
	}

	spall::Status createSampler(
		const spall::SamplerCreateInfo&,
		spall::Resource<spall::ISampler>*) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::Status createQueryPool(
		const spall::QueryPoolCreateInfo&,
		spall::Resource<spall::IQueryPool>*) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::Status readTimestamps(
		spall::IQueryPool&,
		std::uint32_t,
		std::span<std::uint64_t>) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	spall::Status createAccelerationStructure(
		const spall::AccelerationStructureCreateInfo&,
		spall::Resource<spall::IAccelerationStructure>*) override
	{
		return spall::ERR_UNSUPPORTED;
	}

	bool FailCreation = false;
	spall::BufferCreateInfo LastCreateInfo = {};
	std::size_t LastDataSize = 0;
	std::uint32_t LastOffset = 0;

private:
	FakeBuffer m_Buffer;
};

TEST_CASE(
	"Creation interfaces expose direct-return overloads",
	"[convenience]")
{
	STATIC_REQUIRE(requires(
		spall::IBackend& backend,
		spall::IDevice& device,
		spall::IPresentationFactory& presentation,
		spall::IResourceFactory& resources,
		spall::IPipelineFactory& pipelines,
		spall::IGraphicsQueue& queue,
		spall::ISwapChain& swapChain) {
		{ backend.createDevice() } -> std::same_as<spall::Resource<spall::IDevice>>;
		{ device.createCommandList() } -> std::same_as<spall::Resource<spall::ICommandList>>;
		{ presentation.createSwapChain({}) } -> std::same_as<spall::Resource<spall::ISwapChain>>;
		{ resources.createTexture1D({}) } -> std::same_as<spall::Resource<spall::ITexture1D>>;
		{ resources.createTexture2D({}) } -> std::same_as<spall::Resource<spall::ITexture2D>>;
		{ resources.createTexture3D({}) } -> std::same_as<spall::Resource<spall::ITexture3D>>;
		{ resources.createTextureView({}) } -> std::same_as<spall::Resource<spall::ITextureView>>;
		{ resources.createFramebuffer({}) } -> std::same_as<spall::Resource<spall::IFramebuffer>>;
		{ resources.createBuffer({}) } -> std::same_as<spall::Resource<spall::IBuffer>>;
		{ resources.createSampler({}) } -> std::same_as<spall::Resource<spall::ISampler>>;
		{ pipelines.createShader({}) } -> std::same_as<spall::Resource<spall::IShader>>;
		{ pipelines.createResourceSetLayout({}) } -> std::same_as<spall::Resource<spall::IResourceSetLayout>>;
		{ pipelines.createResourceSet({}) } -> std::same_as<spall::Resource<spall::IResourceSet>>;
		{ pipelines.createPipeline({}) } -> std::same_as<spall::Resource<spall::IPipeline>>;
		{ pipelines.createComputePipeline({}) } -> std::same_as<spall::Resource<spall::IPipeline>>;
		{ queue.acquireFrame(swapChain) } -> std::same_as<spall::Resource<spall::IFrame>>;
	});
}

TEST_CASE(
	"Typed buffer creation infers the byte size",
	"[convenience][buffer]")
{
	FakeResourceFactory implementation;
	spall::IResourceFactory& resources = implementation;
	const std::uint32_t data[] = {1, 2, 3};

	spall::BufferCreateInfo info = {};
	info.Usage = spall::BufferUsageFlags::Vertex;

	const spall::Resource<spall::IBuffer> buffer = resources.createBufferWithData(info, data);

	REQUIRE(buffer);
	CHECK(implementation.LastCreateInfo.Size == sizeof(data));
	CHECK(implementation.LastDataSize == sizeof(data));
}

TEST_CASE(
	"Typed buffer creation preserves an explicit size",
	"[convenience][buffer]")
{
	FakeResourceFactory implementation;
	spall::IResourceFactory& resources = implementation;
	const std::uint32_t data[] = {1, 2, 3};

	spall::BufferCreateInfo info = {};
	info.Size = 64;
	info.Usage = spall::BufferUsageFlags::Vertex;

	const spall::Resource<spall::IBuffer> buffer = resources.createBufferWithData(info, data);

	REQUIRE(buffer);
	CHECK(implementation.LastCreateInfo.Size == 64);
	CHECK(implementation.LastDataSize == sizeof(data));
}

TEST_CASE(
	"Direct-return creation returns an empty resource on failure",
	"[convenience][buffer]")
{
	FakeResourceFactory implementation;
	implementation.FailCreation = true;
	spall::IResourceFactory& resources = implementation;
	const std::uint32_t data[] = {1, 2, 3};

	const spall::Resource<spall::IBuffer> buffer = resources.createBufferWithData({}, data);

	CHECK(not buffer);
}

TEST_CASE(
	"Typed buffer transfers preserve byte counts and offsets",
	"[convenience][buffer]")
{
	FakeResourceFactory implementation;
	spall::IResourceFactory& resources = implementation;
	FakeBuffer buffer(spall::BufferInfo {
		.Size = 256,
		.Usage = spall::BufferUsageFlags::TransferSource | spall::BufferUsageFlags::TransferDestination});
	std::uint32_t data[] = {1, 2, 3};

	REQUIRE(resources.writeBuffer(buffer, data, 12) == spall::SUCCESS);
	CHECK(implementation.LastDataSize == sizeof(data));
	CHECK(implementation.LastOffset == 12);

	REQUIRE(resources.readBuffer(buffer, data, 20) == spall::SUCCESS);
	CHECK(implementation.LastDataSize == sizeof(data));
	CHECK(implementation.LastOffset == 20);
}
