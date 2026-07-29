#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Resource/SharedObject.h>

#include <spall/Pipeline/Binding/IResourceSet.h>
#include <src/Backends/D3D12/Common/Descriptors/D3D12_DescriptorHeap.h>
#include <src/Backends/D3D12/Common/D3D12_Types.h>

#include <cstdint>
#include <vector>

namespace spall::d3d12
{
	class AccelerationStructure;
	class Buffer;
	class CommandList;
	class Device;
	class ResourceSetLayout;
	class Sampler;
	class TextureView;

	class ResourceSet final : public SharedObject<IResourceSet>
	{
	public:
		ResourceSet(
			Device& device,
			ResourceSetLayout& layout,
			std::vector<std::uint32_t> viewDescriptorIndices);

		~ResourceSet(void) override;

		RenderBackendType backendType(void) const override;

		IResourceSetLayout& layout(void) const override;

		Status writeResources(std::span<const ResourceWrite> writes) override;

	private:
		struct BoundResource
		{
			Resource<spall::d3d12::Buffer> Buffer;
			Resource<spall::d3d12::TextureView> TextureView;
			Resource<spall::d3d12::Sampler> Sampler;
			Resource<spall::d3d12::AccelerationStructure> AccelerationStructure;
			bool Written = false;
		};

		Status createViewDescriptor(
			const ResourceBindingInfo& bindingInfo,
			const ResourceWrite& write,
			std::uint32_t descriptorIndex);

		Resource<Device> m_Device;
		Resource<ResourceSetLayout> m_Layout;

		std::vector<std::uint32_t> m_ViewDescriptorIndices;
		std::vector<BoundResource> m_BoundResources;

		std::uint32_t m_CommandListReferenceCount = 0;

	private:
		friend class CommandList;
		friend class Device;
	};
} // namespace spall::d3d12
