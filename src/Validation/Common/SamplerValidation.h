// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/Resources/Sampler/SamplerCreateInfo.h>

#include <cmath>

namespace spall
{
	inline Status validateSamplerCreateInfo(const SamplerCreateInfo& info);
} // namespace spall

#include <src/Validation/Common/SamplerValidation.inl>
