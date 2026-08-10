// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Enums/RenderBackendType.h>
#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/Device/DeviceCreateInfo.h>
#include <spall/Device/IDevice.h>

namespace spall
{
	/// Creates devices for one graphics API.
	///
	/// The convenience overload returns an empty resource on failure; the Status
	/// overload preserves detailed failure information.
	class IBackend
	{
	public:
		virtual ~IBackend(void) = default;

		virtual RenderBackendType backendType(void) const = 0;

		Resource<IDevice> createDevice(
			const DeviceCreateInfo& info = {});

		virtual Status createDevice(
			const DeviceCreateInfo& info,
			Resource<IDevice>* device) = 0;
	};
} // namespace spall

#include <spall/Backend/IBackend.inl>
