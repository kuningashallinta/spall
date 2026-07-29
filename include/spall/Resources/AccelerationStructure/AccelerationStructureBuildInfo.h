#pragma once

#include <cstdint>

namespace spall
{
	/// Describes one recorded acceleration-structure build.
	struct AccelerationStructureBuildInfo
	{
		/// Refits the existing structure in place. Requires AllowUpdate and a prior build.
		bool Update = false;

		/// Top-level instances to build, at most the count declared at creation.
		/// Zero builds them all.
		std::uint32_t InstanceCount = 0;
	};
} // namespace spall
