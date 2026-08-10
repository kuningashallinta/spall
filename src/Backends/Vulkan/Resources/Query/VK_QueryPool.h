// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Resources/Query/IQueryPool.h>
#include <src/Backends/Vulkan/Common/VK_Error.h>

#include <string>

namespace spall::vk
{
	class Device;
	class GraphicsQueue;
	class CommandList;

	class QueryPool final : public SharedObject<IQueryPool>
	{
	public:
		QueryPool(
			Device& device,
			const QueryPoolInfo& info,
			VkQueryPool queryPool);

		~QueryPool(void) override;

		RenderBackendType backendType(void) const override;
		QueryPoolInfo info(void) const override;

	private:
		Resource<Device> m_Device;

		std::string m_DebugName;
		QueryPoolInfo m_Info = {};

		VkQueryPool m_QueryPool = VK_NULL_HANDLE;

	private:
		friend class Device;
		friend class GraphicsQueue;
		friend class CommandList;
	};
} // namespace spall::vk
