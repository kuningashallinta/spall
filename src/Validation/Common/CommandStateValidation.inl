// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	inline Status validateRecordingState(
		bool isRecording,
		bool expectedRecording)
	{
		if (isRecording != expectedRecording)
		{
			return ERR_INVALID_STATE;
		}

		return {};
	}

	inline Status validateDebugLabel(
		const char* label)
	{
		if ((label == nullptr) or (label[0] == '\0'))
		{
			return ERR_INVALID_ARGUMENT;
		}

		return {};
	}

	inline Status validateDispatchState(
		bool renderPassActive,
		bool hasComputePipeline)
	{
		if (renderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (not hasComputePipeline)
		{
			return ERR_INVALID_STATE;
		}

		return {};
	}

	template <VertexBufferSlot SlotType>
	inline Status validateDrawState(
		bool renderPassActive,
		bool hasGraphicsPipeline,
		bool viewportSet,
		bool scissorSet,
		std::span<const VertexBindingInfo> pipelineBindings,
		std::span<const SlotType> boundSlots,
		bool hasIndexBuffer,
		bool indexed)
	{
		if (not renderPassActive)
		{
			return ERR_INVALID_STATE;
		}

		if (not hasGraphicsPipeline)
		{
			return ERR_INVALID_STATE;
		}

		if ((not viewportSet) or (not scissorSet))
		{
			return ERR_INVALID_STATE;
		}

		for (const VertexBindingInfo& binding : pipelineBindings)
		{
			if ((binding.Binding >= boundSlots.size()) or (boundSlots[binding.Binding].Resource == nullptr))
			{
				return ERR_INVALID_BINDING;
			}

			if (boundSlots[binding.Binding].Stride != binding.Stride)
			{
				return ERR_INVALID_BINDING;
			}
		}

		if (indexed and (not hasIndexBuffer))
		{
			return ERR_INVALID_BINDING;
		}

		return {};
	}

	inline Status validateNoSampledAttachmentAliasing(
		std::span<const void* const> sampledTextures,
		std::span<const void* const> attachmentTextures)
	{
		for (const void* sampledTexture : sampledTextures)
		{
			for (const void* attachmentTexture : attachmentTextures)
			{
				if ((sampledTexture != nullptr) and (sampledTexture == attachmentTexture))
				{
					return ERR_INVALID_RESOURCE_STATE;
				}
			}
		}

		return {};
	}
} // namespace spall
