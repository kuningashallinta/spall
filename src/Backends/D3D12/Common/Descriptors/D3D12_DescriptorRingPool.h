#pragma once

#include <spall/Common/Status/Status.h>
#include <src/Backends/D3D12/Common/Descriptors/D3D12_DescriptorRing.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <mutex>
#include <vector>

namespace spall::d3d12
{
	/// The shader-visible heaps one recording bump-allocates from.
	struct DescriptorRingSet
	{
		DescriptorRing Views;
		DescriptorRing Samplers;
	};

	/// Recycles shader-visible heaps between command lists. Acquisition is thread-safe.
	///
	/// Creating a descriptor heap is expensive, and a caller that builds a fresh
	/// command list every frame would otherwise pay for two of them per frame.
	class DescriptorRingPool
	{
	public:
		Status acquire(
			ID3D12Device& device,
			DescriptorRingSet* rings);

		void release(DescriptorRingSet rings);

	private:
		std::vector<DescriptorRingSet> m_FreeSets;
		std::mutex m_Mutex;
	};
} // namespace spall::d3d12
