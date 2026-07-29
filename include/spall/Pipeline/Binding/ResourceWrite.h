#pragma once

#include <spall/Common/Enums/PipelineEnums.h>
#include <spall/Pipeline/Binding/ResourceBindingInfo.h>

#include <cstdint>

namespace spall
{
	class IAccelerationStructure;
	class IBuffer;
	class ISampler;
	class ITextureView;

	/// Assigns one resource to a declared resource-set binding.
	///
	/// Only the fields required by Type are read.
	struct ResourceWrite
	{
		std::uint32_t Binding = 0;
		ResourceBindingType Type = ResourceBindingType::UniformBuffer;

		/// Used by uniform-buffer and storage-buffer bindings.
		IBuffer* Buffer = nullptr;

		/// Used by sampled-texture and storage-texture bindings.
		ITextureView* TextureView = nullptr;

		/// Used together with TextureView by sampled-texture bindings.
		ISampler* Sampler = nullptr;

		/// Used by acceleration-structure bindings.
		IAccelerationStructure* AccelerationStructure = nullptr;
	};
} // namespace spall
