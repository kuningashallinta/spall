// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Limits.h>
#include <spall/Common/Status/Status.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureBuildInfo.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureCreateInfo.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureInfo.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureInstance.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <src/Validation/Common/FlagValidation.h>
#include <src/Validation/Common/FormatValidation.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstdint>

namespace spall
{
	/// Reports whether a buffer may be read by an acceleration-structure build.
	inline bool isAccelerationStructureInput(const IBuffer& buffer);

	inline Status validateAccelerationStructureGeometry(const AccelerationStructureGeometry& geometry);

	inline Status validateAccelerationStructureCreateInfo(const AccelerationStructureCreateInfo& info);

	inline Status validateAccelerationStructureBuildInfo(
		const AccelerationStructureInfo& info,
		const AccelerationStructureBuildInfo& buildInfo);

	inline Status validateAccelerationStructureCompaction(const AccelerationStructureInfo& info);
} // namespace spall

#include <src/Validation/Common/AccelerationStructureValidation.inl>
