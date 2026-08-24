// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <spall/Backends/Vulkan/VK_Backend.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>
#include <windows.h>

#pragma comment(lib, "vulkan-1.lib")

namespace spall::vk
{
	VKAPI_ATTR VkBool32 VKAPI_CALL Backend::handleValidationMessage(
		VkDebugUtilsMessageSeverityFlagBitsEXT,
		VkDebugUtilsMessageTypeFlagsEXT,
		const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
		void*)
	{
		if ((callbackData != nullptr) and (callbackData->pMessage != nullptr))
		{
			OutputDebugStringA("[Spall RHI Vulkan] ");
			OutputDebugStringA(callbackData->pMessage);
			OutputDebugStringA("\n");
		}

		return VK_FALSE;
	}

	Status Backend::hasInstanceLayer(
		const char* layerName,
		bool* found)
	{
		*found = false;

		std::vector<VkLayerProperties> properties;
		VkResult vkResult = VK_SUCCESS;

		do
		{
			std::uint32_t propertyCount = 0;
			vkResult = vkEnumerateInstanceLayerProperties(&propertyCount, nullptr);

			if (vkResult != VK_SUCCESS)
			{
				return mapStatus(vkResult);
			}

			properties.resize(propertyCount);

			if (propertyCount != 0)
			{
				vkResult = vkEnumerateInstanceLayerProperties(&propertyCount, properties.data());
				properties.resize(propertyCount);
			}
		} while (vkResult == VK_INCOMPLETE);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		for (const VkLayerProperties& property : properties)
		{
			if (std::strcmp(property.layerName, layerName) == 0)
			{
				*found = true;
				break;
			}
		}

		return {};
	}

	Status Backend::hasInstanceExtension(
		const char* extensionName,
		bool* found)
	{
		*found = false;

		std::vector<VkExtensionProperties> properties;
		VkResult vkResult = VK_SUCCESS;

		do
		{
			std::uint32_t propertyCount = 0;
			vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, nullptr);

			if (vkResult != VK_SUCCESS)
			{
				return mapStatus(vkResult);
			}

			properties.resize(propertyCount);

			if (propertyCount != 0)
			{
				vkResult = vkEnumerateInstanceExtensionProperties(nullptr, &propertyCount, properties.data());
				properties.resize(propertyCount);
			}
		} while (vkResult == VK_INCOMPLETE);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		for (const VkExtensionProperties& property : properties)
		{
			if (std::strcmp(property.extensionName, extensionName) == 0)
			{
				*found = true;
				break;
			}
		}

