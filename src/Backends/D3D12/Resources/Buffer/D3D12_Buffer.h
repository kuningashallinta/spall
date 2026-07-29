#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <string>

namespace spall::d3d12
{
	class CommandList;
	class Device;
	class ResourceStateTracker;

	class Buffer final : public SharedObject<IBuffer>
	{
	public:
		Buffer(
			Device& device,
			const BufferInfo& info,
			ComPtr<ID3D12Resource> resource,
			D3D12_HEAP_TYPE heapType);

		~Buffer(void) override;

		RenderBackendType backendType(void) const override;
		BufferInfo info(void) const override;

	private:
		Resource<Device> m_Device;

		std::string m_DebugName;
		BufferInfo m_Info = {};

		ComPtr<ID3D12Resource> m_Resource;
		D3D12_HEAP_TYPE m_HeapType = D3D12_HEAP_TYPE_DEFAULT;

		/// State left by the most recently submitted command list.
		ResourceStateFlags m_State = ResourceStateFlags::Common;

		ResourceStateFlags m_PermanentState = ResourceStateFlags::Unknown;

	private:
		friend class CommandList;
		friend class Device;
		friend class ResourceSet;
		friend class ResourceStateTracker;
	};
} // namespace spall::d3d12
