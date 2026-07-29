#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/CommandList/IndirectCommands.h>
#include <spall/Common/Color/Color.h>
#include <spall/Common/Enums/ResourceEnums.h>
#include <spall/Common/Enums/ResourceStateFlags.h>
#include <spall/Common/Enums/ShaderStageFlags.h>
#include <spall/Common/Scissor/Scissor.h>
#include <spall/Common/Status/Status.h>
#include <spall/Common/Viewport/Viewport.h>
#include <spall/RenderPass/RenderPassBeginInfo.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureBuildInfo.h>
#include <spall/Resources/Texture/TextureRegion.h>
#include <spall/Resources/Texture/TextureSubresourceRange.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace spall
{
	class IAccelerationStructure;
	class IBuffer;
	class IPipeline;
	class IQueryPool;
	class IResourceSet;
	class ITexture;

	/// Records GPU work for later submission to a graphics queue.
	/// For portable reuse, start a fresh begin and end recording before each submission.
	class ICommandList : public IResource
	{
	public:
		/// Starts a fresh recording, discarding any previous one.
		/// A rejected call leaves the recording intact; a backend failure invalidates the list, and begin makes it recordable again.
		/// Explicit backends return InvalidState while the previous submission is still pending.
		virtual Status begin(void) = 0;

		/// Finishes the current recording, making it eligible for submission.
		virtual Status end(void) = 0;

		/// Starts a nested UTF-8 debug label. A zero-alpha color lets tooling choose its display color.
		virtual Status pushDebugGroup(
			const char* label,
			Color color = {}) = 0;

		/// Ends the innermost debug label. Groups must be balanced before end.
		virtual Status popDebugGroup(void) = 0;

		/// Inserts a standalone UTF-8 debug label into the command stream.
		virtual Status insertDebugMarker(
			const char* label,
			Color color = {}) = 0;

		virtual Status beginRenderPass(const RenderPassBeginInfo& beginInfo) = 0;

		virtual Status endRenderPass(void) = 0;

		virtual Status setViewport(const Viewport& viewport) = 0;

		virtual Status setScissor(const Scissor& scissor) = 0;

		/// Overrides the bound graphics pipeline's default stencil reference until another graphics pipeline is bound.
		virtual Status setStencilReference(std::uint8_t reference) = 0;

		virtual Status setVertexBuffer(
			std::uint32_t slot,
			IBuffer& buffer,
			std::uint32_t stride,
			std::uint32_t offset) = 0;

		virtual Status setIndexBuffer(
			IBuffer& buffer,
			IndexFormat format,
			std::uint32_t offset) = 0;

		virtual Status bindGraphicsPipeline(IPipeline& pipeline) = 0;

		virtual Status bindComputePipeline(IPipeline& pipeline) = 0;

		virtual Status bindRayTracingPipeline(IPipeline& pipeline) = 0;

		virtual Status bindResourceSet(
			std::uint32_t slot,
			IResourceSet& resourceSet) = 0;

		/// Updates part of the declared push-constant block. The stage mask must match the pipeline declaration.
		virtual Status setPushConstants(
			ShaderStageFlags stages,
			std::uint32_t offset,
			std::span<const std::byte> data) = 0;

		template <typename T, std::size_t Extent>
			requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
		Status setPushConstants(
			ShaderStageFlags stages,
			std::uint32_t offset,
			std::span<const T, Extent> data);

		template <typename T>
			requires std::is_trivially_copyable_v<T>
		Status setPushConstants(
			ShaderStageFlags stages,
			std::uint32_t offset,
			const T& data);

		virtual Status setEnableAutomaticBarriers(bool enable) = 0;

		/// Starts tracking a buffer in a known state without recording a transition.
		virtual Status beginTrackingBufferState(
			IBuffer& buffer,
			ResourceStateFlags state) = 0;

		/// Starts tracking texture subresources in a known state without recording a transition.
		virtual Status beginTrackingTextureState(
			ITexture& texture,
			ResourceStateFlags state,
			const TextureSubresourceRange& subresources = {}) = 0;

		/// Requests a transition of a buffer to the given state.
		virtual Status setBufferState(
			IBuffer& buffer,
			ResourceStateFlags state) = 0;

		/// Requests a transition of texture subresources to the given state.
		virtual Status setTextureState(
			ITexture& texture,
			ResourceStateFlags state,
			const TextureSubresourceRange& subresources = {}) = 0;

		/// Transitions a buffer and makes the resulting state persistent across submissions.
		virtual Status setPermanentBufferState(
			IBuffer& buffer,
			ResourceStateFlags state) = 0;

		/// Transitions a texture and makes the resulting state persistent across submissions.
		virtual Status setPermanentTextureState(
			ITexture& texture,
			ResourceStateFlags state) = 0;

		/// Records all pending resource-state transitions at the current command-stream position.
		virtual Status commitBarriers(void) = 0;

		/// Returns the buffer state currently tracked by this recording, or Unknown when unavailable.
		virtual ResourceStateFlags bufferState(IBuffer& buffer) const = 0;

		/// Returns the common tracked state of the texture subresources, or Unknown when unavailable or mixed.
		virtual ResourceStateFlags textureState(
			ITexture& texture,
			const TextureSubresourceRange& subresources = {}) const = 0;

		virtual Status draw(
			std::uint32_t vertexCount,
			std::uint32_t startVertex,
			std::uint32_t instanceCount = 1,
			std::uint32_t startInstance = 0) = 0;

		virtual Status drawIndexed(
			std::uint32_t indexCount,
			std::uint32_t startIndex,
			std::int32_t vertexOffset,
			std::uint32_t instanceCount = 1,
			std::uint32_t startInstance = 0) = 0;

		virtual Status dispatch(
			std::uint32_t groupCountX,
			std::uint32_t groupCountY,
			std::uint32_t groupCountZ) = 0;

		virtual Status dispatchRays(
			std::uint32_t width,
			std::uint32_t height,
			std::uint32_t depth) = 0;

		/// Draws using one DrawIndirectCommand read from the argument buffer.
		virtual Status drawIndirect(
			IBuffer& argumentBuffer,
			std::uint32_t offset) = 0;

		/// Draws using one DrawIndexedIndirectCommand read from the argument buffer.
		virtual Status drawIndexedIndirect(
			IBuffer& argumentBuffer,
			std::uint32_t offset) = 0;

		/// Dispatches using one DispatchIndirectCommand read from the argument buffer.
		virtual Status dispatchIndirect(
			IBuffer& argumentBuffer,
			std::uint32_t offset) = 0;

		/// Builds or refits an acceleration structure and inserts the barrier its readers need.
		///
		/// The structure is built from the description it was created with, so
		/// its input buffers are transitioned automatically. Cannot be recorded
		/// inside a render pass.
		virtual Status buildAccelerationStructure(
			IAccelerationStructure& accelerationStructure,
			const AccelerationStructureBuildInfo& buildInfo = {}) = 0;

		/// Compacts a built structure in place.
		///
		/// Requires AllowCompaction and a completed measured build. Compaction
		/// changes deviceAddress, so referencing instances must be rewritten and
		/// their top-level structures rebuilt. It is invalid inside a render pass
		/// or on an already compacted structure.
		virtual Status compactAccelerationStructure(
			IAccelerationStructure& accelerationStructure) = 0;

		virtual Status copyBuffer(
			IBuffer& destination,
			std::uint32_t destinationOffset,
			IBuffer& source,
			std::uint32_t sourceOffset,
			std::uint32_t size) = 0;

		virtual Status copyBufferToTexture(
			ITexture& destination,
			const TextureRegion& region,
			IBuffer& source,
			std::uint32_t sourceOffset,
			std::uint32_t sourceRowPitch) = 0;

		virtual Status copyTextureToBuffer(
			IBuffer& destination,
			std::uint32_t destinationOffset,
			std::uint32_t destinationRowPitch,
			ITexture& source,
			const TextureRegion& region) = 0;

		/// Generates every mip level after level zero by linear filtering.
		///
		/// The texture must be a color texture with more than one mip level and
		/// Sampled, TransferSource, and TransferDestination usage.
		virtual Status generateMips(ITexture& texture) = 0;

		virtual Status copyTexture(
			ITexture& destination,
			ITexture& source) = 0;

		/// Writes the GPU timestamp after all preceding work into one query slot.
		virtual Status writeTimestamp(
			IQueryPool& queryPool,
			std::uint32_t query) = 0;
	};
} // namespace spall

#include <spall/CommandList/ICommandList.inl>
