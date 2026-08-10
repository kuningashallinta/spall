// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline Resource<IDevice> IBackend::createDevice(
		const DeviceCreateInfo& info)
	{
		Resource<IDevice> device;
		createDevice(info, &device);
		return device;
	}
} // namespace spall
