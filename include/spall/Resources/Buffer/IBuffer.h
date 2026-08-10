// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/Resources/Buffer/BufferInfo.h>

namespace spall
{
	/// Represents GPU-accessible linear memory.
	class IBuffer : public IResource
	{
	public:
		virtual BufferInfo info(void) const = 0;
	};
} // namespace spall
