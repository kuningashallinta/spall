#include <src/Backends/Vulkan/Pipeline/Binding/VK_ResourceSet.h>

#include <spall/Common/Assert.h>
#include <spall/Common/Enums/RenderBackendType.h>
#include <src/Backends/Vulkan/Device/VK_Device.h>
#include <src/Backends/Vulkan/Resources/AccelerationStructure/VK_AccelerationStructure.h>
#include <src/Backends/Vulkan/Resources/Buffer/VK_Buffer.h>
#include <src/Backends/Vulkan/Resources/Sampler/VK_Sampler.h>
#include <src/Backends/Vulkan/Resources/Texture/VK_Texture.h>
#include <src/Backends/Vulkan/Resources/TextureView/VK_TextureView.h>
#include <src/Validation/Common.h>

#include <cstddef>
#include <utility>
#include <vector>

namespace spall::vk
{
	ResourceSet::ResourceSet(
		Device& device,
		ResourceSetLayout& layout,
		VkDescriptorPool descriptorPool,
		VkDescriptorSet descriptorSet)
		: m_Device(&device), m_Layout(&layout), m_DescriptorPool(descriptorPool), m_DescriptorSet(descriptorSet)
	{
	}

	ResourceSet::~ResourceSet()
	{
		SPALL_VERIFY(m_CommandListReferenceCount == 0);

		if (m_Device and (m_Device->m_Device != VK_NULL_HANDLE) and (m_DescriptorPool != VK_NULL_HANDLE))
		{
			vkDestroyDescriptorPool(m_Device->m_Device, m_DescriptorPool, nullptr);
		}
	}

	RenderBackendType ResourceSet::backendType() const
	{
		return RenderBackendType::Vulkan;
	}

	IResourceSetLayout& ResourceSet::layout() const
	{
		return *m_Layout;
	}

	const ResourceWrite* ResourceSet::findWrite(
		std::uint32_t binding) const
	{
		for (const ResourceWrite& write : m_Writes)
		{
			if (write.Binding == binding)
			{
				return &write;
			}
		}

		return nullptr;
	}

