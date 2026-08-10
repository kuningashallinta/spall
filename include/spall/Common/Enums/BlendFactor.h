// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace spall
{
	enum class BlendFactor
	{
		Zero,
		One,
		SourceColor,
		OneMinusSourceColor,
		DestinationColor,
		OneMinusDestinationColor,
		SourceAlpha,
		OneMinusSourceAlpha,
		DestinationAlpha,
		OneMinusDestinationAlpha,
		Source1Color,
		OneMinusSource1Color,
		Source1Alpha,
		OneMinusSource1Alpha
	};
} // namespace spall
