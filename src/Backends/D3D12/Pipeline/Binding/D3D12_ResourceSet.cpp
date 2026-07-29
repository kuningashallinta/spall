#include <src/Backends/D3D12/Pipeline/Binding/D3D12_ResourceSet.h>

#include <spall/Common/Alignment.h>
#include <spall/Common/Assert.h>
#include <src/Backends/D3D12/Common/D3D12_BackendCast.h>
#include <src/Backends/D3D12/Device/D3D12_Device.h>
#include <src/Backends/D3D12/Pipeline/Binding/D3D12_ResourceSetLayout.h>
#include <src/Backends/D3D12/Resources/AccelerationStructure/D3D12_AccelerationStructure.h>
#include <src/Backends/D3D12/Resources/Buffer/D3D12_Buffer.h>
#include <src/Backends/D3D12/Resources/Sampler/D3D12_Sampler.h>
#include <src/Backends/D3D12/Resources/Texture/D3D12_Texture.h>
#include <src/Backends/D3D12/Resources/TextureView/D3D12_TextureView.h>
#include <src/Common/DXGI/DXGIFormatMappings.h>
#include <src/Validation/Common.h>

#include <utility>

namespace spall::d3d12
{
	ResourceSet::ResourceSet(
		Device& device,
		ResourceSetLayout& layout,
		std::vector<std::uint32_t> viewDescriptorIndices)
		: m_Device(&device), m_Layout(&layout), m_ViewDescriptorIndices(std::move(viewDescriptorIndices))
	{
		m_BoundResources.resize(m_ViewDescriptorIndices.size());
	}

	ResourceSet::~ResourceSet()
	{
		for (const std::uint32_t descriptorIndex : m_ViewDescriptorIndices)
		{
			m_Device->m_ShaderResourceDescriptors.release(descriptorIndex);
		}
	}

	RenderBackendType ResourceSet::backendType() const
	{
		return RenderBackendType::D3D12;
	}

	IResourceSetLayout& ResourceSet::layout() const
	{
		return *m_Layout;
	}

