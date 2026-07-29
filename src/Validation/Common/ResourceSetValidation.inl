namespace spall
{
	inline Status validateBindingStages(
		ShaderStageFlags stages)
	{
		constexpr ShaderStageFlags knownStages = ShaderStageFlags::Vertex | ShaderStageFlags::Fragment | ShaderStageFlags::Compute |
			ShaderStageFlags::RayGeneration | ShaderStageFlags::Miss | ShaderStageFlags::ClosestHit |
			ShaderStageFlags::AnyHit | ShaderStageFlags::Intersection;

		if (stages == ShaderStageFlags::None)
		{
			return ERR_INVALID_SHADER_STAGE;
		}

		if (not hasOnlyKnownFlags(stages, knownStages))
		{
			return ERR_UNSUPPORTED_SHADER_STAGE;
		}

		return {};
	}

	inline Status validateResourceSetLayoutCreateInfo(
		const ResourceSetLayoutCreateInfo& info)
	{
		if (info.Bindings.empty())
		{
			return ERR_INVALID_BINDING;
		}

		for (std::size_t bindingIndex = 0; bindingIndex < info.Bindings.size(); ++bindingIndex)
		{
			const ResourceBindingInfo& bindingInfo = info.Bindings[bindingIndex];

			for (std::size_t compareIndex = bindingIndex + 1; compareIndex < info.Bindings.size(); ++compareIndex)
			{
				if (bindingInfo.Binding == info.Bindings[compareIndex].Binding)
				{
					return ERR_INVALID_BINDING;
				}
			}

			if (not isSupportedBindingType(bindingInfo.Type))
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			SPALL_TRY(validateBindingStages(bindingInfo.Stages));

			constexpr ShaderStageFlags storageStages = ShaderStageFlags::Compute | ShaderStageFlags::Fragment |
				ShaderStageFlags::RayGeneration | ShaderStageFlags::Miss | ShaderStageFlags::ClosestHit |
				ShaderStageFlags::AnyHit | ShaderStageFlags::Intersection;

			if (((bindingInfo.Type == ResourceBindingType::StorageBuffer) or
					(bindingInfo.Type == ResourceBindingType::StorageTexture)) and
				(not hasOnlyKnownFlags(bindingInfo.Stages, storageStages)))
			{
				return ERR_UNSUPPORTED_SHADER_STAGE;
			}
		}

		return {};
	}

	inline Status validateResourceSetCreateInfo(
		const ResourceSetCreateInfo& info)
	{
		if (info.Layout == nullptr)
		{
			return ERR_INVALID_RESOURCE;
		}

		return {};
	}

	inline Status validateResourceWrites(
		std::span<const ResourceWrite> writes)
	{
		if (writes.empty())
		{
			return ERR_INVALID_BINDING;
		}

		for (std::size_t writeIndex = 0; writeIndex < writes.size(); ++writeIndex)
		{
			const ResourceWrite& write = writes[writeIndex];

			for (std::size_t compareIndex = writeIndex + 1; compareIndex < writes.size(); ++compareIndex)
			{
				if (write.Binding == writes[compareIndex].Binding)
				{
					return ERR_INVALID_BINDING;
				}
			}

			if (not isSupportedBindingType(write.Type))
			{
				return ERR_UNSUPPORTED_USAGE;
			}

			if ((write.Type == ResourceBindingType::UniformBuffer) and (write.Buffer == nullptr))
			{
				return ERR_INVALID_RESOURCE;
			}

			if ((write.Type == ResourceBindingType::StorageBuffer) and (write.Buffer == nullptr))
			{
				return ERR_INVALID_RESOURCE;
			}

			if (write.Type == ResourceBindingType::SampledTexture)
			{
				if ((write.TextureView == nullptr) or (write.Sampler == nullptr))
				{
					return ERR_INVALID_RESOURCE;
				}

				const TextureInfo textureInfo = write.TextureView->texture().info();

				if ((textureInfo.Usage & TextureUsageFlags::Sampled) == TextureUsageFlags::None)
				{
					return ERR_INVALID_USAGE_FLAGS;
				}
			}

			if (write.Type == ResourceBindingType::StorageTexture)
			{
				if ((write.TextureView == nullptr) or (write.Sampler != nullptr))
				{
					return ERR_INVALID_RESOURCE;
				}

				const TextureInfo textureInfo = write.TextureView->texture().info();

				if (((textureInfo.Usage & TextureUsageFlags::Storage) == TextureUsageFlags::None) or
					(write.TextureView->aspects() != TextureAspectFlags::Color) or
					(write.TextureView->mipLevels() != 1))
				{
					return ERR_INVALID_USAGE_FLAGS;
				}
			}

			if ((write.Type == ResourceBindingType::AccelerationStructure) and (write.AccelerationStructure == nullptr))
			{
				return ERR_INVALID_RESOURCE;
			}
		}

		return {};
	}
} // namespace spall
