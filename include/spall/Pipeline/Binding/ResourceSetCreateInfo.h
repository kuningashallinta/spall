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
