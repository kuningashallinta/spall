// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/Format.h>
#include <spall/Common/Status/Status.h>
#include <spall/Device/FormatCapabilities.h>
#include <src/Validation/Common/FormatValidation.h>

namespace spall
{
	inline Status validateFormatCapabilityQuery(
		Format format,
		FormatCapabilities* capabilities);
} // namespace spall

#include <src/Validation/Common/DeviceValidation.inl>
