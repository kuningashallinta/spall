// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#include <limits>

namespace spall
{
	inline Resource<ITexture1D> IResourceFactory::createTexture1D(
		const Texture1DCreateInfo& createInfo)
	{
		Resource<ITexture1D> texture;
		createTexture1D(createInfo, &texture);
		return texture;
	}

	inline Resource<ITexture2D> IResourceFactory::createTexture2D(
		const Texture2DCreateInfo& createInfo)
	{
		Resource<ITexture2D> texture;
		createTexture2D(createInfo, &texture);
		return texture;
	}

	inline Resource<ITexture3D> IResourceFactory::createTexture3D(
		const Texture3DCreateInfo& createInfo)
	{
		Resource<ITexture3D> texture;
		createTexture3D(createInfo, &texture);
		return texture;
	}

	inline Resource<ITextureView> IResourceFactory::createTextureView(
		const TextureViewCreateInfo& info)
	{
		Resource<ITextureView> textureView;
		createTextureView(info, &textureView);
		return textureView;
	}

	inline Resource<IFramebuffer> IResourceFactory::createFramebuffer(
		const FramebufferCreateInfo& createInfo)
	{
		Resource<IFramebuffer> framebuffer;
		createFramebuffer(createInfo, &framebuffer);
		return framebuffer;
	}

	inline Resource<IBuffer> IResourceFactory::createBuffer(
		const BufferCreateInfo& createInfo)
	{
		Resource<IBuffer> buffer;
		createBuffer(createInfo, &buffer);
		return buffer;
	}

	inline Resource<IBuffer> IResourceFactory::createBufferWithData(
		BufferCreateInfo createInfo,
		std::span<const std::byte> data)
	{
		if (createInfo.Size == 0)
		{
			if (data.size_bytes() > (std::numeric_limits<std::uint32_t>::max)())
			{
				return {};
			}

			createInfo.Size = static_cast<std::uint32_t>(data.size_bytes());
		}

		Resource<IBuffer> buffer;
		createBufferWithData(createInfo, data, &buffer);
		return buffer;
	}

	template <typename T, std::size_t Extent>
		requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
	inline Resource<IBuffer> IResourceFactory::createBufferWithData(
		BufferCreateInfo createInfo,
		std::span<const T, Extent> data)
	{
		return createBufferWithData(createInfo, std::as_bytes(data));
	}

	template <typename T, std::size_t Size>
		requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
	inline Resource<IBuffer> IResourceFactory::createBufferWithData(
		BufferCreateInfo createInfo,
		const T (&data)[Size])
	{
		return createBufferWithData(createInfo, std::span {data});
	}

	template <typename T, std::size_t Extent>
		requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
	inline Status IResourceFactory::writeBuffer(
		IBuffer& buffer,
		std::span<const T, Extent> data,
		std::uint32_t offset)
	{
		return writeBuffer(buffer, std::as_bytes(data), offset);
	}

	template <typename T, std::size_t Size>
		requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
	inline Status IResourceFactory::writeBuffer(
		IBuffer& buffer,
		const T (&data)[Size],
		std::uint32_t offset)
	{
		return writeBuffer(buffer, std::span {data}, offset);
	}

	template <typename T, std::size_t Extent>
		requires std::is_trivially_copyable_v<T> and (not std::is_const_v<T>) and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
	inline Status IResourceFactory::readBuffer(
		IBuffer& buffer,
		std::span<T, Extent> data,
		std::uint32_t offset)
	{
		return readBuffer(buffer, std::as_writable_bytes(data), offset);
	}

	template <typename T, std::size_t Size>
		requires std::is_trivially_copyable_v<T> and (not std::is_const_v<T>) and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
	inline Status IResourceFactory::readBuffer(
		IBuffer& buffer,
		T (&data)[Size],
		std::uint32_t offset)
	{
		return readBuffer(buffer, std::span {data}, offset);
	}

	inline Resource<ISampler> IResourceFactory::createSampler(
		const SamplerCreateInfo& info)
	{
		Resource<ISampler> sampler;
		createSampler(info, &sampler);
		return sampler;
	}

	inline Resource<IQueryPool> IResourceFactory::createQueryPool(
		const QueryPoolCreateInfo& info)
	{
		Resource<IQueryPool> queryPool;
		createQueryPool(info, &queryPool);
		return queryPool;
	}

	inline Resource<IAccelerationStructure> IResourceFactory::createAccelerationStructure(
		const AccelerationStructureCreateInfo& info)
	{
		Resource<IAccelerationStructure> accelerationStructure;
		createAccelerationStructure(info, &accelerationStructure);
		return accelerationStructure;
	}
} // namespace spall
