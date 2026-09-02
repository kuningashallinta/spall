// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include "Shaders.h"

#include <spall/Backend/BackendFactory.h>
#include <spall/CommandList/ICommandList.h>
#include <spall/Device/IDevice.h>
#include <spall/Frame/IFrame.h>
#include <spall/Framebuffer/IFramebuffer.h>
#include <spall/Pipeline/Binding/IResourceSet.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <spall/RenderPass/RenderPassBeginInfo.h>
#include <spall/Resources/Sampler/ISampler.h>
#include <spall/Resources/Texture/ITexture2D.h>
#include <spall/Resources/TextureView/ITextureView.h>
#include <spall/SwapChain/ISwapChain.h>

#include <windows.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <span>
#include <vector>

static constexpr std::uint32_t WindowWidth = 800;
static constexpr std::uint32_t WindowHeight = 600;

static constexpr std::uint32_t ImageWidth = 512;
static constexpr std::uint32_t ImageHeight = 512;
static constexpr std::uint32_t ComputeGroupSize = 8;

static constexpr spall::RenderBackendType BackendType = spall::RenderBackendType::Vulkan;

static std::uint32_t groupCount(
	std::uint32_t extent)
{
	return (extent + ComputeGroupSize - 1) / ComputeGroupSize;
}

static LRESULT CALLBACK windowProcedure(
	HWND window,
	UINT message,
	WPARAM wordParameter,
	LPARAM longParameter)
{
	if (message == WM_DESTROY)
	{
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProcW(window, message, wordParameter, longParameter);
}

int WINAPI wWinMain(
	HINSTANCE instance,
	HINSTANCE,
	PWSTR,
	int showCommand)
{
	WNDCLASSW windowClass = {};
	windowClass.lpfnWndProc = windowProcedure;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = L"SpallComputeImage";
	RegisterClassW(&windowClass);

	constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;

	RECT bounds = {0, 0, WindowWidth, WindowHeight};
	AdjustWindowRect(&bounds, windowStyle, FALSE);

	HWND window = CreateWindowExW(
		0,
		windowClass.lpszClassName,
		L"Compute Image",
		windowStyle,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		bounds.right - bounds.left,
		bounds.bottom - bounds.top,
		nullptr,
		nullptr,
		instance,
		nullptr);

	ShowWindow(window, showCommand);

	const std::unique_ptr<spall::IBackend> backend = spall::createBackend(BackendType);

	if (backend == nullptr)
	{
		return EXIT_FAILURE;
	}

	const spall::Resource<spall::IDevice> device = backend->createDevice();

	if (not device)
	{
		return EXIT_FAILURE;
	}

	const spall::Resource<spall::ISwapChain> swapChain = device->presentation().createSwapChain({
		.Window = {.Value = window},
		.Width = WindowWidth,
		.Height = WindowHeight,
		.Format = spall::Format::BGRA8Srgb});

	if (not swapChain)
	{
		return EXIT_FAILURE;
	}

	spall::Resource<spall::IShader> computeShader;
	spall::Resource<spall::IShader> vertexShader;
	spall::Resource<spall::IShader> fragmentShader;

	const char* computeEntryPoint = nullptr;
	const char* vertexEntryPoint = nullptr;
	const char* fragmentEntryPoint = nullptr;

	switch (BackendType)
	{
		case spall::RenderBackendType::D3D12:
		{
			computeShader = device->pipelines().createShader(spall::ShaderStage::Compute, spall::HlslCompute);
			vertexShader = device->pipelines().createShader(spall::ShaderStage::Vertex, spall::HlslVertex);
			fragmentShader = device->pipelines().createShader(spall::ShaderStage::Fragment, spall::HlslFragment);

			computeEntryPoint = "csMain";
			vertexEntryPoint = "vsMain";
			fragmentEntryPoint = "psMain";
			break;
		}

		case spall::RenderBackendType::Vulkan:
		{
			computeShader = device->pipelines().createShader(spall::ShaderStage::Compute, spall::VulkanCompute);
			vertexShader = device->pipelines().createShader(spall::ShaderStage::Vertex, spall::VulkanVertex);
			fragmentShader = device->pipelines().createShader(spall::ShaderStage::Fragment, spall::VulkanFragment);

			computeEntryPoint = "main";
			vertexEntryPoint = "main";
			fragmentEntryPoint = "main";
			break;
		}
	}

	const spall::Resource<spall::ITexture2D> texture = device->resources().createTexture2D({.Width = ImageWidth,
		.Height = ImageHeight,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Storage | spall::TextureUsageFlags::Sampled});

	const spall::Resource<spall::ITextureView> textureView = device->resources().createTextureView({.Texture = texture.get(),
		.Aspects = spall::TextureAspectFlags::Color});

	const spall::Resource<spall::ISampler> sampler = device->resources().createSampler({});

	const spall::ResourceBindingInfo computeBinding = {0, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::Compute};
	const spall::Resource<spall::IResourceSetLayout> computeLayout = device->pipelines().createResourceSetLayout({std::span {&computeBinding, 1}});

	const spall::ResourceBindingInfo sampleBinding = {0, spall::ResourceBindingType::SampledTexture, spall::ShaderStageFlags::Fragment};
	const spall::Resource<spall::IResourceSetLayout> sampleLayout = device->pipelines().createResourceSetLayout({std::span {&sampleBinding, 1}});

	const spall::ResourceWrite computeWrite = {
		.Binding = 0,
		.Type = spall::ResourceBindingType::StorageTexture,
		.TextureView = textureView.get()};

	const spall::Resource<spall::IResourceSet> computeSet = device->pipelines().createResourceSet(
		{computeLayout.get(), std::span {&computeWrite, 1}});

	const spall::ResourceWrite sampleWrite = {
		.Binding = 0,
		.Type = spall::ResourceBindingType::SampledTexture,
		.TextureView = textureView.get(),
		.Sampler = sampler.get()};

	const spall::Resource<spall::IResourceSet> sampleSet = device->pipelines().createResourceSet(
		{sampleLayout.get(), std::span {&sampleWrite, 1}});

	const spall::IResourceSetLayout* const computeLayouts[] = {computeLayout.get()};

	const spall::Resource<spall::IPipeline> computePipeline = device->pipelines().createComputePipeline({.ComputeShader = {computeShader.get(), computeEntryPoint},
		.ResourceSetLayouts = computeLayouts,
		.PushConstants = {spall::ShaderStageFlags::Compute, sizeof(float)}});

	const spall::IResourceSetLayout* const sampleLayouts[] = {sampleLayout.get()};

	const spall::Resource<spall::IPipeline> pipeline = device->pipelines().createPipeline({.VertexShader = {vertexShader.get(), vertexEntryPoint},
		.FragmentShader = {fragmentShader.get(), fragmentEntryPoint},
		.ResourceSetLayouts = sampleLayouts,
		.PrimitiveTopology = spall::PrimitiveTopology::TriangleList,
		.ColorTargetFormats = {swapChain->format()},
		.ColorTargetFormatCount = 1});

	std::vector<spall::Resource<spall::IFramebuffer>> framebuffers(swapChain->frameCount());
	std::vector<spall::Resource<spall::ICommandList>> commandLists(swapChain->frameCount());

	spall::RenderPassBeginInfo renderPass = {};
	renderPass.ColorAttachments[0].LoadAction = spall::LoadAction::Clear;
	renderPass.ColorAttachments[0].ClearColor = {0.02f, 0.03f, 0.05f, 1.0f};

	const spall::Viewport viewport = {
		.Width = static_cast<float>(WindowWidth),
		.Height = static_cast<float>(WindowHeight)};

	const spall::Scissor scissor = {.Width = WindowWidth, .Height = WindowHeight};

	const spall::Resource<spall::ICommandList> compute = device->createCommandList();
	const auto start = std::chrono::steady_clock::now();
	MSG message = {};

	while (message.message != WM_QUIT)
	{
		if (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessageW(&message);
			continue;
		}

		const float time = std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count();

		compute->begin();
		compute->bindComputePipeline(*computePipeline);
		compute->bindResourceSet(0, *computeSet);
		compute->setPushConstants(spall::ShaderStageFlags::Compute, 0, time);
		compute->dispatch(groupCount(ImageWidth), groupCount(ImageHeight), 1);
		compute->end();

		device->graphicsQueue().submit(*compute);

		const spall::Resource<spall::IFrame> frame = device->graphicsQueue().acquireFrame(*swapChain);
		const std::uint32_t frameIndex = frame->index();

		if (not framebuffers[frameIndex])
		{
			framebuffers[frameIndex] = device->resources().createFramebuffer({
				.ColorAttachments = {&frame->presentTextureView()},
				.ColorAttachmentCount = 1});
		}

		if (not commandLists[frameIndex])
		{
			commandLists[frameIndex] = device->createCommandList();
		}

		spall::ICommandList& commands = *commandLists[frameIndex];
		renderPass.Framebuffer = framebuffers[frameIndex].get();

		commands.begin();
		commands.beginRenderPass(renderPass);
		commands.setViewport(viewport);
		commands.setScissor(scissor);
		commands.bindGraphicsPipeline(*pipeline);
		commands.bindResourceSet(0, *sampleSet);
		commands.draw(3, 0);
		commands.endRenderPass();
		commands.end();

		device->graphicsQueue().submit(commands);
		device->graphicsQueue().present(*frame);

		device->graphicsQueue().waitIdle();
	}

	device->graphicsQueue().waitIdle();

	return EXIT_SUCCESS;
}
