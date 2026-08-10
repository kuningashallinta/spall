// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace spall
{
	/// Describes how a graphics device is created.
	struct DeviceCreateInfo
	{
		/// Enables backend validation and diagnostic reporting when available.
		bool Debug = false;
	};
} // namespace spall
