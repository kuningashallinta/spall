// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/Resources/AccelerationStructure/AccelerationStructureInfo.h>

#include <cstdint>

namespace spall
{
	/// Represents a ray-traceable spatial structure over geometry or instances.
	///
	/// The structure holds one state for its whole lifetime and is never
	/// transitioned. A recorded build inserts the barrier its readers need.
	class IAccelerationStructure : public IResource
	{
	public:
		virtual AccelerationStructureInfo info(void) const = 0;

		/// Gets the opaque GPU address for AccelerationStructureInstance::AccelerationStructure.
		///
		/// Contents remain undefined until the first build completes. Storing this
		/// address in an instance does not retain the referenced structure; callers
		/// must keep it alive while a top-level structure may trace it.
		virtual std::uint64_t deviceAddress(void) const = 0;
	};
} // namespace spall
