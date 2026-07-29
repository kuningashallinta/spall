namespace spall
{
	inline AccelerationStructureInstance makeAccelerationStructureInstance(
		const IAccelerationStructure& bottomLevel,
		const float (&transform)[12],
		std::uint32_t instanceId,
		std::uint8_t instanceMask,
		AccelerationStructureInstanceFlags flags,
		std::uint32_t instanceContribution)
	{
		AccelerationStructureInstance instance = {};

		for (std::uint32_t element = 0; element < 12; ++element)
		{
			instance.Transform[element] = transform[element];
		}

		instance.InstanceIdAndMask =
			(instanceId & MaxAccelerationStructureInstanceId) |
			(static_cast<std::uint32_t>(instanceMask) << 24);

		instance.InstanceContributionAndFlags =
			(instanceContribution & MaxAccelerationStructureInstanceId) |
			((static_cast<std::uint32_t>(flags) & 0xFF) << 24);

		instance.AccelerationStructure = bottomLevel.deviceAddress();

		return instance;
	}
} // namespace spall
