// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/RenderBackendType.h>

#include <cstdint>

namespace spall
{
	/// Base interface for intrusive reference-counted graphics objects.
	class IResource
	{
	public:
		/// Adds one strong reference and returns the new reference count.
		virtual std::uint32_t addRef(void) = 0;

		/// Releases one strong reference and destroys the object when the count reaches zero.
		virtual std::uint32_t release(void) = 0;

		/// Gets the graphics API that owns this resource.
		virtual RenderBackendType backendType(void) const = 0;

	protected:
		virtual ~IResource(void) = default;
	};
} // namespace spall
