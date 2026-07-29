#pragma once

#include <spall/Common/Enums/RenderBackendType.h>
#include <spall/Framebuffer/IFramebuffer.h>
#include <spall/Pipeline/Binding/IResourceSetLayout.h>
#include <spall/Pipeline/Shader/IShader.h>
#include <spall/Resources/AccelerationStructure/IAccelerationStructure.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <spall/Resources/Query/IQueryPool.h>
#include <spall/Resources/Sampler/ISampler.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/TextureView/ITextureView.h>

#include <cstdint>

namespace spall::tests
{
	template <typename Interface, typename Info>
	class FakeResource : public Interface
	{
	public:
		explicit FakeResource(
			const Info& info)
			: m_Info(info)
		{
		}

		std::uint32_t addRef(
			void) override
		{
			return ++m_ReferenceCount;
		}

		std::uint32_t release(
			void) override
		{
			return --m_ReferenceCount;
		}

		RenderBackendType backendType(
			void) const override
		{
			return RenderBackendType::Vulkan;
		}

		Info info(
			void) const override
		{
			return m_Info;
		}

		std::uint32_t referenceCount(
			void) const
		{
			return m_ReferenceCount;
		}

	private:
		Info m_Info;
		std::uint32_t m_ReferenceCount = 1;
	};

	class FakeTexture final : public FakeResource<ITexture, TextureInfo>
	{
	public:
		using FakeResource::FakeResource;
	};

	class FakeBuffer final : public FakeResource<IBuffer, BufferInfo>
	{
	public:
		using FakeResource::FakeResource;
	};

	class FakeFramebuffer final : public FakeResource<IFramebuffer, FramebufferInfo>
	{
	public:
		using FakeResource::FakeResource;
	};

	class FakeQueryPool final : public FakeResource<IQueryPool, QueryPoolInfo>
	{
	public:
		using FakeResource::FakeResource;
	};

	class FakeAccelerationStructure final : public FakeResource<IAccelerationStructure, AccelerationStructureInfo>
	{
	public:
		using FakeResource::FakeResource;

		std::uint64_t deviceAddress(
			void) const override
		{
			return 0x1000;
		}
	};

	template <typename Interface>
	class FakeMarker : public Interface
	{
	public:
		std::uint32_t addRef(
			void) override
		{
			return ++m_ReferenceCount;
		}

		std::uint32_t release(
			void) override
		{
			return --m_ReferenceCount;
		}

		RenderBackendType backendType(
			void) const override
		{
			return RenderBackendType::Vulkan;
		}

	private:
		std::uint32_t m_ReferenceCount = 1;
	};

	class FakeSampler final : public FakeMarker<ISampler>
	{
	};

	class FakeResourceSetLayout final : public FakeMarker<IResourceSetLayout>
	{
	};

	class FakeShader final : public FakeMarker<IShader>
	{
	};

	class FakeTextureView final : public ITextureView
	{
	public:
		FakeTextureView(
			ITexture& texture,
			TextureAspectFlags aspects,
			std::uint32_t baseMipLevel = 0,
			std::uint32_t mipLevels = 1,
			std::uint32_t baseArrayLayer = 0,
			std::uint32_t arrayLayers = 1,
			bool cubemap = false)
			: m_Texture(&texture), m_Aspects(aspects), m_BaseMipLevel(baseMipLevel), m_MipLevels(mipLevels), m_BaseArrayLayer(baseArrayLayer), m_ArrayLayers(arrayLayers), m_Cubemap(cubemap)
		{
		}

		std::uint32_t addRef(
			void) override
		{
			return ++m_ReferenceCount;
		}

		std::uint32_t release(
			void) override
		{
			return --m_ReferenceCount;
		}

		RenderBackendType backendType(
			void) const override
		{
			return RenderBackendType::Vulkan;
		}

		ITexture& texture(
			void) const override
		{
			return *m_Texture;
		}

		spall::Format format(
			void) const override
		{
			return m_Texture->info().Format;
		}

		TextureAspectFlags aspects(
			void) const override
		{
			return m_Aspects;
		}

		std::uint32_t baseMipLevel(
			void) const override
		{
			return m_BaseMipLevel;
		}

		std::uint32_t mipLevels(
			void) const override
		{
			return m_MipLevels;
		}

		std::uint32_t baseArrayLayer(
			void) const override
		{
			return m_BaseArrayLayer;
		}

		std::uint32_t arrayLayers(
			void) const override
		{
			return m_ArrayLayers;
		}

		bool isCubemap(
			void) const override
		{
			return m_Cubemap;
		}

	private:
		ITexture* m_Texture = nullptr;
		TextureAspectFlags m_Aspects = TextureAspectFlags::None;
		std::uint32_t m_BaseMipLevel = 0;
		std::uint32_t m_MipLevels = 1;
		std::uint32_t m_BaseArrayLayer = 0;
		std::uint32_t m_ArrayLayers = 1;
		bool m_Cubemap = false;
		std::uint32_t m_ReferenceCount = 1;
	};

	inline TextureInfo textureInfo(
		TextureUsageFlags usage,
		std::uint32_t mipLevels = 1,
		std::uint32_t arrayLayers = 1,
		bool cubemap = false)
	{
		TextureInfo info = {};
		info.Width = 64;
		info.Height = 64;
		info.MipLevels = mipLevels;
		info.ArrayLayers = arrayLayers;
		info.Cubemap = cubemap;
		info.Format = spall::Format::RGBA8;
		info.Usage = usage;

		return info;
	}

	inline QueryPoolInfo queryPoolInfo(
		std::uint32_t timestampCount = 8)
	{
		QueryPoolInfo info = {};
		info.TimestampCount = timestampCount;

		return info;
	}

	inline BufferInfo bufferInfo(
		BufferUsageFlags usage,
		std::uint32_t size = 256)
	{
		BufferInfo info = {};
		info.Size = size;
		info.Usage = usage;

		return info;
	}

	inline AccelerationStructureInfo accelerationStructureInfo(
		AccelerationStructureType type,
		AccelerationStructureBuildFlags flags = AccelerationStructureBuildFlags::PreferFastTrace,
		std::uint32_t instanceCount = 0)
	{
		AccelerationStructureInfo info = {};
		info.Type = type;
		info.Flags = flags;
		info.Size = 1024;
		info.BuildScratchSize = 512;
		info.GeometryCount = (type == AccelerationStructureType::BottomLevel) ? 1 : 0;
		info.InstanceCount = instanceCount;

		return info;
	}
} // namespace spall::tests
