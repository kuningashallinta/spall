#pragma once

#include <spall/Backend/IBackend.h>
#include <spall/Common/Enums/RenderBackendType.h>
#include <spall/Common/Status/Status.h>

#include <memory>

namespace spall
{
	/// Creates an enabled rendering backend.
	Status createBackend(
		RenderBackendType backendType,
		std::unique_ptr<IBackend>* backend);

	/// Creates an enabled rendering backend, returning null on failure.
	std::unique_ptr<IBackend> createBackend(RenderBackendType backendType);
} // namespace spall
