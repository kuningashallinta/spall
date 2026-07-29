#pragma once

namespace spall
{
	/// Identifies what one bottom-level geometry holds.
	///
	/// Triangles are intersected by fixed-function hardware. Aabbs are bounding
	/// boxes a shader intersects itself, reported as procedural candidates.
	enum class AccelerationStructureGeometryType
	{
		Triangles,
		Aabbs
	};
} // namespace spall
