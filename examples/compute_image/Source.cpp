#include "Shaders.h"

#include <spall/CommandList/ICommandList.h>
#include <spall/Device/IDevice.h>
#include <spall/Pipeline/Binding/IResourceSet.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <spall/Queue/IGraphicsQueue.h>
#include <spall/Resources/Sampler/ISampler.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/TextureView/ITextureView.h>
#include <spall/Runtime/Renderer.h>

#include <windows.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <span>

namespace
{
	constexpr std::uint32_t WindowWidth = 800;
	constexpr std::uint32_t WindowHeight = 600;

	constexpr std::uint32_t ImageWidth = 512;
	constexpr std::uint32_t ImageHeight = 512;
	constexpr std::uint32_t ComputeGroupSize = 8;

	constexpr spall::RenderBackendType BackendType = spall::RenderBackendType::Vulkan;

	std::uint32_t groupCount(
		std::uint32_t extent)
	{
		return (extent + ComputeGroupSize - 1) / ComputeGroupSize;
	}

	LRESULT CALLBACK windowProcedure(
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
} // namespace

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

	spall::Renderer renderer = spall::Renderer::create({
		.Backend = BackendType,
		.SwapChain = {
			.Window = {.Value = window},
			.Width = WindowWidth,
			.Height = WindowHeight,
			.Format = spall::Format::BGRA8Srgb}});

	if (not renderer)
	{
		return EXIT_FAILURE;
	}

	spall::IDevice& device = renderer.device();

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
			computeShader = device.pipelines().createShader(spall::ShaderStage::Compute, shaders::HlslCompute);
			vertexShader = device.pipelines().createShader(spall::ShaderStage::Vertex, shaders::HlslVertex);
			fragmentShader = device.pipelines().createShader(spall::ShaderStage::Fragment, shaders::HlslFragment);

			computeEntryPoint = "csMain";
			vertexEntryPoint = "vsMain";
			fragmentEntryPoint = "psMain";
			break;
		}

		case spall::RenderBackendType::Vulkan:
		{
			computeShader = device.pipelines().createShader(spall::ShaderStage::Compute, shaders::VulkanCompute);
			vertexShader = device.pipelines().createShader(spall::ShaderStage::Vertex, shaders::VulkanVertex);
			fragmentShader = device.pipelines().createShader(spall::ShaderStage::Fragment, shaders::VulkanFragment);

			computeEntryPoint = "main";
			vertexEntryPoint = "main";
			fragmentEntryPoint = "main";
			break;
		}
	}

	const spall::Resource<spall::ITexture> texture = device.resources().createTexture({
		.Width = ImageWidth,
		.Height = ImageHeight,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Storage | spall::TextureUsageFlags::Sampled});

	const spall::Resource<spall::ITextureView> textureView = device.resources().createTextureView({
		.Texture = texture.get(),
		.Aspects = spall::TextureAspectFlags::Color});

	const spall::Resource<spall::ISampler> sampler = device.resources().createSampler({});

	const spall::ResourceBindingInfo computeBinding = {0, spall::ResourceBindingType::StorageTexture, spall::ShaderStageFlags::Compute};
	const spall::Resource<spall::IResourceSetLayout> computeLayout = device.pipelines().createResourceSetLayout({std::span {&computeBinding, 1}});

	const spall::ResourceBindingInfo sampleBinding = {0, spall::ResourceBindingType::SampledTexture, spall::ShaderStageFlags::Fragment};
	const spall::Resource<spall::IResourceSetLayout> sampleLayout = device.pipelines().createResourceSetLayout({std::span {&sampleBinding, 1}});

	const spall::ResourceWrite computeWrite = {
		.Binding = 0,
		.Type = spall::ResourceBindingType::StorageTexture,
		.TextureView = textureView.get()};

	const spall::Resource<spall::IResourceSet> computeSet = device.pipelines().createResourceSet(
		{computeLayout.get(), std::span {&computeWrite, 1}});

	const spall::ResourceWrite sampleWrite = {
		.Binding = 0,
		.Type = spall::ResourceBindingType::SampledTexture,
		.TextureView = textureView.get(),
		.Sampler = sampler.get()};

	const spall::Resource<spall::IResourceSet> sampleSet = device.pipelines().createResourceSet(
		{sampleLayout.get(), std::span {&sampleWrite, 1}});

	const spall::IResourceSetLayout* const computeLayouts[] = {computeLayout.get()};

	const spall::Resource<spall::IPipeline> computePipeline = device.pipelines().createComputePipeline({
		.ComputeShader = {computeShader.get(), computeEntryPoint},
		.ResourceSetLayouts = computeLayouts,
		.PushConstants = {spall::ShaderStageFlags::Compute, sizeof(float)}});

	const spall::IResourceSetLayout* const sampleLayouts[] = {sampleLayout.get()};

	const spall::Resource<spall::IPipeline> pipeline = device.pipelines().createPipeline({
		.VertexShader = {vertexShader.get(), vertexEntryPoint},
		.FragmentShader = {fragmentShader.get(), fragmentEntryPoint},
		.ResourceSetLayouts = sampleLayouts,
		.PrimitiveTopology = spall::PrimitiveTopology::TriangleList,
		.ColorTargetFormats = {renderer.colorFormat()},
		.ColorTargetFormatCount = 1});

	const spall::FrameBeginInfo frameInfo = {.ClearColor = {0.02f, 0.03f, 0.05f, 1.0f}};

	const spall::Resource<spall::ICommandList> compute = device.createCommandList();
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

		device.graphicsQueue().submit(*compute);

		spall::Frame frame = renderer.beginFrame(frameInfo);

		spall::ICommandList& commands = frame.commands();
		commands.bindGraphicsPipeline(*pipeline);
		commands.bindResourceSet(0, *sampleSet);
		commands.draw(3, 0);

		frame.end();

		device.graphicsQueue().waitIdle();
	}

	return EXIT_SUCCESS;
}
