// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <spall/Backend/BackendFactory.h>

#if SPALL_HAS_D3D12
	#include <spall/Backends/D3D12/D3D12_Backend.h>
#endif

#if SPALL_HAS_VULKAN
	#include <spall/Backends/Vulkan/VK_Backend.h>
#endif

#include <memory>

namespace spall
{
	Status createBackend(
		RenderBackendType backendType,
		std::unique_ptr<IBackend>* backend)
	{
		if (backend == nullptr)
		{
			return ERR_INVALID_ARGUMENT;
		}

		backend->reset();

		switch (backendType)
		{
			case RenderBackendType::D3D12:
			{
#if SPALL_HAS_D3D12
				*backend = std::make_unique<d3d12::Backend>();
				return {};
#else
				return ERR_UNSUPPORTED_BACKEND;
#endif
			}

			case RenderBackendType::Vulkan:
			{
#if SPALL_HAS_VULKAN
				*backend = std::make_unique<vk::Backend>();
				return {};
#else
				return ERR_UNSUPPORTED_BACKEND;
#endif
			}
		}

		return ERR_UNSUPPORTED_BACKEND;
	}

	std::unique_ptr<IBackend> createBackend(
		RenderBackendType backendType)
	{
		std::unique_ptr<IBackend> backend;
		createBackend(backendType, &backend);
		return backend;
	}
} // namespace spall
