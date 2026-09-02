#include <catch2/catch_test_macros.hpp>

#include <spall/CommandList/ICommandList.h>
#include <spall/CommandList/IndirectCommands.h>
#include <spall/Device/IDevice.h>
#include <spall/Frame/IFrame.h>
#include <spall/Framebuffer/IFramebuffer.h>
#include <spall/Pipeline/Binding/IResourceSet.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <spall/Resources/Query/IQueryPool.h>
#include <spall/Resources/Sampler/ISampler.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/Texture/TextureSubresourceRange.h>
#include <spall/Resources/TextureView/ITextureView.h>
#include <spall/SwapChain/ISwapChain.h>
#include <tests/Support/TestDevice.h>
#include <tests/Support/TestWindow.h>

#include <windows.h>

#include <d3dcompiler.h>
#include <wrl/client.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

static std::uint32_t mipExtent(
	std::uint32_t baseExtent,
	std::uint32_t mipLevel)
{
	const std::uint32_t extent = baseExtent >> mipLevel;

	return (extent != 0) ? extent : 1;
}

static void clearAndReadBack(
	spall::IDevice& device,
	std::uint32_t extent)
{
	spall::Texture2DCreateInfo textureInfo = {};
	textureInfo.Width = extent;
	textureInfo.Height = extent;
	textureInfo.Format = spall::Format::RGBA8;
	textureInfo.Usage = spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::TransferSource;

	spall::Resource<spall::ITexture2D> texture;
	REQUIRE(device.resources().createTexture2D(textureInfo, &texture) == spall::SUCCESS);

	spall::TextureViewCreateInfo viewInfo = {};
	viewInfo.Texture = texture.get();

	spall::Resource<spall::ITextureView> view;
	REQUIRE(device.resources().createTextureView(viewInfo, &view) == spall::SUCCESS);

	spall::FramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.ColorAttachments[0] = view.get();
	framebufferInfo.ColorAttachmentCount = 1;

	spall::Resource<spall::IFramebuffer> framebuffer;
	REQUIRE(device.resources().createFramebuffer(framebufferInfo, &framebuffer) == spall::SUCCESS);

	const std::uint32_t rowPitch = extent * 4;

	spall::BufferCreateInfo readbackInfo = {};
	readbackInfo.Size = rowPitch * extent;
	readbackInfo.Usage = spall::BufferUsageFlags::TransferDestination;
	readbackInfo.CpuAccess = spall::MemoryAccess::Read;
	readbackInfo.InitialState = spall::ResourceStateFlags::CopyDest;

	spall::Resource<spall::IBuffer> readback;
	REQUIRE(device.resources().createBuffer(readbackInfo, &readback) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	spall::RenderPassBeginInfo passInfo = {};
	passInfo.Framebuffer = framebuffer.get();
	passInfo.ColorAttachments[0].LoadAction = spall::LoadAction::Clear;
	passInfo.ColorAttachments[0].ClearColor = {0.25f, 0.5f, 0.75f, 1.0f};

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->beginRenderPass(passInfo) == spall::SUCCESS);
	REQUIRE(commands->endRenderPass() == spall::SUCCESS);
	REQUIRE(commands->copyTextureToBuffer(*readback, 0, rowPitch, *texture, {}) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	std::vector<std::uint8_t> pixels(readbackInfo.Size);
	REQUIRE(device.resources()
				.readBuffer(
					*readback,
					std::span<std::byte>(reinterpret_cast<std::byte*>(pixels.data()), pixels.size()),
					0) == spall::SUCCESS);

	const std::uint8_t expected[4] = {64, 128, 191, 255};
	bool everyPixelMatches = true;

	for (std::uint32_t y = 0; y < extent; ++y)
	{
		for (std::uint32_t x = 0; x < extent; ++x)
		{
			const std::uint8_t* pixel = pixels.data() + (y * rowPitch) + (x * 4);

			for (std::uint32_t channel = 0; channel < 4; ++channel)
			{
				const int difference = static_cast<int>(pixel[channel]) - static_cast<int>(expected[channel]);

				if ((difference > 1) or (difference < -1))
				{
					everyPixelMatches = false;
				}
			}
		}
	}

	CHECK(everyPixelMatches);
}

TEST_CASE(
	"A D3D12 device reports usable limits",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	const spall::DeviceLimits& limits = testDevice.Device->limits();

	CHECK(limits.MaxTexture2DDimension >= 16384);
	CHECK(limits.MaxColorAttachments == spall::MaxColorAttachments);
	CHECK(limits.MaxResourceSets == spall::MaxResourceSets);
	CHECK((limits.SupportedSampleCounts & 1u) != 0);
}

TEST_CASE(
	"A D3D12 compute queue submits a compute command list",
	"[d3d12][GPU][compute]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::ICommandList> compute;
	REQUIRE(device.createCommandList(spall::QueueType::Compute, &compute) == spall::SUCCESS);

	REQUIRE(compute->begin() == spall::SUCCESS);
	REQUIRE(compute->end() == spall::SUCCESS);
	REQUIRE(device.computeQueue().submit(*compute) == spall::SUCCESS);
	REQUIRE(device.computeQueue().waitIdle() == spall::SUCCESS);
}

TEST_CASE(
	"A D3D12 queue rejects a command list of the wrong type",
	"[d3d12][GPU][compute]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::ICommandList> compute;
	REQUIRE(device.createCommandList(spall::QueueType::Compute, &compute) == spall::SUCCESS);
	REQUIRE(compute->begin() == spall::SUCCESS);
	REQUIRE(compute->end() == spall::SUCCESS);
	CHECK(device.graphicsQueue().submit(*compute) != spall::SUCCESS);

	spall::Resource<spall::ICommandList> graphics;
	REQUIRE(device.createCommandList(spall::QueueType::Graphics, &graphics) == spall::SUCCESS);
	REQUIRE(graphics->begin() == spall::SUCCESS);
	REQUIRE(graphics->end() == spall::SUCCESS);
	CHECK(device.computeQueue().submit(*graphics) != spall::SUCCESS);
}

TEST_CASE(
	"A D3D12 compute queue can wait on the graphics queue",
	"[d3d12][GPU][compute]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::ICommandList> graphics;
	REQUIRE(device.createCommandList(spall::QueueType::Graphics, &graphics) == spall::SUCCESS);
	REQUIRE(graphics->begin() == spall::SUCCESS);
	REQUIRE(graphics->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*graphics) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> compute;
	REQUIRE(device.createCommandList(spall::QueueType::Compute, &compute) == spall::SUCCESS);
	REQUIRE(compute->begin() == spall::SUCCESS);
	REQUIRE(compute->end() == spall::SUCCESS);

	REQUIRE(device.computeQueue().waitForQueue(device.graphicsQueue()) == spall::SUCCESS);
	REQUIRE(device.computeQueue().submit(*compute) == spall::SUCCESS);
	REQUIRE(device.computeQueue().waitIdle() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);
}

TEST_CASE(
	"A D3D12 buffer round-trips through a device-local copy",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	std::vector<std::uint8_t> sourceData(1024);

	for (std::size_t index = 0; index < sourceData.size(); ++index)
	{
		sourceData[index] = static_cast<std::uint8_t>((index * 7) + 3);
	}

	spall::BufferCreateInfo deviceInfo = {};
	deviceInfo.Size = static_cast<std::uint32_t>(sourceData.size());
	deviceInfo.Usage = spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::TransferSource;
	deviceInfo.InitialState = spall::ResourceStateFlags::VertexBuffer;

	spall::Resource<spall::IBuffer> deviceBuffer;
	REQUIRE(device.resources()
				.createBufferWithData(
					deviceInfo,
					std::span<const std::byte>(reinterpret_cast<const std::byte*>(sourceData.data()), sourceData.size()),
					&deviceBuffer) == spall::SUCCESS);

	spall::BufferCreateInfo readbackInfo = {};
	readbackInfo.Size = deviceInfo.Size;
	readbackInfo.Usage = spall::BufferUsageFlags::TransferDestination;
	readbackInfo.CpuAccess = spall::MemoryAccess::Read;
	readbackInfo.InitialState = spall::ResourceStateFlags::CopyDest;

	spall::Resource<spall::IBuffer> readback;
	REQUIRE(device.resources().createBuffer(readbackInfo, &readback) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->copyBuffer(*readback, 0, *deviceBuffer, 0, deviceInfo.Size) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	std::vector<std::uint8_t> readbackData(sourceData.size());
	REQUIRE(device.resources()
				.readBuffer(
					*readback,
					std::span<std::byte>(reinterpret_cast<std::byte*>(readbackData.data()), readbackData.size()),
					0) == spall::SUCCESS);

	CHECK(readbackData == sourceData);
}

TEST_CASE(
	"A D3D12 render pass clears to the requested color",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);

	SECTION("with a row pitch the copy alignment already satisfies")
	{
		clearAndReadBack(*testDevice.Device, 64);
	}

	SECTION("with a row pitch the copy must realign")
	{
		clearAndReadBack(*testDevice.Device, 60);
	}
}

TEST_CASE(
	"A D3D12 resource set accepts writes matching its layout",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::ISampler> sampler = device.resources().createSampler({});
	REQUIRE(sampler);

	spall::BufferCreateInfo uniformInfo = {};
	uniformInfo.Size = 256;
	uniformInfo.Usage = spall::BufferUsageFlags::Uniform;
	uniformInfo.CpuAccess = spall::MemoryAccess::Write;
	uniformInfo.InitialState = spall::ResourceStateFlags::ConstantBuffer;

	spall::Resource<spall::IBuffer> uniformBuffer;
	REQUIRE(device.resources().createBuffer(uniformInfo, &uniformBuffer) == spall::SUCCESS);

	spall::Texture2DCreateInfo textureInfo = {};
	textureInfo.Width = 8;
	textureInfo.Height = 8;
	textureInfo.Format = spall::Format::RGBA8;
	textureInfo.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::TransferDestination;

	spall::Resource<spall::ITexture2D> texture;
	REQUIRE(device.resources().createTexture2D(textureInfo, &texture) == spall::SUCCESS);

	spall::TextureViewCreateInfo viewInfo = {};
	viewInfo.Texture = texture.get();

	spall::Resource<spall::ITextureView> view;
	REQUIRE(device.resources().createTextureView(viewInfo, &view) == spall::SUCCESS);

	const spall::ResourceBindingInfo bindings[] = {
		{0, spall::ResourceBindingType::UniformBuffer, spall::ShaderStageFlags::Vertex},
		{1, spall::ResourceBindingType::SampledTexture, spall::ShaderStageFlags::Fragment}};

	spall::ResourceSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.Bindings = bindings;

	spall::Resource<spall::IResourceSetLayout> layout;
	REQUIRE(device.pipelines().createResourceSetLayout(layoutInfo, &layout) == spall::SUCCESS);

	spall::ResourceWrite writes[2] = {};
	writes[0].Binding = 0;
	writes[0].Type = spall::ResourceBindingType::UniformBuffer;
	writes[0].Buffer = uniformBuffer.get();
	writes[1].Binding = 1;
	writes[1].Type = spall::ResourceBindingType::SampledTexture;
	writes[1].TextureView = view.get();
	writes[1].Sampler = sampler.get();

	spall::ResourceSetCreateInfo resourceSetInfo = {};
	resourceSetInfo.Layout = layout.get();
	resourceSetInfo.Writes = writes;

	spall::Resource<spall::IResourceSet> resourceSet;
	REQUIRE(device.pipelines().createResourceSet(resourceSetInfo, &resourceSet) == spall::SUCCESS);
	CHECK(&resourceSet->layout() == layout.get());

	spall::ResourceWrite mismatched = writes[0];
	mismatched.Type = spall::ResourceBindingType::StorageBuffer;

	CHECK(resourceSet->writeResources(std::span<const spall::ResourceWrite>(&mismatched, 1)) != spall::SUCCESS);
	CHECK(resourceSet->writeResources(writes) == spall::SUCCESS);
}

TEST_CASE(
	"A D3D12 graphics pipeline draws into a color target",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	static const char shaderSource[] =
		"struct VertexInput { float2 Position : ATTRIBUTE0; };\n"
		"struct VertexOutput { float4 Position : SV_Position; };\n"
		"VertexOutput vsMain(VertexInput input)\n"
		"{\n"
		"    VertexOutput output;\n"
		"    output.Position = float4(input.Position, 0.0f, 1.0f);\n"
		"    return output;\n"
		"}\n"
		"float4 psMain(VertexOutput input) : SV_Target\n"
		"{\n"
		"    return float4(0.25f, 0.5f, 0.75f, 1.0f);\n"
		"}\n";

	Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> fragmentBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;

	if (FAILED(D3DCompile(
			shaderSource,
			sizeof(shaderSource) - 1,
			nullptr,
			nullptr,
			nullptr,
			"vsMain",
			"vs_5_0",
			0,
			0,
			&vertexBlob,
			&errors)))
	{
		SKIP("The test vertex shader could not be compiled.");
	}

	if (FAILED(D3DCompile(
			shaderSource,
			sizeof(shaderSource) - 1,
			nullptr,
			nullptr,
			nullptr,
			"psMain",
			"ps_5_0",
			0,
			0,
			&fragmentBlob,
			&errors)))
	{
		SKIP("The test fragment shader could not be compiled.");
	}

	spall::ShaderCreateInfo vertexShaderInfo = {};
	vertexShaderInfo.Stage = spall::ShaderStage::Vertex;
	vertexShaderInfo.Bytecode = std::span<const std::byte>(
		static_cast<const std::byte*>(vertexBlob->GetBufferPointer()),
		vertexBlob->GetBufferSize());

	spall::Resource<spall::IShader> vertexShader;
	REQUIRE(device.pipelines().createShader(vertexShaderInfo, &vertexShader) == spall::SUCCESS);

	spall::ShaderCreateInfo fragmentShaderInfo = {};
	fragmentShaderInfo.Stage = spall::ShaderStage::Fragment;
	fragmentShaderInfo.Bytecode = std::span<const std::byte>(
		static_cast<const std::byte*>(fragmentBlob->GetBufferPointer()),
		fragmentBlob->GetBufferSize());

	spall::Resource<spall::IShader> fragmentShader;
	REQUIRE(device.pipelines().createShader(fragmentShaderInfo, &fragmentShader) == spall::SUCCESS);

	const float vertices[] = {-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};

	spall::BufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.Size = sizeof(vertices);
	vertexBufferInfo.Usage = spall::BufferUsageFlags::Vertex;
	vertexBufferInfo.InitialState = spall::ResourceStateFlags::VertexBuffer;

	spall::Resource<spall::IBuffer> vertexBuffer;
	REQUIRE(device.resources()
				.createBufferWithData(
					vertexBufferInfo,
					std::span<const std::byte>(reinterpret_cast<const std::byte*>(vertices), sizeof(vertices)),
					&vertexBuffer) == spall::SUCCESS);

	const spall::VertexBindingInfo vertexBindings[] = {{0, sizeof(float) * 2}};
	const spall::VertexAttributeInfo vertexAttributes[] = {{0, 0, spall::Format::RG32Float, 0}};

	constexpr std::uint32_t Extent = 64;

	spall::PipelineCreateInfo pipelineInfo = {};
	pipelineInfo.VertexShader.Module = vertexShader.get();
	pipelineInfo.VertexShader.Entry = "vsMain";
	pipelineInfo.FragmentShader.Module = fragmentShader.get();
	pipelineInfo.FragmentShader.Entry = "psMain";
	pipelineInfo.VertexBindings = vertexBindings;
	pipelineInfo.VertexAttributes = vertexAttributes;
	pipelineInfo.PrimitiveTopology = spall::PrimitiveTopology::TriangleList;
	pipelineInfo.ColorTargetFormats[0] = spall::Format::RGBA8;
	pipelineInfo.ColorTargetFormatCount = 1;

	spall::Resource<spall::IPipeline> pipeline;
	REQUIRE(device.pipelines().createPipeline(pipelineInfo, &pipeline) == spall::SUCCESS);

	spall::Texture2DCreateInfo textureInfo = {};
	textureInfo.Width = Extent;
	textureInfo.Height = Extent;
	textureInfo.Format = spall::Format::RGBA8;
	textureInfo.Usage = spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::TransferSource;

	spall::Resource<spall::ITexture2D> texture;
	REQUIRE(device.resources().createTexture2D(textureInfo, &texture) == spall::SUCCESS);

	spall::TextureViewCreateInfo viewInfo = {};
	viewInfo.Texture = texture.get();

	spall::Resource<spall::ITextureView> view;
	REQUIRE(device.resources().createTextureView(viewInfo, &view) == spall::SUCCESS);

	spall::FramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.ColorAttachments[0] = view.get();
	framebufferInfo.ColorAttachmentCount = 1;

	spall::Resource<spall::IFramebuffer> framebuffer;
	REQUIRE(device.resources().createFramebuffer(framebufferInfo, &framebuffer) == spall::SUCCESS);

	const std::uint32_t rowPitch = Extent * 4;

	spall::BufferCreateInfo readbackInfo = {};
	readbackInfo.Size = rowPitch * Extent;
	readbackInfo.Usage = spall::BufferUsageFlags::TransferDestination;
	readbackInfo.CpuAccess = spall::MemoryAccess::Read;
	readbackInfo.InitialState = spall::ResourceStateFlags::CopyDest;

	spall::Resource<spall::IBuffer> readback;
	REQUIRE(device.resources().createBuffer(readbackInfo, &readback) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	spall::RenderPassBeginInfo passInfo = {};
	passInfo.Framebuffer = framebuffer.get();
	passInfo.ColorAttachments[0].LoadAction = spall::LoadAction::Clear;
	passInfo.ColorAttachments[0].ClearColor = {1.0f, 0.0f, 0.0f, 1.0f};

	spall::Viewport viewport = {};
	viewport.Width = static_cast<float>(Extent);
	viewport.Height = static_cast<float>(Extent);

	spall::Scissor scissor = {};
	scissor.Width = Extent;
	scissor.Height = Extent;

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->beginRenderPass(passInfo) == spall::SUCCESS);
	REQUIRE(commands->bindGraphicsPipeline(*pipeline) == spall::SUCCESS);
	REQUIRE(commands->setViewport(viewport) == spall::SUCCESS);
	REQUIRE(commands->setScissor(scissor) == spall::SUCCESS);
	REQUIRE(commands->setVertexBuffer(0, *vertexBuffer, sizeof(float) * 2, 0) == spall::SUCCESS);
	REQUIRE(commands->draw(3, 0) == spall::SUCCESS);
	REQUIRE(commands->endRenderPass() == spall::SUCCESS);
	REQUIRE(commands->copyTextureToBuffer(*readback, 0, rowPitch, *texture, {}) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	std::vector<std::uint8_t> pixels(readbackInfo.Size);
	REQUIRE(device.resources()
				.readBuffer(
					*readback,
					std::span<std::byte>(reinterpret_cast<std::byte*>(pixels.data()), pixels.size()),
					0) == spall::SUCCESS);

	const std::uint8_t expected[4] = {64, 128, 191, 255};
	bool everyPixelMatches = true;

	for (std::uint32_t y = 0; y < Extent; ++y)
	{
		for (std::uint32_t x = 0; x < Extent; ++x)
		{
			const std::uint8_t* pixel = pixels.data() + (y * rowPitch) + (x * 4);

			for (std::uint32_t channel = 0; channel < 4; ++channel)
			{
				const int difference = static_cast<int>(pixel[channel]) - static_cast<int>(expected[channel]);

				if ((difference > 1) or (difference < -1))
				{
					everyPixelMatches = false;
				}
			}
		}
	}

	CHECK(everyPixelMatches);
}

TEST_CASE(
	"A D3D12 depth test rejects a farther indexed draw",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	static const char shaderSource[] =
		"cbuffer Push : register(b13) { float4 Color; float Depth; };\n"
		"struct VertexInput { float2 Position : ATTRIBUTE0; };\n"
		"struct VertexOutput { float4 Position : SV_Position; };\n"
		"VertexOutput vsMain(VertexInput input)\n"
		"{\n"
		"    VertexOutput output;\n"
		"    output.Position = float4(input.Position, Depth, 1.0f);\n"
		"    return output;\n"
		"}\n"
		"float4 psMain(VertexOutput input) : SV_Target\n"
		"{\n"
		"    return Color;\n"
		"}\n";

	Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> fragmentBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;

	if (FAILED(D3DCompile(shaderSource, sizeof(shaderSource) - 1, nullptr, nullptr, nullptr, "vsMain", "vs_5_0", 0, 0, &vertexBlob, &errors)) or
		FAILED(D3DCompile(shaderSource, sizeof(shaderSource) - 1, nullptr, nullptr, nullptr, "psMain", "ps_5_0", 0, 0, &fragmentBlob, &errors)))
	{
		SKIP("The depth-test shaders could not be compiled.");
	}

	spall::ShaderCreateInfo vertexShaderInfo = {};
	vertexShaderInfo.Stage = spall::ShaderStage::Vertex;
	vertexShaderInfo.Bytecode = std::span<const std::byte>(
		static_cast<const std::byte*>(vertexBlob->GetBufferPointer()),
		vertexBlob->GetBufferSize());

	spall::ShaderCreateInfo fragmentShaderInfo = {};
	fragmentShaderInfo.Stage = spall::ShaderStage::Fragment;
	fragmentShaderInfo.Bytecode = std::span<const std::byte>(
		static_cast<const std::byte*>(fragmentBlob->GetBufferPointer()),
		fragmentBlob->GetBufferSize());

	spall::Resource<spall::IShader> vertexShader;
	spall::Resource<spall::IShader> fragmentShader;
	REQUIRE(device.pipelines().createShader(vertexShaderInfo, &vertexShader) == spall::SUCCESS);
	REQUIRE(device.pipelines().createShader(fragmentShaderInfo, &fragmentShader) == spall::SUCCESS);

	const float vertices[] = {-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};
	const std::uint16_t indices[] = {0, 1, 2};

	spall::BufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.Size = sizeof(vertices);
	vertexBufferInfo.Usage = spall::BufferUsageFlags::Vertex;
	vertexBufferInfo.InitialState = spall::ResourceStateFlags::VertexBuffer;

	spall::Resource<spall::IBuffer> vertexBuffer;
	REQUIRE(device.resources()
				.createBufferWithData(
					vertexBufferInfo,
					std::span<const std::byte>(reinterpret_cast<const std::byte*>(vertices), sizeof(vertices)),
					&vertexBuffer) == spall::SUCCESS);

	spall::BufferCreateInfo indexBufferInfo = {};
	indexBufferInfo.Size = sizeof(indices);
	indexBufferInfo.Usage = spall::BufferUsageFlags::Index;
	indexBufferInfo.InitialState = spall::ResourceStateFlags::IndexBuffer;

	spall::Resource<spall::IBuffer> indexBuffer;
	REQUIRE(device.resources()
				.createBufferWithData(
					indexBufferInfo,
					std::span<const std::byte>(reinterpret_cast<const std::byte*>(indices), sizeof(indices)),
					&indexBuffer) == spall::SUCCESS);

	constexpr std::uint32_t Extent = 32;

	const spall::VertexBindingInfo vertexBindings[] = {{0, sizeof(float) * 2}};
	const spall::VertexAttributeInfo vertexAttributes[] = {{0, 0, spall::Format::RG32Float, 0}};

	spall::PipelineCreateInfo pipelineInfo = {};
	pipelineInfo.VertexShader = {vertexShader.get(), "vsMain"};
	pipelineInfo.FragmentShader = {fragmentShader.get(), "psMain"};
	pipelineInfo.VertexBindings = vertexBindings;
	pipelineInfo.VertexAttributes = vertexAttributes;
	pipelineInfo.PrimitiveTopology = spall::PrimitiveTopology::TriangleList;
	pipelineInfo.ColorTargetFormats[0] = spall::Format::RGBA8;
	pipelineInfo.ColorTargetFormatCount = 1;
	pipelineInfo.DepthStencilFormat = spall::Format::Depth32Float;
	pipelineInfo.EnableDepthTest = true;
	pipelineInfo.EnableDepthWrite = true;
	pipelineInfo.PushConstants = {spall::ShaderStageFlags::Vertex | spall::ShaderStageFlags::Fragment, 32};

	spall::Resource<spall::IPipeline> pipeline;
	REQUIRE(device.pipelines().createPipeline(pipelineInfo, &pipeline) == spall::SUCCESS);

	spall::Texture2DCreateInfo colorInfo = {};
	colorInfo.Width = Extent;
	colorInfo.Height = Extent;
	colorInfo.Format = spall::Format::RGBA8;
	colorInfo.Usage = spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::TransferSource;

	spall::Resource<spall::ITexture2D> colorTexture;
	REQUIRE(device.resources().createTexture2D(colorInfo, &colorTexture) == spall::SUCCESS);

	spall::Texture2DCreateInfo depthInfo = {};
	depthInfo.Width = Extent;
	depthInfo.Height = Extent;
	depthInfo.Format = spall::Format::Depth32Float;
	depthInfo.Usage = spall::TextureUsageFlags::DepthStencilAttachment;

	spall::Resource<spall::ITexture2D> depthTexture;
	REQUIRE(device.resources().createTexture2D(depthInfo, &depthTexture) == spall::SUCCESS);

	spall::TextureViewCreateInfo colorViewInfo = {};
	colorViewInfo.Texture = colorTexture.get();

	spall::TextureViewCreateInfo depthViewInfo = {};
	depthViewInfo.Texture = depthTexture.get();

	spall::Resource<spall::ITextureView> colorView;
	spall::Resource<spall::ITextureView> depthView;
	REQUIRE(device.resources().createTextureView(colorViewInfo, &colorView) == spall::SUCCESS);
	REQUIRE(device.resources().createTextureView(depthViewInfo, &depthView) == spall::SUCCESS);

	spall::FramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.ColorAttachments[0] = colorView.get();
	framebufferInfo.ColorAttachmentCount = 1;
	framebufferInfo.DepthAttachment = depthView.get();

	spall::Resource<spall::IFramebuffer> framebuffer;
	REQUIRE(device.resources().createFramebuffer(framebufferInfo, &framebuffer) == spall::SUCCESS);

	const std::uint32_t rowPitch = Extent * 4;

	spall::BufferCreateInfo readbackInfo = {};
	readbackInfo.Size = rowPitch * Extent;
	readbackInfo.Usage = spall::BufferUsageFlags::TransferDestination;
	readbackInfo.CpuAccess = spall::MemoryAccess::Read;
	readbackInfo.InitialState = spall::ResourceStateFlags::CopyDest;

	spall::Resource<spall::IBuffer> readback;
	REQUIRE(device.resources().createBuffer(readbackInfo, &readback) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	spall::RenderPassBeginInfo passInfo = {};
	passInfo.Framebuffer = framebuffer.get();
	passInfo.ColorAttachments[0].LoadAction = spall::LoadAction::Clear;
	passInfo.ColorAttachments[0].ClearColor = {0.0f, 0.0f, 1.0f, 1.0f};
	passInfo.DepthAttachment.DepthLoadAction = spall::LoadAction::Clear;
	passInfo.DepthAttachment.ClearDepth = 1.0f;

	spall::Viewport viewport = {};
	viewport.Width = static_cast<float>(Extent);
	viewport.Height = static_cast<float>(Extent);

	spall::Scissor scissor = {};
	scissor.Width = Extent;
	scissor.Height = Extent;

	struct PushBlock
	{
		float Color[4] = {};
		float Depth = 0.0f;
		float Padding[3] = {};
	};

	PushBlock nearBlock = {};
	nearBlock.Color[0] = 1.0f;
	nearBlock.Color[3] = 1.0f;
	nearBlock.Depth = 0.5f;

	PushBlock farBlock = {};
	farBlock.Color[1] = 1.0f;
	farBlock.Color[3] = 1.0f;
	farBlock.Depth = 0.9f;

	const spall::ShaderStageFlags pushStages = spall::ShaderStageFlags::Vertex | spall::ShaderStageFlags::Fragment;

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->beginRenderPass(passInfo) == spall::SUCCESS);
	REQUIRE(commands->bindGraphicsPipeline(*pipeline) == spall::SUCCESS);
	REQUIRE(commands->setViewport(viewport) == spall::SUCCESS);
	REQUIRE(commands->setScissor(scissor) == spall::SUCCESS);
	REQUIRE(commands->setVertexBuffer(0, *vertexBuffer, sizeof(float) * 2, 0) == spall::SUCCESS);
	REQUIRE(commands->setIndexBuffer(*indexBuffer, spall::IndexFormat::UInt16, 0) == spall::SUCCESS);
	REQUIRE(commands->setPushConstants(pushStages, 0, nearBlock) == spall::SUCCESS);
	REQUIRE(commands->drawIndexed(3, 0, 0) == spall::SUCCESS);
	REQUIRE(commands->setPushConstants(pushStages, 0, farBlock) == spall::SUCCESS);
	REQUIRE(commands->drawIndexed(3, 0, 0) == spall::SUCCESS);
	REQUIRE(commands->endRenderPass() == spall::SUCCESS);
	REQUIRE(commands->copyTextureToBuffer(*readback, 0, rowPitch, *colorTexture, {}) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	std::vector<std::uint8_t> pixels(readbackInfo.Size);
	REQUIRE(device.resources()
				.readBuffer(
					*readback,
					std::span<std::byte>(reinterpret_cast<std::byte*>(pixels.data()), pixels.size()),
					0) == spall::SUCCESS);

	const std::uint8_t expected[4] = {255, 0, 0, 255};
	bool nearestDrawSurvived = true;

	for (std::uint32_t y = 0; y < Extent; ++y)
	{
		for (std::uint32_t x = 0; x < Extent; ++x)
		{
			const std::uint8_t* pixel = pixels.data() + (y * rowPitch) + (x * 4);

			for (std::uint32_t channel = 0; channel < 4; ++channel)
			{
				if (pixel[channel] != expected[channel])
				{
					nearestDrawSurvived = false;
				}
			}
		}
	}

	CHECK(nearestDrawSurvived);
}

TEST_CASE(
	"A D3D12 indirect draw reads its arguments from a buffer",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	static const char shaderSource[] =
		"struct VertexInput { float2 Position : ATTRIBUTE0; };\n"
		"struct VertexOutput { float4 Position : SV_Position; };\n"
		"VertexOutput vsMain(VertexInput input)\n"
		"{\n"
		"    VertexOutput output;\n"
		"    output.Position = float4(input.Position, 0.0f, 1.0f);\n"
		"    return output;\n"
		"}\n"
		"float4 psMain(VertexOutput input) : SV_Target\n"
		"{\n"
		"    return float4(0.0f, 1.0f, 0.0f, 1.0f);\n"
		"}\n";

	Microsoft::WRL::ComPtr<ID3DBlob> vertexBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> fragmentBlob;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;

	if (FAILED(D3DCompile(shaderSource, sizeof(shaderSource) - 1, nullptr, nullptr, nullptr, "vsMain", "vs_5_0", 0, 0, &vertexBlob, &errors)) or
		FAILED(D3DCompile(shaderSource, sizeof(shaderSource) - 1, nullptr, nullptr, nullptr, "psMain", "ps_5_0", 0, 0, &fragmentBlob, &errors)))
	{
		SKIP("The indirect-draw shaders could not be compiled.");
	}

	spall::ShaderCreateInfo vertexShaderInfo = {};
	vertexShaderInfo.Stage = spall::ShaderStage::Vertex;
	vertexShaderInfo.Bytecode = std::span<const std::byte>(
		static_cast<const std::byte*>(vertexBlob->GetBufferPointer()),
		vertexBlob->GetBufferSize());

	spall::ShaderCreateInfo fragmentShaderInfo = {};
	fragmentShaderInfo.Stage = spall::ShaderStage::Fragment;
	fragmentShaderInfo.Bytecode = std::span<const std::byte>(
		static_cast<const std::byte*>(fragmentBlob->GetBufferPointer()),
		fragmentBlob->GetBufferSize());

	spall::Resource<spall::IShader> vertexShader;
	spall::Resource<spall::IShader> fragmentShader;
	REQUIRE(device.pipelines().createShader(vertexShaderInfo, &vertexShader) == spall::SUCCESS);
	REQUIRE(device.pipelines().createShader(fragmentShaderInfo, &fragmentShader) == spall::SUCCESS);

	const float vertices[] = {-1.0f, -1.0f, 3.0f, -1.0f, -1.0f, 3.0f};

	spall::BufferCreateInfo vertexBufferInfo = {};
	vertexBufferInfo.Size = sizeof(vertices);
	vertexBufferInfo.Usage = spall::BufferUsageFlags::Vertex;
	vertexBufferInfo.InitialState = spall::ResourceStateFlags::VertexBuffer;

	spall::Resource<spall::IBuffer> vertexBuffer;
	REQUIRE(device.resources()
				.createBufferWithData(
					vertexBufferInfo,
					std::span<const std::byte>(reinterpret_cast<const std::byte*>(vertices), sizeof(vertices)),
					&vertexBuffer) == spall::SUCCESS);

	spall::DrawIndirectCommand arguments = {};
	arguments.VertexCount = 3;
	arguments.InstanceCount = 1;

	spall::BufferCreateInfo argumentBufferInfo = {};
	argumentBufferInfo.Size = sizeof(arguments);
	argumentBufferInfo.Usage = spall::BufferUsageFlags::Indirect;
	argumentBufferInfo.InitialState = spall::ResourceStateFlags::IndirectArgument;

	spall::Resource<spall::IBuffer> argumentBuffer;
	REQUIRE(device.resources()
				.createBufferWithData(
					argumentBufferInfo,
					std::span<const std::byte>(reinterpret_cast<const std::byte*>(&arguments), sizeof(arguments)),
					&argumentBuffer) == spall::SUCCESS);

	constexpr std::uint32_t Extent = 32;

	const spall::VertexBindingInfo vertexBindings[] = {{0, sizeof(float) * 2}};
	const spall::VertexAttributeInfo vertexAttributes[] = {{0, 0, spall::Format::RG32Float, 0}};

	spall::PipelineCreateInfo pipelineInfo = {};
	pipelineInfo.VertexShader = {vertexShader.get(), "vsMain"};
	pipelineInfo.FragmentShader = {fragmentShader.get(), "psMain"};
	pipelineInfo.VertexBindings = vertexBindings;
	pipelineInfo.VertexAttributes = vertexAttributes;
	pipelineInfo.PrimitiveTopology = spall::PrimitiveTopology::TriangleList;
	pipelineInfo.ColorTargetFormats[0] = spall::Format::RGBA8;
	pipelineInfo.ColorTargetFormatCount = 1;

	spall::Resource<spall::IPipeline> pipeline;
	REQUIRE(device.pipelines().createPipeline(pipelineInfo, &pipeline) == spall::SUCCESS);

	spall::Texture2DCreateInfo colorInfo = {};
	colorInfo.Width = Extent;
	colorInfo.Height = Extent;
	colorInfo.Format = spall::Format::RGBA8;
	colorInfo.Usage = spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::TransferSource;

	spall::Resource<spall::ITexture2D> colorTexture;
	REQUIRE(device.resources().createTexture2D(colorInfo, &colorTexture) == spall::SUCCESS);

	spall::TextureViewCreateInfo colorViewInfo = {};
	colorViewInfo.Texture = colorTexture.get();

	spall::Resource<spall::ITextureView> colorView;
	REQUIRE(device.resources().createTextureView(colorViewInfo, &colorView) == spall::SUCCESS);

	spall::FramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.ColorAttachments[0] = colorView.get();
	framebufferInfo.ColorAttachmentCount = 1;

	spall::Resource<spall::IFramebuffer> framebuffer;
	REQUIRE(device.resources().createFramebuffer(framebufferInfo, &framebuffer) == spall::SUCCESS);

	const std::uint32_t rowPitch = Extent * 4;

	spall::BufferCreateInfo readbackInfo = {};
	readbackInfo.Size = rowPitch * Extent;
	readbackInfo.Usage = spall::BufferUsageFlags::TransferDestination;
	readbackInfo.CpuAccess = spall::MemoryAccess::Read;
	readbackInfo.InitialState = spall::ResourceStateFlags::CopyDest;

	spall::Resource<spall::IBuffer> readback;
	REQUIRE(device.resources().createBuffer(readbackInfo, &readback) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	spall::RenderPassBeginInfo passInfo = {};
	passInfo.Framebuffer = framebuffer.get();
	passInfo.ColorAttachments[0].LoadAction = spall::LoadAction::Clear;
	passInfo.ColorAttachments[0].ClearColor = {1.0f, 0.0f, 0.0f, 1.0f};

	spall::Viewport viewport = {};
	viewport.Width = static_cast<float>(Extent);
	viewport.Height = static_cast<float>(Extent);

	spall::Scissor scissor = {};
	scissor.Width = Extent;
	scissor.Height = Extent;

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->beginRenderPass(passInfo) == spall::SUCCESS);
	REQUIRE(commands->bindGraphicsPipeline(*pipeline) == spall::SUCCESS);
	REQUIRE(commands->setViewport(viewport) == spall::SUCCESS);
	REQUIRE(commands->setScissor(scissor) == spall::SUCCESS);
	REQUIRE(commands->setVertexBuffer(0, *vertexBuffer, sizeof(float) * 2, 0) == spall::SUCCESS);
	REQUIRE(commands->drawIndirect(*argumentBuffer, 0) == spall::SUCCESS);
	REQUIRE(commands->endRenderPass() == spall::SUCCESS);
	REQUIRE(commands->copyTextureToBuffer(*readback, 0, rowPitch, *colorTexture, {}) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	std::vector<std::uint8_t> pixels(readbackInfo.Size);
	REQUIRE(device.resources()
				.readBuffer(
					*readback,
					std::span<std::byte>(reinterpret_cast<std::byte*>(pixels.data()), pixels.size()),
					0) == spall::SUCCESS);

	const std::uint8_t expected[4] = {0, 255, 0, 255};
	bool indirectDrawRan = true;

	for (std::uint32_t y = 0; y < Extent; ++y)
	{
		for (std::uint32_t x = 0; x < Extent; ++x)
		{
			const std::uint8_t* pixel = pixels.data() + (y * rowPitch) + (x * 4);

			for (std::uint32_t channel = 0; channel < 4; ++channel)
			{
				if (pixel[channel] != expected[channel])
				{
					indirectDrawRan = false;
				}
			}
		}
	}

	CHECK(indirectDrawRan);
}

TEST_CASE(
	"A D3D12 texture tracks mip levels independently",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	constexpr std::uint32_t Extent = 8;
	constexpr std::uint32_t MipLevels = 2;

	spall::Texture2DCreateInfo textureInfo = {};
	textureInfo.Width = Extent;
	textureInfo.Height = Extent;
	textureInfo.MipLevels = MipLevels;
	textureInfo.Format = spall::Format::RGBA8;
	textureInfo.Usage =
		spall::TextureUsageFlags::Sampled |
		spall::TextureUsageFlags::TransferSource |
		spall::TextureUsageFlags::TransferDestination;

	spall::Resource<spall::ITexture2D> texture;
	REQUIRE(device.resources().createTexture2D(textureInfo, &texture) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	const spall::TextureSubresourceRange baseMip = {0, 1, 0, 1};
	const spall::TextureSubresourceRange secondMip = {1, 1, 0, 1};

	REQUIRE(commands->begin() == spall::SUCCESS);

	REQUIRE(commands->setTextureState(*texture, spall::ResourceStateFlags::ShaderResource, secondMip) == spall::SUCCESS);

	CHECK(commands->textureState(*texture, secondMip) == spall::ResourceStateFlags::ShaderResource);
	CHECK(commands->textureState(*texture, baseMip) == spall::ResourceStateFlags::Common);
	CHECK(commands->textureState(*texture, {}) == spall::ResourceStateFlags::Unknown);

	REQUIRE(commands->setTextureState(*texture, spall::ResourceStateFlags::ShaderResource, baseMip) == spall::SUCCESS);

	CHECK(commands->textureState(*texture, {}) == spall::ResourceStateFlags::ShaderResource);

	REQUIRE(commands->commitBarriers() == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);
}

TEST_CASE(
	"A D3D12 generateMips fills the levels below the base",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	constexpr std::uint32_t Extent = 16;
	constexpr std::uint32_t MipLevels = 5;

	spall::Texture2DCreateInfo textureInfo = {};
	textureInfo.Width = Extent;
	textureInfo.Height = Extent;
	textureInfo.MipLevels = MipLevels;
	textureInfo.Format = spall::Format::RGBA8;
	textureInfo.Usage =
		spall::TextureUsageFlags::Sampled |
		spall::TextureUsageFlags::TransferSource |
		spall::TextureUsageFlags::TransferDestination;

	spall::Resource<spall::ITexture2D> texture;
	REQUIRE(device.resources().createTexture2D(textureInfo, &texture) == spall::SUCCESS);

	const std::uint32_t basePitch = Extent * 4;
	std::vector<std::uint8_t> baseLevel(basePitch * Extent);

	for (std::uint32_t y = 0; y < Extent; ++y)
	{
		for (std::uint32_t x = 0; x < Extent; ++x)
		{
			const std::uint8_t level = (x < (Extent / 2)) ? 0 : 255;
			std::uint8_t* pixel = baseLevel.data() + (y * basePitch) + (x * 4);

			pixel[0] = level;
			pixel[1] = level;
			pixel[2] = level;
			pixel[3] = 255;
		}
	}

	spall::BufferCreateInfo uploadInfo = {};
	uploadInfo.Size = static_cast<std::uint32_t>(baseLevel.size());
	uploadInfo.Usage = spall::BufferUsageFlags::TransferSource;
	uploadInfo.CpuAccess = spall::MemoryAccess::Write;
	uploadInfo.InitialState = spall::ResourceStateFlags::CopySource;

	spall::Resource<spall::IBuffer> upload;
	REQUIRE(device.resources()
				.createBufferWithData(
					uploadInfo,
					std::span<const std::byte>(reinterpret_cast<const std::byte*>(baseLevel.data()), baseLevel.size()),
					&upload) == spall::SUCCESS);

	const std::uint32_t smallestExtent = mipExtent(Extent, MipLevels - 1);
	const std::uint32_t smallestPitch = smallestExtent * 4;

	spall::BufferCreateInfo readbackInfo = {};
	readbackInfo.Size = smallestPitch * smallestExtent;
	readbackInfo.Usage = spall::BufferUsageFlags::TransferDestination;
	readbackInfo.CpuAccess = spall::MemoryAccess::Read;
	readbackInfo.InitialState = spall::ResourceStateFlags::CopyDest;

	spall::Resource<spall::IBuffer> readback;
	REQUIRE(device.resources().createBuffer(readbackInfo, &readback) == spall::SUCCESS);

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	spall::TextureRegion baseRegion = {};

	spall::TextureRegion smallestRegion = {};
	smallestRegion.MipLevel = MipLevels - 1;

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->copyBufferToTexture(*texture, baseRegion, *upload, 0, basePitch) == spall::SUCCESS);
	REQUIRE(commands->generateMips(*texture) == spall::SUCCESS);
	REQUIRE(commands->copyTextureToBuffer(*readback, 0, smallestPitch, *texture, smallestRegion) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	std::vector<std::uint8_t> smallest(readbackInfo.Size);
	REQUIRE(device.resources()
				.readBuffer(
					*readback,
					std::span<std::byte>(reinterpret_cast<std::byte*>(smallest.data()), smallest.size()),
					0) == spall::SUCCESS);

	REQUIRE(smallestExtent == 1);

	const std::uint8_t* smallestPixel = smallest.data();

	CHECK(smallestPixel[3] == 255);
	CHECK(smallestPixel[0] > 118);
	CHECK(smallestPixel[0] < 138);
	CHECK(smallestPixel[1] == smallestPixel[0]);
	CHECK(smallestPixel[2] == smallestPixel[0]);
}

TEST_CASE(
	"A D3D12 query pool reports elapsed GPU time",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	if (not device.limits().SupportsTimestampQueries)
	{
		SKIP("The device does not support timestamp queries.");
	}

	spall::QueryPoolCreateInfo poolInfo = {};
	poolInfo.TimestampCount = 2;

	spall::Resource<spall::IQueryPool> queryPool;
	REQUIRE(device.resources().createQueryPool(poolInfo, &queryPool) == spall::SUCCESS);
	CHECK(queryPool->info().TimestampCount == 2);

	std::uint64_t nanoseconds[2] = {};

	CHECK(device.resources().readTimestamps(*queryPool, 0, nanoseconds) == spall::ERR_NOT_READY);

	constexpr std::uint32_t Extent = 256;

	spall::Texture2DCreateInfo textureInfo = {};
	textureInfo.Width = Extent;
	textureInfo.Height = Extent;
	textureInfo.Format = spall::Format::RGBA8;
	textureInfo.Usage = spall::TextureUsageFlags::ColorAttachment;

	spall::Resource<spall::ITexture2D> texture;
	REQUIRE(device.resources().createTexture2D(textureInfo, &texture) == spall::SUCCESS);

	spall::TextureViewCreateInfo viewInfo = {};
	viewInfo.Texture = texture.get();

	spall::Resource<spall::ITextureView> view;
	REQUIRE(device.resources().createTextureView(viewInfo, &view) == spall::SUCCESS);

	spall::FramebufferCreateInfo framebufferInfo = {};
	framebufferInfo.ColorAttachments[0] = view.get();
	framebufferInfo.ColorAttachmentCount = 1;

	spall::Resource<spall::IFramebuffer> framebuffer;
	REQUIRE(device.resources().createFramebuffer(framebufferInfo, &framebuffer) == spall::SUCCESS);

	spall::RenderPassBeginInfo passInfo = {};
	passInfo.Framebuffer = framebuffer.get();
	passInfo.ColorAttachments[0].LoadAction = spall::LoadAction::Clear;

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->writeTimestamp(*queryPool, 0) == spall::SUCCESS);

	for (std::uint32_t pass = 0; pass < 32; ++pass)
	{
		REQUIRE(commands->beginRenderPass(passInfo) == spall::SUCCESS);
		REQUIRE(commands->endRenderPass() == spall::SUCCESS);
	}

	REQUIRE(commands->writeTimestamp(*queryPool, 1) == spall::SUCCESS);
	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	REQUIRE(device.resources().readTimestamps(*queryPool, 0, nanoseconds) == spall::SUCCESS);

	CHECK(nanoseconds[0] > 0);
	CHECK(nanoseconds[1] > nanoseconds[0]);
	CHECK((nanoseconds[1] - nanoseconds[0]) < 1000000000ull);

	CHECK(device.resources().readTimestamps(*queryPool, 5, nanoseconds) != spall::SUCCESS);
}

TEST_CASE(
	"A D3D12 swap chain presents and resizes",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	const HiddenWindow window;

	if (window.handle() == nullptr)
	{
		SKIP("The test window could not be created.");
	}

	spall::SwapChainCreateInfo swapChainInfo = {};
	swapChainInfo.Window.Type = spall::WindowHandleType::Win32;
	swapChainInfo.Window.Value = window.handle();
	swapChainInfo.Width = 256;
	swapChainInfo.Height = 256;
	swapChainInfo.Format = spall::Format::BGRA8Srgb;

	spall::Resource<spall::ISwapChain> swapChain;
	REQUIRE(device.presentation().createSwapChain(swapChainInfo, &swapChain) == spall::SUCCESS);

	std::vector<spall::Resource<spall::ICommandList>> commandLists(swapChain->frameCount());

	for (spall::Resource<spall::ICommandList>& commandList : commandLists)
	{
		REQUIRE(device.createCommandList(&commandList) == spall::SUCCESS);
	}

	for (std::uint32_t frameIndex = 0; frameIndex < 8; ++frameIndex)
	{
		spall::ICommandList& commands = *commandLists[frameIndex % commandLists.size()];

		spall::Resource<spall::IFrame> frame;
		REQUIRE(device.graphicsQueue().acquireFrame(*swapChain, &frame) == spall::SUCCESS);

		spall::FramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.ColorAttachments[0] = &frame->presentTextureView();
		framebufferInfo.ColorAttachmentCount = 1;

		spall::Resource<spall::IFramebuffer> framebuffer;
		REQUIRE(device.resources().createFramebuffer(framebufferInfo, &framebuffer) == spall::SUCCESS);

		spall::RenderPassBeginInfo passInfo = {};
		passInfo.Framebuffer = framebuffer.get();
		passInfo.ColorAttachments[0].LoadAction = spall::LoadAction::Clear;
		passInfo.ColorAttachments[0].ClearColor = {0.1f * static_cast<float>(frameIndex), 0.2f, 0.3f, 1.0f};

		REQUIRE(commands.begin() == spall::SUCCESS);
		REQUIRE(commands.beginRenderPass(passInfo) == spall::SUCCESS);
		REQUIRE(commands.endRenderPass() == spall::SUCCESS);
		REQUIRE(commands.end() == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().submit(commands) == spall::SUCCESS);
		REQUIRE(device.graphicsQueue().present(*frame) == spall::SUCCESS);
	}

	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);

	commandLists.clear();

	CHECK(swapChain->resize(320, 240) == spall::SUCCESS);
}

TEST_CASE(
	"A D3D12 command list records nested debug groups",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->pushDebugGroup("Frame", {1.0f, 0.0f, 0.0f, 1.0f}) == spall::SUCCESS);
	REQUIRE(commands->pushDebugGroup("Shadows", {0.0f, 1.0f, 0.0f, 1.0f}) == spall::SUCCESS);
	REQUIRE(commands->insertDebugMarker("Cascade 0", {0.0f, 0.0f, 1.0f, 1.0f}) == spall::SUCCESS);
	REQUIRE(commands->popDebugGroup() == spall::SUCCESS);
	REQUIRE(commands->popDebugGroup() == spall::SUCCESS);

	CHECK(commands->popDebugGroup() != spall::SUCCESS);

	REQUIRE(commands->end() == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().submit(*commands) == spall::SUCCESS);
	REQUIRE(device.graphicsQueue().waitIdle() == spall::SUCCESS);
}

TEST_CASE(
	"A D3D12 command list refuses to end with an open debug group",
	"[d3d12][GPU]")
{
	const TestDevice testDevice = requireDevice(spall::RenderBackendType::D3D12);
	spall::IDevice& device = *testDevice.Device;

	spall::Resource<spall::ICommandList> commands;
	REQUIRE(device.createCommandList(&commands) == spall::SUCCESS);

	REQUIRE(commands->begin() == spall::SUCCESS);
	REQUIRE(commands->pushDebugGroup("Unclosed", {}) == spall::SUCCESS);

	CHECK(commands->end() != spall::SUCCESS);
}