		return {};
	}

	Status Backend::hasDeviceExtension(
		VkPhysicalDevice physicalDevice,
		const char* extensionName,
		bool* found)
	{
		*found = false;

		std::vector<VkExtensionProperties> properties;
		VkResult vkResult = VK_SUCCESS;

		do
		{
			std::uint32_t propertyCount = 0;
			vkResult = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, nullptr);

			if (vkResult != VK_SUCCESS)
			{
				return mapStatus(vkResult);
			}

			properties.resize(propertyCount);

			if (propertyCount != 0)
			{
				vkResult = vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &propertyCount, properties.data());
				properties.resize(propertyCount);
			}
		} while (vkResult == VK_INCOMPLETE);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		for (const VkExtensionProperties& property : properties)
		{
			if (std::strcmp(property.extensionName, extensionName) == 0)
			{
				*found = true;
				break;
			}
		}

		return {};
	}

	void Backend::destroyInstanceObjects(
		VkInstance instance,
		VkDebugUtilsMessengerEXT debugMessenger)
	{
		if ((instance != VK_NULL_HANDLE) and (debugMessenger != VK_NULL_HANDLE))
		{
			const PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(
					instance,
					"vkDestroyDebugUtilsMessengerEXT"));

			if (destroyDebugMessenger != nullptr)
			{
				destroyDebugMessenger(instance, debugMessenger, nullptr);
			}
		}

		if (instance != VK_NULL_HANDLE)
		{
			vkDestroyInstance(instance, nullptr);
		}
	}

	RenderBackendType Backend::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	Status Backend::createDevice(
		const DeviceCreateInfo& info,
		Resource<IDevice>* device)
	{
		if (device == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		const char* validationLayerName = "VK_LAYER_KHRONOS_validation";
		std::vector<const char*> enabledLayers;

		bool hasSurface = false;
		SPALL_TRY(hasInstanceExtension(VK_KHR_SURFACE_EXTENSION_NAME, &hasSurface));

		bool hasPlatformSurface = false;
		SPALL_TRY(hasInstanceExtension(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, &hasPlatformSurface));

		if ((not hasSurface) or (not hasPlatformSurface))
		{
			return ERR_UNSUPPORTED;
		}

		std::vector<const char*> enabledExtensions = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_WIN32_SURFACE_EXTENSION_NAME};

		bool hasDebugUtils = false;
		SPALL_TRY(hasInstanceExtension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME, &hasDebugUtils));

		if (info.Debug)
		{
			bool hasValidationLayer = false;
			SPALL_TRY(hasInstanceLayer(validationLayerName, &hasValidationLayer));

			if (not hasValidationLayer)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if (not hasDebugUtils)
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			enabledLayers.push_back(validationLayerName);
		}

		if (hasDebugUtils)
		{
			enabledExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		VkApplicationInfo applicationInfo = {};
		applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName = "Spall RHI application";
		applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.pEngineName = "Spall RHI";
		applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.apiVersion = VK_API_VERSION_1_2;

		VkDebugUtilsMessengerCreateInfoEXT debugMessengerCreateInfo = {};
		debugMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		debugMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		debugMessengerCreateInfo.messageType =
			VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
			VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debugMessengerCreateInfo.pfnUserCallback = handleValidationMessage;

		VkInstanceCreateInfo instanceCreateInfo = {};
		instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		instanceCreateInfo.pApplicationInfo = &applicationInfo;
		instanceCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(enabledExtensions.size());
		instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
		instanceCreateInfo.enabledLayerCount = static_cast<std::uint32_t>(enabledLayers.size());
		instanceCreateInfo.ppEnabledLayerNames = enabledLayers.empty() ? nullptr : enabledLayers.data();
		instanceCreateInfo.pNext = info.Debug ? &debugMessengerCreateInfo : nullptr;

		VkInstance instance = VK_NULL_HANDLE;
		VkResult vkResult = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);

		if (vkResult != VK_SUCCESS)
		{
			return mapStatus(vkResult);
		}

		VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

		if (info.Debug)
		{
			const PFN_vkCreateDebugUtilsMessengerEXT createDebugMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
				vkGetInstanceProcAddr(
					instance,
					"vkCreateDebugUtilsMessengerEXT"));

			if (createDebugMessenger == nullptr)
			{
				destroyInstanceObjects(instance, VK_NULL_HANDLE);
				return ERR_BACKEND_FAILURE;
			}

			vkResult = createDebugMessenger(instance, &debugMessengerCreateInfo, nullptr, &debugMessenger);

			if (vkResult != VK_SUCCESS)
			{
				destroyInstanceObjects(instance, VK_NULL_HANDLE);

				return mapStatus(vkResult);
			}
		}

		std::uint32_t physicalDeviceCount = 0;
		std::vector<VkPhysicalDevice> physicalDevices;

		do
		{
			physicalDeviceCount = 0;
			vkResult = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);

			if (vkResult != VK_SUCCESS)
			{
				destroyInstanceObjects(instance, debugMessenger);
				return mapStatus(vkResult);
			}

			physicalDevices.resize(physicalDeviceCount);

			if (physicalDeviceCount != 0)
			{
				vkResult = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
				physicalDevices.resize(physicalDeviceCount);
			}
		} while (vkResult == VK_INCOMPLETE);

		if (vkResult != VK_SUCCESS)
		{
			destroyInstanceObjects(instance, debugMessenger);
			return mapStatus(vkResult);
		}

		if (physicalDevices.empty())
		{
			destroyInstanceObjects(instance, debugMessenger);
			return ERR_UNSUPPORTED_BACKEND;
		}

		struct DeviceSelection
		{
			VkPhysicalDevice PhysicalDevice = VK_NULL_HANDLE;
			std::uint32_t QueueFamilyIndex = UINT32_MAX;
			bool RayTracing = false;
			bool RayTracingPipeline = false;
		};

		DeviceSelection selection = {};

		for (VkPhysicalDevice physicalDevice : physicalDevices)
		{
			bool hasSwapChainExtension = false;
			const Status extensionError = hasDeviceExtension(physicalDevice, VK_KHR_SWAPCHAIN_EXTENSION_NAME, &hasSwapChainExtension);

			if (extensionError != SUCCESS)
			{
				destroyInstanceObjects(instance, debugMessenger);
				return extensionError;
			}

			if (not hasSwapChainExtension)
			{
				continue;
			}

			std::uint32_t queueFamilyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

			std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

			for (std::uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyCount; ++queueFamilyIndex)
			{
				const bool supportsGraphics = ((queueFamilies[queueFamilyIndex].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0);
				const bool supportsPresentation = (vkGetPhysicalDeviceWin32PresentationSupportKHR(physicalDevice, queueFamilyIndex) == VK_TRUE);

				if (supportsGraphics and supportsPresentation)
				{
					selection.PhysicalDevice = physicalDevice;
					selection.QueueFamilyIndex = queueFamilyIndex;
					break;
				}
			}

			if (selection.PhysicalDevice != VK_NULL_HANDLE)
			{
				break;
			}
		}

		if (selection.PhysicalDevice == VK_NULL_HANDLE)
		{
			destroyInstanceObjects(instance, debugMessenger);
			return ERR_UNSUPPORTED_BACKEND;
		}

		bool hasAccelerationStructureExtension = false;
		bool hasRayQueryExtension = false;
		bool hasDeferredHostOperationsExtension = false;
		bool hasRayTracingPipelineExtension = false;

		(void)hasDeviceExtension(selection.PhysicalDevice, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME, &hasAccelerationStructureExtension);
		(void)hasDeviceExtension(selection.PhysicalDevice, VK_KHR_RAY_QUERY_EXTENSION_NAME, &hasRayQueryExtension);
		(void)hasDeviceExtension(selection.PhysicalDevice, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME, &hasDeferredHostOperationsExtension);
		(void)hasDeviceExtension(selection.PhysicalDevice, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME, &hasRayTracingPipelineExtension);

		if (hasAccelerationStructureExtension and hasRayQueryExtension and hasDeferredHostOperationsExtension)
		{
			VkPhysicalDeviceRayTracingPipelineFeaturesKHR supportedRayTracingPipelineFeatures = {};
			supportedRayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;

			VkPhysicalDeviceRayQueryFeaturesKHR supportedRayQueryFeatures = {};
			supportedRayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;
			supportedRayQueryFeatures.pNext = &supportedRayTracingPipelineFeatures;

			VkPhysicalDeviceAccelerationStructureFeaturesKHR supportedAccelerationStructureFeatures = {};
			supportedAccelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
			supportedAccelerationStructureFeatures.pNext = &supportedRayQueryFeatures;

			VkPhysicalDeviceVulkan12Features supportedVulkan12Features = {};
			supportedVulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
			supportedVulkan12Features.pNext = &supportedAccelerationStructureFeatures;

			VkPhysicalDeviceFeatures2 supportedFeatures2 = {};
			supportedFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
			supportedFeatures2.pNext = &supportedVulkan12Features;

			vkGetPhysicalDeviceFeatures2(selection.PhysicalDevice, &supportedFeatures2);

			selection.RayTracing = (supportedVulkan12Features.bufferDeviceAddress == VK_TRUE) and
				(supportedVulkan12Features.descriptorIndexing == VK_TRUE) and
				(supportedAccelerationStructureFeatures.accelerationStructure == VK_TRUE) and
				(supportedRayQueryFeatures.rayQuery == VK_TRUE);

			selection.RayTracingPipeline = selection.RayTracing and
				hasRayTracingPipelineExtension and
				(supportedRayTracingPipelineFeatures.rayTracingPipeline == VK_TRUE);
		}

		std::uint32_t selectedFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(selection.PhysicalDevice, &selectedFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> selectedFamilies(selectedFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(selection.PhysicalDevice, &selectedFamilyCount, selectedFamilies.data());

		const bool separateComputeQueue = (selection.QueueFamilyIndex < selectedFamilyCount) and
			(selectedFamilies[selection.QueueFamilyIndex].queueCount >= 2);
		const std::uint32_t computeQueueIndex = separateComputeQueue ? 1u : 0u;

		const float queuePriorities[2] = {1.0f, 1.0f};

		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = selection.QueueFamilyIndex;
		queueCreateInfo.queueCount = separateComputeQueue ? 2u : 1u;
		queueCreateInfo.pQueuePriorities = queuePriorities;

		std::vector<const char*> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

		if (selection.RayTracing)
		{
			deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
			deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
			deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		}

		if (selection.RayTracingPipeline)
		{
			deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
		}

		VkPhysicalDeviceFeatures supportedFeatures = {};
		vkGetPhysicalDeviceFeatures(selection.PhysicalDevice, &supportedFeatures);

		VkPhysicalDeviceFeatures enabledFeatures = {};
		enabledFeatures.samplerAnisotropy = supportedFeatures.samplerAnisotropy;
		enabledFeatures.depthBiasClamp = supportedFeatures.depthBiasClamp;
		enabledFeatures.fillModeNonSolid = supportedFeatures.fillModeNonSolid;
		enabledFeatures.wideLines = supportedFeatures.wideLines;
		enabledFeatures.fragmentStoresAndAtomics = supportedFeatures.fragmentStoresAndAtomics;
		enabledFeatures.tessellationShader = supportedFeatures.tessellationShader;
		enabledFeatures.geometryShader = supportedFeatures.geometryShader;
		enabledFeatures.dualSrcBlend = supportedFeatures.dualSrcBlend;

		VkPhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {};
		rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;

		VkPhysicalDeviceRayQueryFeaturesKHR rayQueryFeatures = {};
		rayQueryFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR;

		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {};
		accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;

		VkPhysicalDeviceVulkan12Features vulkan12Features = {};
		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		vulkan12Features.timelineSemaphore = VK_TRUE;

		if (selection.RayTracing)
		{
			vulkan12Features.bufferDeviceAddress = VK_TRUE;
			vulkan12Features.descriptorIndexing = VK_TRUE;
			accelerationStructureFeatures.accelerationStructure = VK_TRUE;
			rayQueryFeatures.rayQuery = VK_TRUE;

			vulkan12Features.pNext = &accelerationStructureFeatures;
			accelerationStructureFeatures.pNext = &rayQueryFeatures;
		}

		if (selection.RayTracingPipeline)
		{
			rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;
			rayQueryFeatures.pNext = &rayTracingPipelineFeatures;
		}

		VkDeviceCreateInfo deviceCreateInfo = {};
		deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceCreateInfo.pNext = &vulkan12Features;
		deviceCreateInfo.queueCreateInfoCount = 1;
		deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
		deviceCreateInfo.enabledExtensionCount = static_cast<std::uint32_t>(deviceExtensions.size());
		deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

		VkDevice vkDevice = VK_NULL_HANDLE;
		vkResult = vkCreateDevice(selection.PhysicalDevice, &deviceCreateInfo, nullptr, &vkDevice);

		if (vkResult != VK_SUCCESS)
		{
			destroyInstanceObjects(instance, debugMessenger);

			return mapStatus(vkResult);
		}

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		vkGetDeviceQueue(vkDevice, selection.QueueFamilyIndex, 0, &graphicsQueue);

		VkQueue computeQueue = VK_NULL_HANDLE;
		vkGetDeviceQueue(vkDevice, selection.QueueFamilyIndex, computeQueueIndex, &computeQueue);

		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		commandPoolCreateInfo.queueFamilyIndex = selection.QueueFamilyIndex;

		VkCommandPool commandPool = VK_NULL_HANDLE;
		vkResult = vkCreateCommandPool(vkDevice, &commandPoolCreateInfo, nullptr, &commandPool);

		if (vkResult != VK_SUCCESS)
		{
			vkDestroyDevice(vkDevice, nullptr);
			destroyInstanceObjects(instance, debugMessenger);

			return mapStatus(vkResult);
		}

		Device* createdDevice = new Device(
			instance,
			debugMessenger,
			hasDebugUtils,
			selection.PhysicalDevice,
			vkDevice,
			selection.QueueFamilyIndex,
			graphicsQueue,
			computeQueue,
			commandPool,
			selection.RayTracing,
			selection.RayTracingPipeline);

		*device = Resource<IDevice>(createdDevice);

		return {};
	}
} // namespace spall::vk
