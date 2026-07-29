#include "Shaders.h"

#include <spall/CommandList/ICommandList.h>
#include <spall/Device/IDevice.h>
#include <spall/Pipeline/Pipeline/IPipeline.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <spall/Runtime/Renderer.h>

#include <windows.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>

namespace
{
	constexpr std::uint32_t WindowWidth = 800;
	constexpr std::uint32_t WindowHeight = 600;

	constexpr spall::RenderBackendType BackendType = spall::RenderBackendType::D3D12;

	struct Vertex
	{
		float Position[3];
		float Color[3];
	};

	struct PushConstants
	{
		float Angle;
		float Aspect;
	};

	constexpr Vertex Vertices[] = {
		{{-0.75f, -0.75f, -0.75f}, {0.0f, 0.0f, 0.0f}},
		{{0.75f, -0.75f, -0.75f}, {1.0f, 0.0f, 0.0f}},
		{{-0.75f, 0.75f, -0.75f}, {0.0f, 1.0f, 0.0f}},
		{{0.75f, 0.75f, -0.75f}, {1.0f, 1.0f, 0.0f}},
		{{-0.75f, -0.75f, 0.75f}, {0.0f, 0.0f, 1.0f}},
		{{0.75f, -0.75f, 0.75f}, {1.0f, 0.0f, 1.0f}},
		{{-0.75f, 0.75f, 0.75f}, {0.0f, 1.0f, 1.0f}},
		{{0.75f, 0.75f, 0.75f}, {1.0f, 1.0f, 1.0f}}};

	constexpr std::uint16_t Indices[] = {
		0, 2, 1, 1, 2, 3,
		4, 5, 6, 5, 7, 6,
		0, 4, 2, 4, 6, 2,
		1, 3, 5, 5, 3, 7,
		2, 6, 3, 3, 6, 7,
		0, 1, 4, 1, 5, 4};

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
	windowClass.lpszClassName = L"SpallSpinningCube";
	RegisterClassW(&windowClass);

	constexpr DWORD windowStyle = WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;

	RECT bounds = {0, 0, WindowWidth, WindowHeight};
	AdjustWindowRect(&bounds, windowStyle, FALSE);

	HWND window = CreateWindowExW(
		0,
		windowClass.lpszClassName,
		L"Spinning Cube",
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
			.Format = spall::Format::BGRA8Srgb},
		.DepthStencilFormat = spall::Format::Depth32Float});

	if (not renderer)
	{
		return EXIT_FAILURE;
	}

	spall::IDevice& device = renderer.device();

	spall::Resource<spall::IShader> vertexShader;
	spall::Resource<spall::IShader> fragmentShader;

	const char* vertexEntryPoint = nullptr;
	const char* fragmentEntryPoint = nullptr;

	switch (BackendType)
	{
		case spall::RenderBackendType::D3D12:
		{
			vertexShader = device.pipelines().createShader(spall::ShaderStage::Vertex, shaders::HlslVertex);
			fragmentShader = device.pipelines().createShader(spall::ShaderStage::Fragment, shaders::HlslFragment);

			vertexEntryPoint = "vsMain";
			fragmentEntryPoint = "psMain";
			break;
		}

		case spall::RenderBackendType::Vulkan:
		{
			vertexShader = device.pipelines().createShader(spall::ShaderStage::Vertex, shaders::VulkanVertex);
			fragmentShader = device.pipelines().createShader(spall::ShaderStage::Fragment, shaders::VulkanFragment);

			vertexEntryPoint = "main";
			fragmentEntryPoint = "main";
			break;
		}
	}

	const spall::Resource<spall::IBuffer> vertexBuffer = device.resources().createBufferWithData(
		{.Usage = spall::BufferUsageFlags::Vertex},
		Vertices);

	const spall::Resource<spall::IBuffer> indexBuffer = device.resources().createBufferWithData(
		{.Usage = spall::BufferUsageFlags::Index},
		Indices);

	const spall::VertexBindingInfo vertexBindings[] = {{0, sizeof(Vertex)}};

	const spall::VertexAttributeInfo vertexAttributes[] = {
		{0, 0, spall::Format::RGB32Float, offsetof(Vertex, Position)},
		{1, 0, spall::Format::RGB32Float, offsetof(Vertex, Color)}};

	const spall::Resource<spall::IPipeline> pipeline = device.pipelines().createPipeline({
		.VertexShader = {vertexShader.get(), vertexEntryPoint},
		.FragmentShader = {fragmentShader.get(), fragmentEntryPoint},
		.VertexBindings = vertexBindings,
		.VertexAttributes = vertexAttributes,
		.PrimitiveTopology = spall::PrimitiveTopology::TriangleList,
		.ColorTargetFormats = {spall::Format::BGRA8Srgb},
		.ColorTargetFormatCount = 1,
		.DepthStencilFormat = spall::Format::Depth32Float,
		.EnableDepthTest = true,
		.EnableDepthWrite = true,
		.PushConstants = {spall::ShaderStageFlags::Vertex, sizeof(PushConstants)}});

	const spall::FrameBeginInfo frameInfo = {.ClearColor = {0.02f, 0.03f, 0.05f, 1.0f}};

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

		const PushConstants constants = {
			std::chrono::duration<float>(std::chrono::steady_clock::now() - start).count(),
			static_cast<float>(WindowWidth) / static_cast<float>(WindowHeight)};

		spall::Frame frame = renderer.beginFrame(frameInfo);

		spall::ICommandList& commands = frame.commands();
		commands.bindGraphicsPipeline(*pipeline);
		commands.setPushConstants(spall::ShaderStageFlags::Vertex, 0, constants);
		commands.setVertexBuffer(0, *vertexBuffer, sizeof(Vertex), 0);
		commands.setIndexBuffer(*indexBuffer, spall::IndexFormat::UInt16, 0);
		commands.drawIndexed(std::size(Indices), 0, 0);

		frame.end();
	}

	return EXIT_SUCCESS;
}
