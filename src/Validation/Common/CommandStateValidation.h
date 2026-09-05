// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Status/Status.h>
#include <spall/Pipeline/VertexInput/VertexBindingInfo.h>

#include <concepts>
#include <cstdint>
#include <span>

namespace spall
{
	struct BoundVertexBuffer
	{
		const void* Resource = nullptr;
		std::uint32_t Stride = 0;
	};

	template <typename SlotType>
	concept VertexBufferSlot = requires(const SlotType slot) {
		{ slot.Resource == nullptr } -> std::convertible_to<bool>;
		{ slot.Stride } -> std::convertible_to<std::uint32_t>;
	};

	inline Status validateRecordingState(
		bool isRecording,
		bool expectedRecording);

	inline Status validateDebugLabel(const char* label);

	inline Status validateDispatchState(
		bool renderPassActive,
		bool hasComputePipeline);

	template <VertexBufferSlot SlotType>
	inline Status validateDrawState(
		bool renderPassActive,
		bool hasGraphicsPipeline,
		bool viewportSet,
		bool scissorSet,
		std::span<const VertexBindingInfo> pipelineBindings,
		std::span<const SlotType> boundSlots,
		bool hasIndexBuffer,
		bool indexed);

	inline Status validateNoSampledAttachmentAliasing(
		std::span<const void* const> sampledTextures,
		std::span<const void* const> attachmentTextures);
} // namespace spall

#include <src/Validation/Common/CommandStateValidation.inl>
