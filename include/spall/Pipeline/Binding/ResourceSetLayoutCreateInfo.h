#pragma once

#include <spall/Pipeline/Binding/ResourceBindingInfo.h>

#include <span>

namespace spall
{
	/// Describes the bindings visible through a resource set.
	struct ResourceSetLayoutCreateInfo
	{
		/// Bindings must use unique binding indices.
		std::span<const ResourceBindingInfo> Bindings;
	};
} // namespace spall
