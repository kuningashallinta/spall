#include <catch2/catch_test_macros.hpp>

#include <src/Backends/Vulkan/Common/VK_DebugLabel.h>

#include <cstring>

TEST_CASE(
	"Vulkan debug labels retain their text and color",
	"[vulkan][commandlist][debugmarker]")
{
	const spall::Color color = {0.25f, 0.5f, 0.75f, 1.0f};
	const VkDebugUtilsLabelEXT label = spall::vk::debugUtilsLabel("Lighting", color);

	CHECK(label.sType == VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT);
	CHECK(label.pNext == nullptr);
	CHECK(std::strcmp(label.pLabelName, "Lighting") == 0);
	CHECK(label.color[0] == 0.25f);
	CHECK(label.color[1] == 0.5f);
	CHECK(label.color[2] == 0.75f);
	CHECK(label.color[3] == 1.0f);
}
