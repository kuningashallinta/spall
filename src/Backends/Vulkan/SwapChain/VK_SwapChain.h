#pragma once

#include <spall/Common/Enums/PresentMode.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/SwapChain/ISwapChain.h>
#include <src/Backends/Vulkan/CommandList/VK_CommandList.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace spall::vk
{
	class Device;
	class Texture;
	class TextureView;
	class Frame;
	class GraphicsQueue;
	class SurfaceLifetime;
	class SwapChainGeneration;

	class SwapChain final : public SharedObject<ISwapChain>
	{
	public:
		struct BackBuffer
		{
			Resource<Texture> Texture;
			Resource<TextureView> View;

			/// Presentation keeps this semaphore in use until the image is acquired again, so it is owned per image rather than per frame slot.
			VkSemaphore RenderFinishedSemaphore = VK_NULL_HANDLE;
		};

		struct FrameResources
		{
			VkSemaphore ImageAvailableSemaphore = VK_NULL_HANDLE;

			// The serial identifies the exact submission that must finish before this slot's semaphore is reused.
			Resource<CommandList> InFlightCommandList;
			std::uint64_t InFlightSubmissionSerial = 0;
		};

		SwapChain(
			Device& device,
			VkSurfaceKHR surface,
			std::uint32_t width,
			std::uint32_t height,
			Format format,
			PresentMode presentMode);

		~SwapChain(void) override;

		RenderBackendType backendType(void) const override;
		Format format(void) const override;
		std::uint32_t frameCount(void) const override;

		Status resize(
			std::uint32_t width,
			std::uint32_t height) override;

		Status recreate(
			std::uint32_t width,
			std::uint32_t height);

	private:
		Status createFrameResources(std::uint32_t frameCount);
		void destroyFrameResources(void);

		Status createBackBuffers(const std::vector<VkImage>& images);
		void destroyBackBuffers(void);

		Resource<Device> m_Device;

		std::shared_ptr<SurfaceLifetime> m_Surface;
		std::shared_ptr<SwapChainGeneration> m_Generation;

		std::uint32_t m_Width = 0;
		std::uint32_t m_Height = 0;
		std::uint32_t m_CurrentFrameSlot = 0;
		std::uint32_t m_LiveFrameCount = 0;
		bool m_RecreateBeforeAcquire = false;

		Format m_RequestedFormat = Format::Unknown;
		Format m_Format = Format::Unknown;
		PresentMode m_PresentMode = PresentMode::VSync;
		VkFormat m_VkFormat = VK_FORMAT_UNDEFINED;
		VkColorSpaceKHR m_ColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

		std::vector<BackBuffer> m_BackBuffers;
		std::vector<FrameResources> m_FrameResources;

	private:
		friend class Device;
		friend class GraphicsQueue;
		friend class Frame;
	};
} // namespace spall::vk