	Status ResourceSet::writeResources(
		std::span<const ResourceWrite> writes)
	{
		if (m_CommandListReferenceCount != 0)
		{
			return ERR_INVALID_RESOURCE_STATE;
		}

		SPALL_TRY(validateResourceWrites(writes));

		std::vector<ResourceWrite> validatedWrites;
		std::vector<Resource<IResource>> retainedResources;
		std::vector<VkDescriptorBufferInfo> bufferInfos;
		std::vector<VkDescriptorImageInfo> imageInfos;
		std::vector<VkWriteDescriptorSetAccelerationStructureKHR> accelerationStructureWrites;
		std::vector<VkAccelerationStructureKHR> accelerationStructureHandles;
		std::vector<VkWriteDescriptorSet> descriptorWrites;

		validatedWrites.reserve(writes.size());
		retainedResources.reserve(writes.size() * 2);
		bufferInfos.reserve(writes.size());
		imageInfos.reserve(writes.size());
		accelerationStructureWrites.reserve(writes.size());
		accelerationStructureHandles.reserve(writes.size());
		descriptorWrites.reserve(writes.size());

		for (const ResourceWrite& write : writes)
		{
			const ResourceBindingInfo* bindingInfo = m_Layout->findBinding(write.Binding);

			if (bindingInfo == nullptr)
			{
				return ERR_INVALID_BINDING;
			}

			if (bindingInfo->Type != write.Type)
			{
				return ERR_INVALID_BINDING;
			}

			VkWriteDescriptorSet descriptorWrite = {};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSet;
			descriptorWrite.dstBinding = write.Binding;
			descriptorWrite.descriptorCount = 1;

			if (write.Type == ResourceBindingType::UniformBuffer)
			{
				Buffer* buffer = dynamic_cast<Buffer*>(write.Buffer);

				if ((buffer == nullptr) or (buffer->m_Device.get() != m_Device.get()) or
					((buffer->m_Info.Usage & BufferUsageFlags::Uniform) == BufferUsageFlags::None))
				{
					return ERR_INVALID_RESOURCE;
				}

				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = buffer->m_Buffer;
				bufferInfo.offset = 0;
				bufferInfo.range = buffer->m_Info.Size;
				bufferInfos.push_back(bufferInfo);
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrite.pBufferInfo = &bufferInfos.back();
				retainedResources.emplace_back(buffer);
			}
			else if (write.Type == ResourceBindingType::SampledTexture)
			{
				TextureView* textureView = dynamic_cast<TextureView*>(write.TextureView);
				Sampler* sampler = dynamic_cast<Sampler*>(write.Sampler);

				if ((textureView == nullptr) or (not textureView->m_Texture) or (textureView->m_Texture->m_Device.get() != m_Device.get()) or
					(textureView->m_View == VK_NULL_HANDLE) or (sampler == nullptr) or (sampler->m_Device.get() != m_Device.get()) or
					(sampler->m_Sampler == VK_NULL_HANDLE))
				{
					return ERR_INVALID_RESOURCE;
				}

				VkDescriptorImageInfo imageInfo = {};
				imageInfo.sampler = sampler->m_Sampler;
				imageInfo.imageView = textureView->m_View;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				imageInfos.push_back(imageInfo);
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrite.pImageInfo = &imageInfos.back();
				retainedResources.emplace_back(textureView);
				retainedResources.emplace_back(sampler);
			}
			else if (write.Type == ResourceBindingType::StorageBuffer)
			{
				Buffer* buffer = dynamic_cast<Buffer*>(write.Buffer);

				if ((buffer == nullptr) or (buffer->m_Device.get() != m_Device.get()) or
					((buffer->m_Info.Usage & BufferUsageFlags::Storage) == BufferUsageFlags::None))
				{
					return ERR_INVALID_RESOURCE;
				}

				VkDescriptorBufferInfo bufferInfo = {};
				bufferInfo.buffer = buffer->m_Buffer;
				bufferInfo.offset = 0;
				bufferInfo.range = buffer->m_Info.Size;
				bufferInfos.push_back(bufferInfo);
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descriptorWrite.pBufferInfo = &bufferInfos.back();
				retainedResources.emplace_back(buffer);
			}
			else if (write.Type == ResourceBindingType::StorageTexture)
			{
				TextureView* textureView = dynamic_cast<TextureView*>(write.TextureView);

				if ((textureView == nullptr) or (not textureView->m_Texture) or
					(textureView->m_Texture->m_Device.get() != m_Device.get()) or
					(textureView->m_View == VK_NULL_HANDLE))
				{
					return ERR_INVALID_RESOURCE;
				}

				VkDescriptorImageInfo imageInfo = {};
				imageInfo.imageView = textureView->m_View;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
				imageInfos.push_back(imageInfo);
				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				descriptorWrite.pImageInfo = &imageInfos.back();
				retainedResources.emplace_back(textureView);
			}
			else if (write.Type == ResourceBindingType::AccelerationStructure)
			{
				AccelerationStructure* structure = dynamic_cast<AccelerationStructure*>(write.AccelerationStructure);

				if ((structure == nullptr) or (structure->m_Device.get() != m_Device.get()) or
					(structure->m_AccelerationStructure == VK_NULL_HANDLE))
				{
					return ERR_INVALID_RESOURCE;
				}

				accelerationStructureHandles.push_back(structure->m_AccelerationStructure);

				VkWriteDescriptorSetAccelerationStructureKHR accelerationStructureWrite = {};
				accelerationStructureWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
				accelerationStructureWrite.accelerationStructureCount = 1;
				accelerationStructureWrite.pAccelerationStructures = &accelerationStructureHandles.back();
				accelerationStructureWrites.push_back(accelerationStructureWrite);

				descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				descriptorWrite.pNext = &accelerationStructureWrites.back();
				retainedResources.emplace_back(structure);
			}
			else
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			descriptorWrites.push_back(descriptorWrite);
			validatedWrites.push_back(write);
		}

		if (not descriptorWrites.empty())
		{
			vkUpdateDescriptorSets(
				m_Device->m_Device,
				static_cast<std::uint32_t>(descriptorWrites.size()),
				descriptorWrites.data(),
				0,
				nullptr);
		}

		m_Writes = std::move(validatedWrites);
		m_RetainedResources = std::move(retainedResources);

		return {};
	}
} // namespace spall::vk
