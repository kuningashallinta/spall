#include <src/Backends/D3D12/Common/Descriptors/D3D12_DescriptorRingPool.h>

#include <spall/Common/Assert.h>
#include <src/Backends/D3D12/Common/D3D12_Limits.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <mutex>
#include <utility>

namespace spall::d3d12
{
	Status DescriptorRingPool::acquire(
		ID3D12Device& device,
		DescriptorRingSet* rings)
	{
		SPALL_ASSERT(rings != nullptr);

		const std::lock_guard<std::mutex> guard(m_Mutex);

		if (not m_FreeSets.empty())
		{
			*rings = std::move(m_FreeSets.back());
			m_FreeSets.pop_back();

			rings->Views.reset();
			rings->Samplers.reset();

			return {};
		}

		DescriptorRingSet created = {};
		SPALL_TRY(created.Views.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, DescriptorRingViewCapacity));
		SPALL_TRY(created.Samplers.initialize(device, D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, DescriptorRingSamplerCapacity));

		*rings = std::move(created);

		return {};
	}

	void DescriptorRingPool::release(
		DescriptorRingSet rings)
	{
		if (rings.Views.heap() == nullptr)
		{
			return;
		}

		const std::lock_guard<std::mutex> guard(m_Mutex);

		m_FreeSets.push_back(std::move(rings));
	}
} // namespace spall::d3d12
