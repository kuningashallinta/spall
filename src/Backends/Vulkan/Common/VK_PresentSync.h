// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vulkan/vulkan.hpp>

namespace spall::vk
{
	/// Stages at which a submission waits for the swap-chain acquire semaphore.
	///
	/// Entry barriers that transition a swap-chain image must include these
	/// stages in their source mask so the layout transition is ordered after
	/// the wait rather than racing the presentation engine.
	inline constexpr VkPipelineStageFlags SwapChainAcquireWaitStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_TRANSFER_BIT;
} // namespace spall::vk
