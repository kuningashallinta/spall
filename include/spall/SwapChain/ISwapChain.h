// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Status/Status.h>

#include <cstdint>

namespace spall
{
	/// Owns the images used to present rendering results to a window.
	class ISwapChain : public IResource
	{
	public:
		virtual Format format(void) const = 0;
		virtual std::uint32_t frameCount(void) const = 0;

		/// Recreates the back buffers. All acquired frames and retained back-buffer references must be released first.
		/// A zero dimension records a minimized extent without recreating the back buffers.
		virtual Status resize(
			std::uint32_t width,
			std::uint32_t height) = 0;
	};
} // namespace spall
