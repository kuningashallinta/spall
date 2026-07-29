namespace spall
{
	inline Resource<ICommandList> IDevice::createCommandList(
		QueueType type)
	{
		Resource<ICommandList> commandList;
		createCommandList(type, &commandList);
		return commandList;
	}

	inline Status IDevice::createCommandList(
		Resource<ICommandList>* commandList)
	{
		return createCommandList(QueueType::Graphics, commandList);
	}
} // namespace spall
