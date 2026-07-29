#pragma once

namespace spall
{
	enum class StencilOp
	{
		Keep,
		Zero,
		Replace,
		IncrementClamp,
		DecrementClamp,
		Invert,
		IncrementWrap,
		DecrementWrap
	};
} // namespace spall
