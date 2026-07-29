#pragma once

namespace spall
{
	/// Identifies the level an acceleration structure occupies.
	///
	/// A bottom-level structure holds geometry. A top-level structure holds
	/// instances that reference bottom-level structures.
	enum class AccelerationStructureType
	{
		BottomLevel,
		TopLevel
	};
} // namespace spall
