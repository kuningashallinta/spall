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
