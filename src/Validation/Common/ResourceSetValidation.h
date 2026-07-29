#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/Pipeline/Binding/ResourceSetCreateInfo.h>
#include <spall/Pipeline/Binding/ResourceSetLayoutCreateInfo.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/TextureView/ITextureView.h>
#include <src/Validation/Common/FlagValidation.h>
#include <src/Validation/Common/ValidationMacros.h>

#include <cstddef>
#include <cstdint>
#include <span>

namespace spall
{
	inline bool isSupportedBindingType(
		ResourceBindingType type)
	{
		return (type == ResourceBindingType::UniformBuffer) or (type == ResourceBindingType::SampledTexture) or
			(type == ResourceBindingType::StorageBuffer) or (type == ResourceBindingType::StorageTexture) or
			(type == ResourceBindingType::AccelerationStructure);
	}

	inline Status validateBindingStages(
		ShaderStageFlags stages);

	inline Status validateResourceSetLayoutCreateInfo(
		const ResourceSetLayoutCreateInfo& info);

	inline Status validateResourceSetCreateInfo(
		const ResourceSetCreateInfo& info);

	inline Status validateResourceWrites(
		std::span<const ResourceWrite> writes);
} // namespace spall

#include <src/Validation/Common/ResourceSetValidation.inl>
