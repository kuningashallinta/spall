// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline Status mapVulkanStatus(VkResult result)
	{
		switch (result)
		{
			case VK_SUCCESS:
			case VK_SUBOPTIMAL_KHR:
			{
				return SUCCESS;
			}

			case VK_NOT_READY:
			{
				return ERR_NOT_READY;
			}

			case VK_ERROR_OUT_OF_HOST_MEMORY:
			case VK_ERROR_OUT_OF_DEVICE_MEMORY:
			{
				return ERR_OUT_OF_MEMORY;
			}

			case VK_ERROR_DEVICE_LOST:
			{
				return ERR_DEVICE_LOST;
			}

			case VK_ERROR_EXTENSION_NOT_PRESENT:
			case VK_ERROR_FEATURE_NOT_PRESENT:
			case VK_ERROR_LAYER_NOT_PRESENT:
			{
				return ERR_UNSUPPORTED;
			}

			case VK_ERROR_FORMAT_NOT_SUPPORTED:
			{
				return ERR_UNSUPPORTED_FORMAT;
			}

			case VK_ERROR_OUT_OF_DATE_KHR:
			{
				return ERR_SWAP_CHAIN_OUT_OF_DATE;
			}

			case VK_ERROR_SURFACE_LOST_KHR:
			case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
			{
				return ERR_SWAP_CHAIN_CREATION_FAILED;
			}

			case VK_ERROR_INCOMPATIBLE_DRIVER:
			case VK_ERROR_INITIALIZATION_FAILED:
			default:
			{
				return ERR_BACKEND_FAILURE;
			}
		}
	}
} // namespace spall::vk
