#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/ResourceStateValidation.h>

namespace
{
	spall::BufferInfo bufferInfo(
		spall::BufferUsageFlags usage)
	{
		spall::BufferInfo info = {};
		info.Size = 256;
		info.Usage = usage;

		return info;
	}

	spall::TextureInfo textureInfo(
		spall::TextureUsageFlags usage)
	{
		spall::TextureInfo info = {};
		info.Width = 64;
		info.Height = 64;
		info.Format = spall::Format::RGBA8;
		info.Usage = usage;

		return info;
	}

	constexpr spall::TextureUsageFlags EveryTextureUsage =
		spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::TransferSource |
		spall::TextureUsageFlags::TransferDestination | spall::TextureUsageFlags::Sampled |
		spall::TextureUsageFlags::Storage;

	constexpr spall::BufferUsageFlags EveryBufferUsage =
		spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::Index | spall::BufferUsageFlags::Uniform |
		spall::BufferUsageFlags::TransferSource | spall::BufferUsageFlags::TransferDestination |
		spall::BufferUsageFlags::Storage;
} // namespace

TEST_CASE(
	"A buffer reaches Common regardless of usage",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Vertex), spall::ResourceStateFlags::Common) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::None), spall::ResourceStateFlags::Common) == spall::SUCCESS);
}

TEST_CASE(
	"A buffer rejects the unknown state",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(bufferInfo(EveryBufferUsage), spall::ResourceStateFlags::Unknown) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer never combines Common with another state",
	"[state][buffer]")
{
	const spall::ResourceStateFlags combined = spall::ResourceStateFlags::Common | spall::ResourceStateFlags::VertexBuffer;

	CHECK(spall::validateBufferResourceState(bufferInfo(EveryBufferUsage), combined) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer state requires its matching usage",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Index), spall::ResourceStateFlags::VertexBuffer) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Vertex), spall::ResourceStateFlags::IndexBuffer) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Vertex), spall::ResourceStateFlags::ConstantBuffer) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Vertex), spall::ResourceStateFlags::ShaderResource) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Vertex), spall::ResourceStateFlags::UnorderedAccess) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Vertex), spall::ResourceStateFlags::CopySource) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Vertex), spall::ResourceStateFlags::CopyDest) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer state is reachable with its matching usage",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Vertex), spall::ResourceStateFlags::VertexBuffer) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Index), spall::ResourceStateFlags::IndexBuffer) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Uniform), spall::ResourceStateFlags::ConstantBuffer) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Storage), spall::ResourceStateFlags::ShaderResource) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Storage), spall::ResourceStateFlags::UnorderedAccess) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::TransferSource), spall::ResourceStateFlags::CopySource) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::TransferDestination), spall::ResourceStateFlags::CopyDest) == spall::SUCCESS);
}

TEST_CASE(
	"A buffer combines states when every usage is present",
	"[state][buffer]")
{
	const spall::ResourceStateFlags combined =
		spall::ResourceStateFlags::VertexBuffer |
		spall::ResourceStateFlags::IndexBuffer |
		spall::ResourceStateFlags::CopySource;

	CHECK(spall::validateBufferResourceState(bufferInfo(EveryBufferUsage), combined) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::Index), combined) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer cannot transition to a texture-only state",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(bufferInfo(EveryBufferUsage), spall::ResourceStateFlags::RenderTarget) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(EveryBufferUsage), spall::ResourceStateFlags::DepthWrite) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(bufferInfo(EveryBufferUsage), spall::ResourceStateFlags::Present) != spall::SUCCESS);
}

TEST_CASE(
	"A texture reaches Common regardless of usage",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(textureInfo(spall::TextureUsageFlags::None), spall::ResourceStateFlags::Common, false) == spall::SUCCESS);
}

TEST_CASE(
	"A texture rejects the unknown state",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(textureInfo(EveryTextureUsage), spall::ResourceStateFlags::Unknown, true) != spall::SUCCESS);
}

TEST_CASE(
	"A texture state requires its matching usage",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(textureInfo(spall::TextureUsageFlags::Sampled), spall::ResourceStateFlags::RenderTarget, false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(textureInfo(spall::TextureUsageFlags::Sampled), spall::ResourceStateFlags::DepthWrite, false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(textureInfo(spall::TextureUsageFlags::Sampled), spall::ResourceStateFlags::DepthRead, false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(
			  textureInfo(spall::TextureUsageFlags::ColorAttachment),
			  spall::ResourceStateFlags::ShaderResource,
			  false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(textureInfo(spall::TextureUsageFlags::Sampled), spall::ResourceStateFlags::CopySource, false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(textureInfo(spall::TextureUsageFlags::Sampled), spall::ResourceStateFlags::CopyDest, false) != spall::SUCCESS);
}

TEST_CASE(
	"A texture state is reachable with its matching usage",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(
			  textureInfo(spall::TextureUsageFlags::ColorAttachment),
			  spall::ResourceStateFlags::RenderTarget,
			  false) == spall::SUCCESS);
	CHECK(
		spall::validateTextureResourceState(
			textureInfo(spall::TextureUsageFlags::DepthStencilAttachment),
			spall::ResourceStateFlags::DepthWrite,
			false) == spall::SUCCESS);
	CHECK(
		spall::validateTextureResourceState(
			textureInfo(spall::TextureUsageFlags::DepthStencilAttachment),
			spall::ResourceStateFlags::DepthRead,
			false) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(textureInfo(spall::TextureUsageFlags::Sampled), spall::ResourceStateFlags::ShaderResource, false) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(
			  textureInfo(spall::TextureUsageFlags::Storage),
			  spall::ResourceStateFlags::UnorderedAccess,
			  false) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(
			  textureInfo(spall::TextureUsageFlags::TransferSource),
			  spall::ResourceStateFlags::CopySource,
			  false) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(
			  textureInfo(spall::TextureUsageFlags::TransferDestination),
			  spall::ResourceStateFlags::CopyDest,
			  false) == spall::SUCCESS);
}

TEST_CASE(
	"Only a presentable texture reaches Present",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(textureInfo(EveryTextureUsage), spall::ResourceStateFlags::Present, true) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(textureInfo(EveryTextureUsage), spall::ResourceStateFlags::Present, false) != spall::SUCCESS);
}

TEST_CASE(
	"Presentability does not unlock any other texture state",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(textureInfo(spall::TextureUsageFlags::Sampled), spall::ResourceStateFlags::RenderTarget, true) != spall::SUCCESS);
}

TEST_CASE(
	"A texture holds one state at a time",
	"[state][texture]")
{
	const spall::ResourceStateFlags combined = spall::ResourceStateFlags::RenderTarget | spall::ResourceStateFlags::ShaderResource;

	CHECK(spall::validateTextureResourceState(textureInfo(EveryTextureUsage), combined, true) != spall::SUCCESS);
}

TEST_CASE(
	"A texture rejects buffer-only states",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(textureInfo(EveryTextureUsage), spall::ResourceStateFlags::VertexBuffer, true) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(textureInfo(EveryTextureUsage), spall::ResourceStateFlags::IndexBuffer, true) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(textureInfo(EveryTextureUsage), spall::ResourceStateFlags::ConstantBuffer, true) != spall::SUCCESS);
}
