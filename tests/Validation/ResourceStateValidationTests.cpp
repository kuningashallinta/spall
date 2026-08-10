#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/ResourceStateValidation.h>

static constexpr spall::TextureUsageFlags EveryTextureUsage =
	spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::TransferSource |
	spall::TextureUsageFlags::TransferDestination | spall::TextureUsageFlags::Sampled |
	spall::TextureUsageFlags::Storage;

static constexpr spall::BufferUsageFlags EveryBufferUsage =
	spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::Index | spall::BufferUsageFlags::Uniform |
	spall::BufferUsageFlags::TransferSource | spall::BufferUsageFlags::TransferDestination |
	spall::BufferUsageFlags::Storage;

TEST_CASE(
	"A buffer reaches Common regardless of usage",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Vertex}, spall::ResourceStateFlags::Common) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::None}, spall::ResourceStateFlags::Common) == spall::SUCCESS);
}

TEST_CASE(
	"A buffer rejects the unknown state",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = EveryBufferUsage}, spall::ResourceStateFlags::Unknown) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer never combines Common with another state",
	"[state][buffer]")
{
	const spall::ResourceStateFlags combined = spall::ResourceStateFlags::Common | spall::ResourceStateFlags::VertexBuffer;

	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = EveryBufferUsage}, combined) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer state requires its matching usage",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Index}, spall::ResourceStateFlags::VertexBuffer) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Vertex}, spall::ResourceStateFlags::IndexBuffer) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Vertex}, spall::ResourceStateFlags::ConstantBuffer) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Vertex}, spall::ResourceStateFlags::ShaderResource) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Vertex}, spall::ResourceStateFlags::UnorderedAccess) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Vertex}, spall::ResourceStateFlags::CopySource) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Vertex}, spall::ResourceStateFlags::CopyDest) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer state is reachable with its matching usage",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Vertex}, spall::ResourceStateFlags::VertexBuffer) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Index}, spall::ResourceStateFlags::IndexBuffer) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Uniform}, spall::ResourceStateFlags::ConstantBuffer) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Storage}, spall::ResourceStateFlags::ShaderResource) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Storage}, spall::ResourceStateFlags::UnorderedAccess) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::TransferSource}, spall::ResourceStateFlags::CopySource) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::TransferDestination}, spall::ResourceStateFlags::CopyDest) == spall::SUCCESS);
}

TEST_CASE(
	"A buffer combines states when every usage is present",
	"[state][buffer]")
{
	const spall::ResourceStateFlags combined =
		spall::ResourceStateFlags::VertexBuffer |
		spall::ResourceStateFlags::IndexBuffer |
		spall::ResourceStateFlags::CopySource;

	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = EveryBufferUsage}, combined) == spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = spall::BufferUsageFlags::Vertex | spall::BufferUsageFlags::Index}, combined) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer cannot transition to a texture-only state",
	"[state][buffer]")
{
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = EveryBufferUsage}, spall::ResourceStateFlags::RenderTarget) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = EveryBufferUsage}, spall::ResourceStateFlags::DepthWrite) != spall::SUCCESS);
	CHECK(spall::validateBufferResourceState(spall::BufferInfo {.Usage = EveryBufferUsage}, spall::ResourceStateFlags::Present) != spall::SUCCESS);
}

TEST_CASE(
	"A texture reaches Common regardless of usage",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = spall::TextureUsageFlags::None}, spall::ResourceStateFlags::Common, false) == spall::SUCCESS);
}

TEST_CASE(
	"A texture rejects the unknown state",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = EveryTextureUsage}, spall::ResourceStateFlags::Unknown, true) != spall::SUCCESS);
}

TEST_CASE(
	"A texture state requires its matching usage",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = spall::TextureUsageFlags::Sampled}, spall::ResourceStateFlags::RenderTarget, false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = spall::TextureUsageFlags::Sampled}, spall::ResourceStateFlags::DepthWrite, false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = spall::TextureUsageFlags::Sampled}, spall::ResourceStateFlags::DepthRead, false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(
			  spall::TextureInfo {.Usage = spall::TextureUsageFlags::ColorAttachment},
			  spall::ResourceStateFlags::ShaderResource,
			  false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = spall::TextureUsageFlags::Sampled}, spall::ResourceStateFlags::CopySource, false) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = spall::TextureUsageFlags::Sampled}, spall::ResourceStateFlags::CopyDest, false) != spall::SUCCESS);
}

TEST_CASE(
	"A texture state is reachable with its matching usage",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(
			  spall::TextureInfo {.Usage = spall::TextureUsageFlags::ColorAttachment},
			  spall::ResourceStateFlags::RenderTarget,
			  false) == spall::SUCCESS);
	CHECK(
		spall::validateTextureResourceState(
			spall::TextureInfo {.Usage = spall::TextureUsageFlags::DepthStencilAttachment},
			spall::ResourceStateFlags::DepthWrite,
			false) == spall::SUCCESS);
	CHECK(
		spall::validateTextureResourceState(
			spall::TextureInfo {.Usage = spall::TextureUsageFlags::DepthStencilAttachment},
			spall::ResourceStateFlags::DepthRead,
			false) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = spall::TextureUsageFlags::Sampled}, spall::ResourceStateFlags::ShaderResource, false) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(
			  spall::TextureInfo {.Usage = spall::TextureUsageFlags::Storage},
			  spall::ResourceStateFlags::UnorderedAccess,
			  false) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(
			  spall::TextureInfo {.Usage = spall::TextureUsageFlags::TransferSource},
			  spall::ResourceStateFlags::CopySource,
			  false) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(
			  spall::TextureInfo {.Usage = spall::TextureUsageFlags::TransferDestination},
			  spall::ResourceStateFlags::CopyDest,
			  false) == spall::SUCCESS);
}

TEST_CASE(
	"Only a presentable texture reaches Present",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = EveryTextureUsage}, spall::ResourceStateFlags::Present, true) == spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = EveryTextureUsage}, spall::ResourceStateFlags::Present, false) != spall::SUCCESS);
}

TEST_CASE(
	"Presentability does not unlock any other texture state",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = spall::TextureUsageFlags::Sampled}, spall::ResourceStateFlags::RenderTarget, true) != spall::SUCCESS);
}

TEST_CASE(
	"A texture holds one state at a time",
	"[state][texture]")
{
	const spall::ResourceStateFlags combined = spall::ResourceStateFlags::RenderTarget | spall::ResourceStateFlags::ShaderResource;

	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = EveryTextureUsage}, combined, true) != spall::SUCCESS);
}

TEST_CASE(
	"A texture rejects buffer-only states",
	"[state][texture]")
{
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = EveryTextureUsage}, spall::ResourceStateFlags::VertexBuffer, true) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = EveryTextureUsage}, spall::ResourceStateFlags::IndexBuffer, true) != spall::SUCCESS);
	CHECK(spall::validateTextureResourceState(spall::TextureInfo {.Usage = EveryTextureUsage}, spall::ResourceStateFlags::ConstantBuffer, true) != spall::SUCCESS);
}
