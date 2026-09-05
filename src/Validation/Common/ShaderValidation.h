// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/Pipeline/Shader/ShaderCreateInfo.h>

namespace spall
{
	inline Status validateShaderCreateInfo(const ShaderCreateInfo& info);
} // namespace spall

#include <src/Validation/Common/ShaderValidation.inl>