	Status ResourceSet::createViewDescriptor(
		const ResourceBindingInfo& bindingInfo,
		const ResourceWrite& write,
		std::uint32_t descriptorIndex)
	{
		const D3D12_CPU_DESCRIPTOR_HANDLE handle = m_Device->m_ShaderResourceDescriptors.cpuHandle(descriptorIndex);

		switch (bindingInfo.Type)
		{
			case ResourceBindingType::UniformBuffer:
			{
				Buffer* buffer = backendCast<Buffer>(write.Buffer);

				D3D12_CONSTANT_BUFFER_VIEW_DESC viewDesc = {};
				viewDesc.BufferLocation = buffer->m_Resource->GetGPUVirtualAddress();
				viewDesc.SizeInBytes = static_cast<UINT>(
					Alignment::up(buffer->m_Info.Size, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT));

				m_Device->m_Device->CreateConstantBufferView(&viewDesc, handle);

				return {};
			}

			case ResourceBindingType::StorageBuffer:
			{
				Buffer* buffer = backendCast<Buffer>(write.Buffer);

				D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
				viewDesc.Format = DXGI_FORMAT_R32_TYPELESS;
				viewDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
				viewDesc.Buffer.FirstElement = 0;
				viewDesc.Buffer.NumElements = buffer->m_Info.Size / 4;
				viewDesc.Buffer.StructureByteStride = 0;
				viewDesc.Buffer.CounterOffsetInBytes = 0;
				viewDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;

				m_Device->m_Device->CreateUnorderedAccessView(buffer->m_Resource.Get(), nullptr, &viewDesc, handle);

				return {};
			}

			case ResourceBindingType::SampledTexture:
			{
				TextureView* view = backendCast<TextureView>(write.TextureView);
				Texture* texture = view->m_Texture.get();

				D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
				viewDesc.Format = dxgi::nativeFormat(texture->m_Info.Format);
				viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

				const bool layered = (view->m_ArrayLayers > 1) or (view->m_BaseArrayLayer != 0);

				if (view->m_Cubemap and (view->m_ArrayLayers > CubemapFaceCount))
				{
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
					viewDesc.TextureCubeArray.MostDetailedMip = view->m_BaseMipLevel;
					viewDesc.TextureCubeArray.MipLevels = view->m_MipLevels;
					viewDesc.TextureCubeArray.First2DArrayFace = view->m_BaseArrayLayer;
					viewDesc.TextureCubeArray.NumCubes = view->m_ArrayLayers / CubemapFaceCount;
				}
				else if (view->m_Cubemap)
				{
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
					viewDesc.TextureCube.MostDetailedMip = view->m_BaseMipLevel;
					viewDesc.TextureCube.MipLevels = view->m_MipLevels;
				}
				else if (texture->m_Info.Depth > 1)
				{
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
					viewDesc.Texture3D.MostDetailedMip = view->m_BaseMipLevel;
					viewDesc.Texture3D.MipLevels = view->m_MipLevels;
					viewDesc.Texture3D.ResourceMinLODClamp = 0.0f;
				}
				else if (texture->m_Info.SampleCount > 1)
				{
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
				}
				else if (layered)
				{
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					viewDesc.Texture2DArray.MostDetailedMip = view->m_BaseMipLevel;
					viewDesc.Texture2DArray.MipLevels = view->m_MipLevels;
					viewDesc.Texture2DArray.FirstArraySlice = view->m_BaseArrayLayer;
					viewDesc.Texture2DArray.ArraySize = view->m_ArrayLayers;
					viewDesc.Texture2DArray.PlaneSlice = 0;
					viewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
				}
				else
				{
					viewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
					viewDesc.Texture2D.MostDetailedMip = view->m_BaseMipLevel;
					viewDesc.Texture2D.MipLevels = view->m_MipLevels;
					viewDesc.Texture2D.PlaneSlice = 0;
					viewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
				}

				m_Device->m_Device->CreateShaderResourceView(texture->m_Resource.Get(), &viewDesc, handle);

				return {};
			}

			case ResourceBindingType::AccelerationStructure:
			{
				AccelerationStructure* structure = backendCast<AccelerationStructure>(write.AccelerationStructure);

				D3D12_SHADER_RESOURCE_VIEW_DESC viewDesc = {};
				viewDesc.Format = DXGI_FORMAT_UNKNOWN;
				viewDesc.ViewDimension = D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
				viewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
				viewDesc.RaytracingAccelerationStructure.Location = structure->m_Resource->GetGPUVirtualAddress();

				m_Device->m_Device->CreateShaderResourceView(nullptr, &viewDesc, handle);

				return {};
			}

			case ResourceBindingType::StorageTexture:
			default:
			{
				TextureView* view = backendCast<TextureView>(write.TextureView);
				Texture* texture = view->m_Texture.get();

				D3D12_UNORDERED_ACCESS_VIEW_DESC viewDesc = {};
				viewDesc.Format = dxgi::nativeFormat(texture->m_Info.Format);

				if (texture->m_Info.Depth > 1)
				{
					viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
					viewDesc.Texture3D.MipSlice = view->m_BaseMipLevel;
					viewDesc.Texture3D.FirstWSlice = 0;
					viewDesc.Texture3D.WSize = mipLevelExtent(texture->m_Info.Depth, view->m_BaseMipLevel);
				}
				else if ((view->m_ArrayLayers > 1) or (view->m_BaseArrayLayer != 0) or view->m_Cubemap)
				{
					viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
					viewDesc.Texture2DArray.MipSlice = view->m_BaseMipLevel;
					viewDesc.Texture2DArray.FirstArraySlice = view->m_BaseArrayLayer;
					viewDesc.Texture2DArray.ArraySize = view->m_ArrayLayers;
					viewDesc.Texture2DArray.PlaneSlice = 0;
				}
				else
				{
					viewDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
					viewDesc.Texture2D.MipSlice = view->m_BaseMipLevel;
					viewDesc.Texture2D.PlaneSlice = 0;
				}

				m_Device->m_Device->CreateUnorderedAccessView(texture->m_Resource.Get(), nullptr, &viewDesc, handle);

				return {};
			}
		}
	}

