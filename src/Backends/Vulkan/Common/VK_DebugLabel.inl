// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall::vk
{
	inline VkDebugUtilsLabelEXT debugUtilsLabel(
		const char* label,
		Color color)
	{
		VkDebugUtilsLabelEXT info = {};
		info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT;
		info.pLabelName = label;
		info.color[0] = color.R;
		info.color[1] = color.G;
		info.color[2] = color.B;
		info.color[3] = color.A;

		return info;
	}
} // namespace spall::vk
