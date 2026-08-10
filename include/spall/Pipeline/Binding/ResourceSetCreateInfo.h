// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Pipeline/Binding/ResourceWrite.h>

#include <span>

namespace spall
{
	class IResourceSetLayout;

	/// Describes a resource set and its optional initial writes.
	struct ResourceSetCreateInfo
	{
		IResourceSetLayout* Layout = nullptr;
		std::span<const ResourceWrite> Writes;
	};
} // namespace spall
