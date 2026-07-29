#pragma once

#include <cstdint>

namespace spall
{
	/// Describes the effective limits of a graphics device.
	///
	/// Native limits are restricted where Spall RHI exposes a smaller portable
	/// limit. The values remain constant for the lifetime of the device.
	struct DeviceLimits
	{
		std::uint32_t MaxTexture2DDimension = 0;
		std::uint32_t MaxFramebufferWidth = 0;
		std::uint32_t MaxFramebufferHeight = 0;
		std::uint32_t MaxUniformBufferSize = 0;
		std::uint32_t MaxVertexBuffers = 0;
		std::uint32_t MaxVertexAttributes = 0;
		std::uint32_t MaxVertexBufferStride = 0;
		std::uint32_t MaxUniformBuffersPerStage = 0;

		/// Maximum number of combined texture and sampler bindings per shader stage.
		std::uint32_t MaxSampledTexturesPerStage = 0;

		std::uint32_t MaxComputeStorageBuffers = 0;
		std::uint32_t MaxComputeStorageTextures = 0;
		std::uint32_t MaxColorAttachments = 0;
		std::uint32_t MaxResourceSets = 0;

		/// Maximum number of bytes available to a push-constant block.
		std::uint32_t MaxPushConstantSize = 0;

		/// Maximum dispatch group counts for the X, Y, and Z dimensions.
		std::uint32_t MaxComputeWorkGroupCount[3] = {};

		/// Maximum work-group sizes for the X, Y, and Z dimensions.
		std::uint32_t MaxComputeWorkGroupSize[3] = {};

		std::uint32_t MaxComputeWorkGroupInvocations = 0;

		/// Mask of supported framebuffer sample counts, tested against the count itself.
		std::uint32_t SupportedSampleCounts = 1;

		bool SupportsTimestampQueries = false;

		/// Reports support for acceleration structures and shader-side inline ray queries.
		bool SupportsInlineRayTracing = false;

		/// Reports support for ray-tracing pipelines and dispatchRays.
		bool SupportsRayTracingPipeline = false;

		/// Deepest TraceRay nesting a pipeline may declare, or zero when unsupported.
		std::uint32_t MaxRayRecursionDepth = 0;

		float MaxSamplerAnisotropy = 1.0f;
		float MinLineWidth = 1.0f;
		float MaxLineWidth = 1.0f;
	};
} // namespace spall
