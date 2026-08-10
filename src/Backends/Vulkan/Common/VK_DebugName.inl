// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline void setDebugName(
		VkDevice device,
		VkObjectType objectType,
		std::uint64_t objectHandle,
		const char* name)
	{
		if ((device == VK_NULL_HANDLE) or (objectHandle == 0) or (name == nullptr) or (name[0] == '\0'))
		{
			return;
		}

		const PFN_vkSetDebugUtilsObjectNameEXT setObjectName = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(
			vkGetDeviceProcAddr(
				device,
				"vkSetDebugUtilsObjectNameEXT"));

		if (setObjectName == nullptr)
		{
			return;
		}

		VkDebugUtilsObjectNameInfoEXT nameInfo = {};
		nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		nameInfo.objectType = objectType;
		nameInfo.objectHandle = objectHandle;
		nameInfo.pObjectName = name;
		(void)setObjectName(device, &nameInfo);
	}
} // namespace spall::vk
