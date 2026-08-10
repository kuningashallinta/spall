// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/RenderBackendType.h>
#include <spall/Common/Status/Status.h>

namespace spall
{
	class ICommandList;

	/// Owns command submission for one device queue.
	class IQueue
	{
	public:
		virtual ~IQueue(void) = default;

		virtual RenderBackendType backendType(void) const = 0;

		/// Submits an ended command list.
		virtual Status submit(ICommandList& commandList) = 0;

		/// Makes the next submission on this queue wait, on the GPU, until all work
		/// submitted to `other` before this call has completed.
		virtual Status waitForQueue(IQueue& other) = 0;

		/// Blocks until all previously submitted work has completed.
		virtual Status waitIdle(void) = 0;
	};
} // namespace spall
