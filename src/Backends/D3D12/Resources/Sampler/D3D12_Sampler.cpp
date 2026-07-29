#include <src/Backends/D3D12/Resources/Sampler/D3D12_Sampler.h>

#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>

namespace spall::d3d12
{
	Sampler::Sampler(
		Device& device,
		std::uint32_t descriptorIndex)
		: m_Device(&device), m_DescriptorIndex(descriptorIndex)
	{
	}

	Sampler::~Sampler()
	{
		m_Device->m_SamplerDescriptors.release(m_DescriptorIndex);
	}

	RenderBackendType Sampler::backendType() const
	{
		return RenderBackendType::D3D12;
	}
} // namespace spall::d3d12
