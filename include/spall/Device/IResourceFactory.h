#pragma once

#include <spall/Common/Resource/Resource.h>
#include <spall/Common/Status/Status.h>
#include <spall/Framebuffer/FramebufferCreateInfo.h>
#include <spall/Framebuffer/IFramebuffer.h>
#include <spall/Resources/AccelerationStructure/AccelerationStructureCreateInfo.h>
#include <spall/Resources/AccelerationStructure/IAccelerationStructure.h>
#include <spall/Resources/Buffer/BufferCreateInfo.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <spall/Resources/Query/IQueryPool.h>
#include <spall/Resources/Query/QueryPoolCreateInfo.h>
#include <spall/Resources/Sampler/ISampler.h>
#include <spall/Resources/Sampler/SamplerCreateInfo.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/Texture/TextureCreateInfo.h>
#include <spall/Resources/TextureView/ITextureView.h>
#include <spall/Resources/TextureView/TextureViewCreateInfo.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <type_traits>

namespace spall
{
	/// Creates resources owned by a graphics device.
	///
	/// Convenience overloads return an empty resource on failure; Status
	/// overloads preserve detailed failure information.
	class IResourceFactory
	{
	public:
		virtual ~IResourceFactory(void) = default;

		Resource<ITexture> createTexture(
			const TextureCreateInfo& createInfo);

		virtual Status createTexture(
			const TextureCreateInfo& createInfo,
			Resource<ITexture>* texture) = 0;

		Resource<ITextureView> createTextureView(
			const TextureViewCreateInfo& info);

		virtual Status createTextureView(
			const TextureViewCreateInfo& info,
			Resource<ITextureView>* textureView) = 0;

		Resource<IFramebuffer> createFramebuffer(
			const FramebufferCreateInfo& createInfo);

		virtual Status createFramebuffer(
			const FramebufferCreateInfo& createInfo,
			Resource<IFramebuffer>* framebuffer) = 0;

		Resource<IBuffer> createBuffer(
			const BufferCreateInfo& createInfo);

		virtual Status createBuffer(
			const BufferCreateInfo& createInfo,
			Resource<IBuffer>* buffer) = 0;

		/// Creates a buffer and initializes its first bytes from data.
		/// A zero Size is inferred from the supplied data.
		Resource<IBuffer> createBufferWithData(
			BufferCreateInfo createInfo,
			std::span<const std::byte> data);

		template <typename T, std::size_t Extent>
			requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
		Resource<IBuffer> createBufferWithData(
			BufferCreateInfo createInfo,
			std::span<const T, Extent> data);

		template <typename T, std::size_t Size>
			requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
		Resource<IBuffer> createBufferWithData(
			BufferCreateInfo createInfo,
			const T (&data)[Size]);

		virtual Status createBufferWithData(
			const BufferCreateInfo& createInfo,
			std::span<const std::byte> data,
			Resource<IBuffer>* buffer) = 0;

		template <typename T, std::size_t Extent>
			requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
		Status writeBuffer(
			IBuffer& buffer,
			std::span<const T, Extent> data,
			std::uint32_t offset = 0);

		template <typename T, std::size_t Size>
			requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
		Status writeBuffer(
			IBuffer& buffer,
			const T (&data)[Size],
			std::uint32_t offset = 0);

		virtual Status writeBuffer(
			IBuffer& buffer,
			std::span<const std::byte> data,
			std::uint32_t offset) = 0;

		template <typename T, std::size_t Extent>
			requires std::is_trivially_copyable_v<T> and (not std::is_const_v<T>) and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
		Status readBuffer(
			IBuffer& buffer,
			std::span<T, Extent> data,
			std::uint32_t offset = 0);

		template <typename T, std::size_t Size>
			requires std::is_trivially_copyable_v<T> and (not std::is_const_v<T>) and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
		Status readBuffer(
			IBuffer& buffer,
			T (&data)[Size],
			std::uint32_t offset = 0);

		virtual Status readBuffer(
			IBuffer& buffer,
			std::span<std::byte> data,
			std::uint32_t offset) = 0;

		Resource<ISampler> createSampler(
			const SamplerCreateInfo& info);

		virtual Status createSampler(
			const SamplerCreateInfo& info,
			Resource<ISampler>* sampler) = 0;

		Resource<IQueryPool> createQueryPool(
			const QueryPoolCreateInfo& info);

		virtual Status createQueryPool(
			const QueryPoolCreateInfo& info,
			Resource<IQueryPool>* queryPool) = 0;

		/// Reads timestamps as nanoseconds, returning NotReady until the writing submission completes.
		virtual Status readTimestamps(
			IQueryPool& queryPool,
			std::uint32_t firstQuery,
			std::span<std::uint64_t> nanoseconds) = 0;

		/// The structure is sized from the supplied description and is undefined
		/// until a recorded build completes.
		Resource<IAccelerationStructure> createAccelerationStructure(
			const AccelerationStructureCreateInfo& info);

		virtual Status createAccelerationStructure(
			const AccelerationStructureCreateInfo& info,
			Resource<IAccelerationStructure>* accelerationStructure) = 0;
	};
} // namespace spall

#include <spall/Device/IResourceFactory.inl>
