// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Backend/IBackend.h>

#include <vulkan/vulkan.h>

#include <cstdint>

namespace spall::vk
{
	class Backend final : public IBackend
	{
	public:
		using IBackend::createDevice;

		RenderBackendType backendType(void) const override;

		Status createDevice(
			const DeviceCreateInfo& info,
			Resource<IDevice>* device) override;

	private:
		static VKAPI_ATTR VkBool32 VKAPI_CALL handleValidationMessage(
			VkDebugUtilsMessageSeverityFlagBitsEXT severity,
			VkDebugUtilsMessageTypeFlagsEXT type,
			const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
			void* userData);

		static Status hasInstanceLayer(
			const char* layerName,
			bool* found);

		static Status hasInstanceExtension(
			const char* extensionName,
			bool* found);

		static Status hasDeviceExtension(
			VkPhysicalDevice physicalDevice,
			const char* extensionName,
			bool* found);

		static void destroyInstanceObjects(
			VkInstance instance,
			VkDebugUtilsMessengerEXT debugMessenger);
	};
} // namespace spall::vk
