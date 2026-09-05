// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/SwapChain/SwapChainCreateInfo.h>
#include <src/Validation/Common/FormatValidation.h>

namespace spall
{
	inline Status validateSwapChainCreateInfo(const SwapChainCreateInfo& info);
} // namespace spall

#include <src/Validation/Common/SwapChainValidation.inl>
