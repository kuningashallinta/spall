// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

namespace spall
{
	class IQueue;
}

namespace spall::d3d12
{
	/// Inserts a GPU-side wait so the waiting queue blocks until `other` reaches its most recent submission.
	Status insertCrossQueueWait(
		ID3D12CommandQueue& waitingQueue,
		IQueue& other);
} // namespace spall::d3d12
