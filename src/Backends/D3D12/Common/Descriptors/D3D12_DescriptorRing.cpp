#include <src/Backends/D3D12/Common/Descriptors/D3D12_DescriptorRing.h>

#include <spall/Common/Assert.h>
#include <src/Common/DXGI/DXGIError.h>

namespace spall::d3d12
{
	Status DescriptorRing::initialize(
		ID3D12Device& device,
		D3D12_DESCRIPTOR_HEAP_TYPE type,
		std::uint32_t capacity)
	{
		SPALL_ASSERT(capacity != 0);

		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
		heapDesc.Type = type;
		heapDesc.NumDescriptors = capacity;
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

		const HRESULT hr = device.CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_Heap));

		if (FAILED(hr))
		{
			return dxgi::mapHResult(hr);
		}

		m_CpuStart = m_Heap->GetCPUDescriptorHandleForHeapStart();
		m_GpuStart = m_Heap->GetGPUDescriptorHandleForHeapStart();
		m_DescriptorSize = device.GetDescriptorHandleIncrementSize(type);
		m_Capacity = capacity;
		m_Used = 0;

		return {};
	}

	void DescriptorRing::reset()
	{
		m_Used = 0;
	}

	Status DescriptorRing::allocate(
		std::uint32_t count,
		D3D12_CPU_DESCRIPTOR_HANDLE* cpuStart,
		D3D12_GPU_DESCRIPTOR_HANDLE* gpuStart)
	{
		SPALL_ASSERT((cpuStart != nullptr) and (gpuStart != nullptr));

		if (count > (m_Capacity - m_Used))
		{
			return ERR_OUT_OF_MEMORY;
		}

		cpuStart->ptr = m_CpuStart.ptr + (static_cast<SIZE_T>(m_Used) * m_DescriptorSize);
		gpuStart->ptr = m_GpuStart.ptr + (static_cast<UINT64>(m_Used) * m_DescriptorSize);
		m_Used += count;

		return {};
	}

	ID3D12DescriptorHeap* DescriptorRing::heap() const
	{
		return m_Heap.Get();
	}
} // namespace spall::d3d12
