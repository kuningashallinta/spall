namespace spall::d3d12
{
	inline D3D12_HEAP_PROPERTIES heapProperties(
		D3D12_HEAP_TYPE heapType)
	{
		D3D12_HEAP_PROPERTIES properties = {};
		properties.Type = heapType;
		properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		properties.CreationNodeMask = 1;
		properties.VisibleNodeMask = 1;

		return properties;
	}

	inline D3D12_HEAP_TYPE bufferHeapType(
		MemoryAccess access)
	{
		switch (access)
		{
			case MemoryAccess::Write:
			{
				return D3D12_HEAP_TYPE_UPLOAD;
			}

			case MemoryAccess::Read:
			{
				return D3D12_HEAP_TYPE_READBACK;
			}

			case MemoryAccess::None:
			default:
			{
				return D3D12_HEAP_TYPE_DEFAULT;
			}
		}
	}
} // namespace spall::d3d12