	Status ResourceSet::writeResources(
		std::span<const ResourceWrite> writes)
	{
		if (m_CommandListReferenceCount != 0)
		{
			return ERR_INVALID_STATE;
		}

		SPALL_TRY(validateResourceWrites(writes));

		std::vector<std::uint32_t> targetBindings;
		targetBindings.reserve(writes.size());

		for (const ResourceWrite& write : writes)
		{
			const ResourceBindingInfo* bindingInfo = m_Layout->findBinding(write.Binding);

			if ((bindingInfo == nullptr) or (bindingInfo->Type != write.Type))
			{
				return ERR_INVALID_BINDING;
			}

			std::uint32_t bindingIndex = 0;

			while ((bindingIndex < m_Layout->m_Bindings.size()) and
				(m_Layout->m_Bindings[bindingIndex].Binding != write.Binding))
			{
				++bindingIndex;
			}

			SPALL_ASSERT(bindingIndex < m_Layout->m_Bindings.size());

			switch (write.Type)
			{
				case ResourceBindingType::UniformBuffer:
				case ResourceBindingType::StorageBuffer:
				{
					Buffer* buffer = backendCast<Buffer>(write.Buffer);

					if ((buffer == nullptr) or (buffer->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE_TYPE;
					}

					const BufferUsageFlags requiredUsage = (write.Type == ResourceBindingType::UniformBuffer)
						? BufferUsageFlags::Uniform
						: BufferUsageFlags::Storage;

					if ((buffer->m_Info.Usage & requiredUsage) == BufferUsageFlags::None)
					{
						return ERR_INVALID_USAGE_FLAGS;
					}

					break;
				}

				case ResourceBindingType::AccelerationStructure:
				{
					AccelerationStructure* structure = backendCast<AccelerationStructure>(write.AccelerationStructure);

					if ((structure == nullptr) or (structure->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE_TYPE;
					}

					break;
				}

				case ResourceBindingType::SampledTexture:
				case ResourceBindingType::StorageTexture:
				default:
				{
					TextureView* view = backendCast<TextureView>(write.TextureView);

					if ((view == nullptr) or (not view->m_Texture) or (view->m_Texture->m_Device.get() != m_Device.get()))
					{
						return ERR_INVALID_RESOURCE_TYPE;
					}

					const TextureUsageFlags requiredUsage = (write.Type == ResourceBindingType::SampledTexture)
						? TextureUsageFlags::Sampled
						: TextureUsageFlags::Storage;

					if ((view->m_Texture->m_Info.Usage & requiredUsage) == TextureUsageFlags::None)
					{
						return ERR_INVALID_USAGE_FLAGS;
					}

					if (write.Type == ResourceBindingType::SampledTexture)
					{
						Sampler* sampler = backendCast<Sampler>(write.Sampler);

						if ((sampler == nullptr) or (sampler->m_Device.get() != m_Device.get()))
						{
							return ERR_INVALID_RESOURCE_TYPE;
						}
					}

					break;
				}
			}

			targetBindings.push_back(bindingIndex);
		}

		for (std::size_t writeIndex = 0; writeIndex < writes.size(); ++writeIndex)
		{
			const ResourceWrite& write = writes[writeIndex];
			const std::uint32_t bindingIndex = targetBindings[writeIndex];

			SPALL_TRY(createViewDescriptor(
				m_Layout->m_Bindings[bindingIndex],
				write,
				m_ViewDescriptorIndices[bindingIndex]));

			BoundResource& bound = m_BoundResources[bindingIndex];
			bound.Buffer.reset(backendCast<Buffer>(write.Buffer));
			bound.TextureView.reset(backendCast<TextureView>(write.TextureView));
			bound.Sampler.reset(backendCast<Sampler>(write.Sampler));
			bound.AccelerationStructure.reset(backendCast<AccelerationStructure>(write.AccelerationStructure));
			bound.Written = true;
		}

		return {};
	}
} // namespace spall::d3d12
