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

	spall::RenderBackendType backendType(
		void) const override
	{
		return spall::RenderBackendType::Vulkan;
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
class FakeTexture final : public FakeResource<spall::ITexture, spall::TextureInfo>
{
public:
	using FakeResource::FakeResource;

	spall::TextureType type(
		void) const override
	{
		return info().Type;
	}
};

class FakeBuffer final : public FakeResource<spall::IBuffer, spall::BufferInfo>
{
public:
	using FakeResource::FakeResource;
};

class FakeFramebuffer final : public FakeResource<spall::IFramebuffer, spall::FramebufferInfo>
{
public:
	using FakeResource::FakeResource;
};

class FakeQueryPool final : public FakeResource<spall::IQueryPool, spall::QueryPoolInfo>
{
public:
	using FakeResource::FakeResource;
};

class FakeAccelerationStructure final : public FakeResource<spall::IAccelerationStructure, spall::AccelerationStructureInfo>
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

	spall::RenderBackendType backendType(
		void) const override
	{
		return spall::RenderBackendType::Vulkan;
	}

private:
	std::uint32_t m_ReferenceCount = 1;
};

class FakeSampler final : public FakeMarker<spall::ISampler>
{
};

class FakeResourceSetLayout final : public FakeMarker<spall::IResourceSetLayout>
{
};

class FakeShader final : public FakeMarker<spall::IShader>
{
};

class FakeTextureView final : public spall::ITextureView
{
public:
	FakeTextureView(
		spall::ITexture& texture,
		spall::TextureAspectFlags aspects,
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

	spall::RenderBackendType backendType(
		void) const override
	{
		return spall::RenderBackendType::Vulkan;
	}

	spall::ITexture& texture(
		void) const override
	{
		return *m_Texture;
	}

	spall::Format format(
		void) const override
	{
		return m_Texture->info().Format;
	}

	spall::TextureAspectFlags aspects(
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
	spall::ITexture* m_Texture = nullptr;
	spall::TextureAspectFlags m_Aspects = spall::TextureAspectFlags::None;
	std::uint32_t m_BaseMipLevel = 0;
	std::uint32_t m_MipLevels = 1;
	std::uint32_t m_BaseArrayLayer = 0;
	std::uint32_t m_ArrayLayers = 1;
	bool m_Cubemap = false;
	std::uint32_t m_ReferenceCount = 1;
};
