#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/TextureValidation.h>
#include <tests/Support/Fakes.h>

namespace
{
	spall::TextureCreateInfo sampledTextureCreateInfo()
	{
		spall::TextureCreateInfo info = {};
		info.Width = 64;
		info.Height = 64;
		info.Format = spall::Format::RGBA8;
		info.Usage = spall::TextureUsageFlags::Sampled;

		return info;
	}
} // namespace

TEST_CASE(
	"The mip chain runs down to a single pixel",
	"[texture][mips]")
{
	CHECK(spall::maxTextureMipLevels(1, 1) == 1);
	CHECK(spall::maxTextureMipLevels(2, 2) == 2);
	CHECK(spall::maxTextureMipLevels(64, 64) == 7);
	CHECK(spall::maxTextureMipLevels(256, 256) == 9);
}

TEST_CASE(
	"The mip chain follows the larger dimension",
	"[texture][mips]")
{
	CHECK(spall::maxTextureMipLevels(64, 1) == 7);
	CHECK(spall::maxTextureMipLevels(1, 64) == 7);
	CHECK(spall::maxTextureMipLevels(64, 16) == 7);
}

TEST_CASE(
	"A non-power-of-two mip chain halves down to one",
	"[texture][mips]")
{
	CHECK(spall::maxTextureMipLevels(100, 100) == 7);
	CHECK(spall::maxTextureMipLevels(3, 3) == 2);
	CHECK(spall::maxTextureMipLevels(5, 5) == 3);
}

TEST_CASE(
	"A texture accepts every mip level its dimensions imply",
	"[texture][mips]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.MipLevels = 7;

	CHECK(spall::validateTextureCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A texture rejects one mip level beyond its dimensions",
	"[texture][mips]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.MipLevels = 8;

	CHECK(spall::validateTextureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A texture rejects zero mip levels",
	"[texture][mips]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.MipLevels = 0;

	CHECK(spall::validateTextureCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A texture defaults to a single mip level",
	"[texture][mips]")
{
	const spall::TextureCreateInfo info = sampledTextureCreateInfo();

	CHECK(info.MipLevels == 1);
	CHECK(spall::validateTextureCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A texture requires nonzero dimensions",
	"[texture][create]")
{
	spall::TextureCreateInfo zeroWidth = sampledTextureCreateInfo();
	zeroWidth.Width = 0;

	spall::TextureCreateInfo zeroHeight = sampledTextureCreateInfo();
	zeroHeight.Height = 0;

	CHECK(spall::validateTextureCreateInfo(zeroWidth) == spall::ERR_INVALID_SIZE);
	CHECK(spall::validateTextureCreateInfo(zeroHeight) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A texture requires a texture-capable format",
	"[texture][create]")
{
	spall::TextureCreateInfo unknown = sampledTextureCreateInfo();
	unknown.Format = spall::Format::Unknown;

	spall::TextureCreateInfo vertexOnly = sampledTextureCreateInfo();
	vertexOnly.Format = spall::Format::RGB32Float;

	CHECK(spall::validateTextureCreateInfo(unknown) == spall::ERR_INVALID_FORMAT);
	CHECK(spall::validateTextureCreateInfo(vertexOnly) == spall::ERR_INVALID_FORMAT);
}

TEST_CASE(
	"Textures accept sRGB formats for sampling and color attachments",
	"[texture][create][srgb]")
{
	spall::TextureCreateInfo rgba = sampledTextureCreateInfo();
	rgba.Format = spall::Format::RGBA8Srgb;
	rgba.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::ColorAttachment;

	spall::TextureCreateInfo bgra = rgba;
	bgra.Format = spall::Format::BGRA8Srgb;

	CHECK(spall::validateTextureCreateInfo(rgba) == spall::SUCCESS);
	CHECK(spall::validateTextureCreateInfo(bgra) == spall::SUCCESS);
}

TEST_CASE(
	"A texture requires known usage flags",
	"[texture][create]")
{
	spall::TextureCreateInfo noUsage = sampledTextureCreateInfo();
	noUsage.Usage = spall::TextureUsageFlags::None;

	spall::TextureCreateInfo unknownUsage = sampledTextureCreateInfo();
	unknownUsage.Usage = spall::TextureUsageFlags::Sampled | static_cast<spall::TextureUsageFlags>(1u << 20);

	CHECK(spall::validateTextureCreateInfo(noUsage) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTextureCreateInfo(unknownUsage) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"Texture attachment usage must match the format",
	"[texture][create]")
{
	spall::TextureCreateInfo depthAsColor = sampledTextureCreateInfo();
	depthAsColor.Format = spall::Format::Depth32Float;
	depthAsColor.Usage = spall::TextureUsageFlags::ColorAttachment;

	spall::TextureCreateInfo colorAsDepth = sampledTextureCreateInfo();
	colorAsDepth.Usage = spall::TextureUsageFlags::DepthStencilAttachment;

	spall::TextureCreateInfo bothAttachmentKinds = sampledTextureCreateInfo();
	bothAttachmentKinds.Usage = spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::DepthStencilAttachment;

	CHECK(spall::validateTextureCreateInfo(depthAsColor) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTextureCreateInfo(colorAsDepth) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTextureCreateInfo(bothAttachmentKinds) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"Sampled depth textures report an unsupported usage",
	"[texture][create]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.Format = spall::Format::Depth32Float;
	info.Usage = spall::TextureUsageFlags::DepthStencilAttachment | spall::TextureUsageFlags::Sampled;

	CHECK(spall::validateTextureCreateInfo(info) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"Storage usage is limited to color textures",
	"[texture][create][storage]")
{
	spall::TextureCreateInfo color = sampledTextureCreateInfo();
	color.Usage = spall::TextureUsageFlags::Storage;

	spall::TextureCreateInfo depth = color;
	depth.Format = spall::Format::Depth32Float;

	CHECK(spall::validateTextureCreateInfo(color) == spall::SUCCESS);
	CHECK(spall::validateTextureCreateInfo(depth) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"Block-compressed textures are accepted for sampling and transfers",
	"[texture][create][compressed]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.Format = spall::Format::BC7RGBASrgb;
	info.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::TransferDestination;
	info.MipLevels = 7;

	CHECK(spall::validateTextureCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"Block-compressed textures reject attachment and storage usage",
	"[texture][create][compressed]")
{
	spall::TextureCreateInfo colorAttachment = sampledTextureCreateInfo();
	colorAttachment.Format = spall::Format::BC1RGBAUnorm;
	colorAttachment.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::ColorAttachment;

	spall::TextureCreateInfo storage = sampledTextureCreateInfo();
	storage.Format = spall::Format::BC1RGBAUnorm;
	storage.Usage = spall::TextureUsageFlags::Storage;

	CHECK(spall::validateTextureCreateInfo(colorAttachment) == spall::ERR_UNSUPPORTED_USAGE);
	CHECK(spall::validateTextureCreateInfo(storage) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"Block-compressed texture dimensions cover whole blocks",
	"[texture][create][compressed]")
{
	spall::TextureCreateInfo aligned = sampledTextureCreateInfo();
	aligned.Format = spall::Format::BC3RGBAUnorm;
	aligned.Width = 68;
	aligned.Height = 4;

	spall::TextureCreateInfo unalignedWidth = aligned;
	unalignedWidth.Width = 66;

	spall::TextureCreateInfo unalignedHeight = aligned;
	unalignedHeight.Height = 6;

	CHECK(spall::validateTextureCreateInfo(aligned) == spall::SUCCESS);
	CHECK(spall::validateTextureCreateInfo(unalignedWidth) == spall::ERR_INVALID_SIZE);
	CHECK(spall::validateTextureCreateInfo(unalignedHeight) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A texture accepts initial states backed by its usage",
	"[texture][create][state]")
{
	spall::TextureCreateInfo sampled = sampledTextureCreateInfo();
	sampled.InitialState = spall::ResourceStateFlags::ShaderResource;

	spall::TextureCreateInfo color = sampledTextureCreateInfo();
	color.Usage = spall::TextureUsageFlags::ColorAttachment;
	color.InitialState = spall::ResourceStateFlags::RenderTarget;

	spall::TextureCreateInfo depth = sampledTextureCreateInfo();
	depth.Format = spall::Format::Depth32Float;
	depth.Usage = spall::TextureUsageFlags::DepthStencilAttachment;
	depth.InitialState = spall::ResourceStateFlags::DepthWrite;

	spall::TextureCreateInfo depthRead = depth;
	depthRead.InitialState = spall::ResourceStateFlags::DepthRead;

	spall::TextureCreateInfo source = sampledTextureCreateInfo();
	source.Usage = spall::TextureUsageFlags::TransferSource;
	source.InitialState = spall::ResourceStateFlags::CopySource;

	spall::TextureCreateInfo destination = sampledTextureCreateInfo();
	destination.Usage = spall::TextureUsageFlags::TransferDestination;
	destination.InitialState = spall::ResourceStateFlags::CopyDest;

	spall::TextureCreateInfo storage = sampledTextureCreateInfo();
	storage.Usage = spall::TextureUsageFlags::Storage;
	storage.InitialState = spall::ResourceStateFlags::UnorderedAccess;

	CHECK(spall::validateTextureCreateInfo(sampled) == spall::SUCCESS);
	CHECK(spall::validateTextureCreateInfo(color) == spall::SUCCESS);
	CHECK(spall::validateTextureCreateInfo(depth) == spall::SUCCESS);
	CHECK(spall::validateTextureCreateInfo(depthRead) == spall::SUCCESS);
	CHECK(spall::validateTextureCreateInfo(source) == spall::SUCCESS);
	CHECK(spall::validateTextureCreateInfo(destination) == spall::SUCCESS);
	CHECK(spall::validateTextureCreateInfo(storage) == spall::SUCCESS);
}

TEST_CASE(
	"A texture rejects an initial state not backed by its usage",
	"[texture][create][state]")
{
	const spall::ResourceStateFlags incompatibleStates[] = {
		spall::ResourceStateFlags::UnorderedAccess,
		spall::ResourceStateFlags::RenderTarget,
		spall::ResourceStateFlags::DepthWrite,
		spall::ResourceStateFlags::DepthRead,
		spall::ResourceStateFlags::CopySource,
		spall::ResourceStateFlags::CopyDest};

	for (const spall::ResourceStateFlags state : incompatibleStates)
	{
		spall::TextureCreateInfo info = sampledTextureCreateInfo();
		info.InitialState = state;

		CHECK(spall::validateTextureCreateInfo(info) == spall::ERR_INVALID_RESOURCE_STATE);
	}
}

TEST_CASE(
	"A texture initial state contains exactly one state",
	"[texture][create][state]")
{
	spall::TextureCreateInfo unknown = sampledTextureCreateInfo();
	unknown.InitialState = spall::ResourceStateFlags::Unknown;

	spall::TextureCreateInfo combined = sampledTextureCreateInfo();
	combined.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::TransferSource;
	combined.InitialState = spall::ResourceStateFlags::ShaderResource | spall::ResourceStateFlags::CopySource;

	CHECK(spall::validateTextureCreateInfo(unknown) == spall::ERR_INVALID_RESOURCE_STATE);
	CHECK(spall::validateTextureCreateInfo(combined) == spall::ERR_INVALID_RESOURCE_STATE);
}

TEST_CASE(
	"A texture view requires a texture",
	"[texture][view]")
{
	const spall::TextureViewCreateInfo info = {};

	CHECK(spall::validateTextureViewCreateInfo(info) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"A texture view inherits its texture format and default aspects",
	"[texture][view]")
{
	spall::tests::FakeTexture color(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled));
	spall::TextureViewCreateInfo colorView = {};
	colorView.Texture = &color;

	spall::TextureInfo depthInfo = spall::tests::textureInfo(spall::TextureUsageFlags::DepthStencilAttachment);
	depthInfo.Format = spall::Format::Depth24Stencil8;
	spall::tests::FakeTexture depth(depthInfo);
	spall::TextureViewCreateInfo depthView = {};
	depthView.Texture = &depth;

	CHECK(spall::validateTextureViewCreateInfo(colorView) == spall::SUCCESS);
	CHECK(spall::validateTextureViewCreateInfo(depthView) == spall::SUCCESS);
}

TEST_CASE(
	"A texture view accepts a mip subresource range",
	"[texture][view][mips]")
{
	spall::tests::FakeTexture texture(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled, 7));
	spall::TextureViewCreateInfo info = {};
	info.Texture = &texture;
	info.BaseMipLevel = 2;
	info.MipLevels = 3;

	CHECK(spall::validateTextureViewCreateInfo(info) == spall::SUCCESS);

	info.MipLevels = 0;
	CHECK(spall::validateTextureViewCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A texture view rejects mip ranges outside its texture",
	"[texture][view][mips]")
{
	spall::tests::FakeTexture texture(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled, 7));
	spall::TextureViewCreateInfo info = {};
	info.Texture = &texture;
	info.BaseMipLevel = 7;

	CHECK(spall::validateTextureViewCreateInfo(info) == spall::ERR_INVALID_RANGE);

	info.BaseMipLevel = 4;
	info.MipLevels = 4;
	CHECK(spall::validateTextureViewCreateInfo(info) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"A texture view cannot reinterpret its format",
	"[texture][view]")
{
	spall::tests::FakeTexture texture(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled));
	spall::TextureViewCreateInfo info = {};
	info.Texture = &texture;
	info.Format = spall::Format::BGRA8;

	CHECK(spall::validateTextureViewCreateInfo(info) == spall::ERR_UNSUPPORTED_FORMAT);
}

TEST_CASE(
	"A texture view rejects aspects absent from its format",
	"[texture][view]")
{
	spall::tests::FakeTexture color(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled));
	spall::TextureViewCreateInfo colorAsDepth = {};
	colorAsDepth.Texture = &color;
	colorAsDepth.Aspects = spall::TextureAspectFlags::Depth;

	spall::TextureInfo depthInfo = spall::tests::textureInfo(spall::TextureUsageFlags::DepthStencilAttachment);
	depthInfo.Format = spall::Format::Depth32Float;
	spall::tests::FakeTexture depth(depthInfo);
	spall::TextureViewCreateInfo depthAsColor = {};
	depthAsColor.Texture = &depth;
	depthAsColor.Aspects = spall::TextureAspectFlags::Color;

	spall::TextureViewCreateInfo depthAsStencil = {};
	depthAsStencil.Texture = &depth;
	depthAsStencil.Aspects = spall::TextureAspectFlags::Stencil;

	spall::TextureInfo combinedInfo = spall::tests::textureInfo(spall::TextureUsageFlags::DepthStencilAttachment);
	combinedInfo.Format = spall::Format::Depth32FloatStencil8;
	spall::tests::FakeTexture combined(combinedInfo);
	spall::TextureViewCreateInfo combinedAsStencil = {};
	combinedAsStencil.Texture = &combined;
	combinedAsStencil.Aspects = spall::TextureAspectFlags::Stencil;

	CHECK(spall::validateTextureViewCreateInfo(colorAsDepth) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTextureViewCreateInfo(depthAsColor) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTextureViewCreateInfo(depthAsStencil) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTextureViewCreateInfo(combinedAsStencil) == spall::SUCCESS);
}

TEST_CASE(
	"A texture view aspect requires matching texture usage",
	"[texture][view]")
{
	spall::tests::FakeTexture transferOnly(spall::tests::textureInfo(spall::TextureUsageFlags::TransferSource));
	spall::TextureViewCreateInfo colorView = {};
	colorView.Texture = &transferOnly;
	colorView.Aspects = spall::TextureAspectFlags::Color;

	spall::TextureInfo depthInfo = spall::tests::textureInfo(spall::TextureUsageFlags::TransferSource);
	depthInfo.Format = spall::Format::Depth32Float;
	spall::tests::FakeTexture depth(depthInfo);
	spall::TextureViewCreateInfo depthView = {};
	depthView.Texture = &depth;
	depthView.Aspects = spall::TextureAspectFlags::Depth;

	CHECK(spall::validateTextureViewCreateInfo(colorView) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTextureViewCreateInfo(depthView) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"A texture requires at least one array layer",
	"[texture][create][layers]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.ArrayLayers = 0;

	CHECK(spall::validateTextureCreateInfo(info) == spall::ERR_INVALID_SIZE);

	info.ArrayLayers = 8;
	CHECK(spall::validateTextureCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A cubemap requires square faces in multiples of six",
	"[texture][create][cubemap]")
{
	spall::TextureCreateInfo faces = sampledTextureCreateInfo();
	faces.Cubemap = true;
	faces.ArrayLayers = 6;

	CHECK(spall::validateTextureCreateInfo(faces) == spall::SUCCESS);

	faces.ArrayLayers = 12;
	CHECK(spall::validateTextureCreateInfo(faces) == spall::SUCCESS);

	faces.ArrayLayers = 5;
	CHECK(spall::validateTextureCreateInfo(faces) == spall::ERR_INVALID_SIZE);

	spall::TextureCreateInfo oblong = sampledTextureCreateInfo();
	oblong.Cubemap = true;
	oblong.ArrayLayers = 6;
	oblong.Height = 32;

	CHECK(spall::validateTextureCreateInfo(oblong) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A volume texture accepts a depth extent",
	"[texture][create][volume]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.Depth = 8;

	CHECK(spall::validateTextureCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A volume texture rejects array layers",
	"[texture][create][volume]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.Depth = 8;
	info.ArrayLayers = 2;

	CHECK(spall::validateTextureCreateInfo(info) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A volume texture rejects multisampling",
	"[texture][create][volume]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.Depth = 8;
	info.SampleCount = 4;
	info.Usage = spall::TextureUsageFlags::ColorAttachment;

	CHECK(spall::validateTextureCreateInfo(info) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"A volume texture rejects depth-stencil usage",
	"[texture][create][volume]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.Depth = 8;
	info.Format = spall::Format::Depth32Float;
	info.Usage = spall::TextureUsageFlags::DepthStencilAttachment;

	CHECK(spall::validateTextureCreateInfo(info) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"A texture view accepts a layer subresource range",
	"[texture][view][layers]")
{
	spall::tests::FakeTexture texture(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled, 1, 8));
	spall::TextureViewCreateInfo info = {};
	info.Texture = &texture;
	info.BaseArrayLayer = 2;
	info.ArrayLayers = 4;

	CHECK(spall::validateTextureViewCreateInfo(info) == spall::SUCCESS);

	info.ArrayLayers = 0;
	CHECK(spall::validateTextureViewCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A texture view rejects layer ranges outside its texture",
	"[texture][view][layers]")
{
	spall::tests::FakeTexture texture(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled, 1, 8));
	spall::TextureViewCreateInfo info = {};
	info.Texture = &texture;
	info.BaseArrayLayer = 8;

	CHECK(spall::validateTextureViewCreateInfo(info) == spall::ERR_INVALID_RANGE);

	info.BaseArrayLayer = 6;
	info.ArrayLayers = 4;
	CHECK(spall::validateTextureViewCreateInfo(info) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"A cubemap view requires whole faces of a cubemap texture",
	"[texture][view][cubemap]")
{
	spall::tests::FakeTexture array(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled, 1, 12));
	spall::TextureViewCreateInfo arrayAsCube = {};
	arrayAsCube.Texture = &array;
	arrayAsCube.Cubemap = true;

	CHECK(spall::validateTextureViewCreateInfo(arrayAsCube) == spall::ERR_UNSUPPORTED_USAGE);

	spall::tests::FakeTexture cube(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled, 1, 12, true));
	spall::TextureViewCreateInfo wholeCube = {};
	wholeCube.Texture = &cube;
	wholeCube.Cubemap = true;

	CHECK(spall::validateTextureViewCreateInfo(wholeCube) == spall::SUCCESS);

	spall::TextureViewCreateInfo secondCube = wholeCube;
	secondCube.BaseArrayLayer = 6;
	secondCube.ArrayLayers = 6;

	CHECK(spall::validateTextureViewCreateInfo(secondCube) == spall::SUCCESS);

	spall::TextureViewCreateInfo partialFaces = wholeCube;
	partialFaces.BaseArrayLayer = 2;
	partialFaces.ArrayLayers = 6;

	CHECK(spall::validateTextureViewCreateInfo(partialFaces) == spall::ERR_INVALID_RANGE);

	spall::TextureViewCreateInfo singleFace = {};
	singleFace.Texture = &cube;
	singleFace.BaseArrayLayer = 3;
	singleFace.ArrayLayers = 1;

	CHECK(spall::validateTextureViewCreateInfo(singleFace) == spall::SUCCESS);
}

TEST_CASE(
	"A subresource range resolves its open counts",
	"[texture][subresource]")
{
	const spall::TextureInfo info = spall::tests::textureInfo(spall::TextureUsageFlags::Sampled, 7, 6);

	const spall::TextureSubresourceRange whole = spall::resolveTextureSubresourceRange(info, {});
	CHECK(whole.BaseMipLevel == 0);
	CHECK(whole.MipLevels == 7);
	CHECK(whole.BaseArrayLayer == 0);
	CHECK(whole.ArrayLayers == 6);

	const spall::TextureSubresourceRange remainder = spall::resolveTextureSubresourceRange(info, spall::TextureSubresourceRange {4, 0, 2, 0});
	CHECK(remainder.MipLevels == 3);
	CHECK(remainder.ArrayLayers == 4);

	const spall::TextureSubresourceRange single = spall::resolveTextureSubresourceRange(info, spall::TextureSubresourceRange {1, 1, 5, 1});
	CHECK(single.MipLevels == 1);
	CHECK(single.ArrayLayers == 1);
}

TEST_CASE(
	"A subresource range stays inside its texture",
	"[texture][subresource]")
{
	const spall::TextureInfo info = spall::tests::textureInfo(spall::TextureUsageFlags::Sampled, 7, 6);

	CHECK(spall::validateTextureSubresourceRange(info, {}) == spall::SUCCESS);
	CHECK(spall::validateTextureSubresourceRange(info, spall::TextureSubresourceRange {6, 1, 5, 1}) == spall::SUCCESS);
	CHECK(spall::validateTextureSubresourceRange(info, spall::TextureSubresourceRange {5, 3, 0, 0}) == spall::ERR_INVALID_RANGE);
	CHECK(spall::validateTextureSubresourceRange(info, spall::TextureSubresourceRange {0, 0, 4, 4}) == spall::ERR_INVALID_RANGE);
	CHECK(spall::validateTextureSubresourceRange(info, spall::TextureSubresourceRange {7, 0, 0, 0}) == spall::ERR_INVALID_RANGE);
	CHECK(spall::validateTextureSubresourceRange(info, spall::TextureSubresourceRange {0, 0, 6, 0}) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"A subresource index runs mip-major over the layers",
	"[texture][subresource]")
{
	const spall::TextureInfo info = spall::tests::textureInfo(spall::TextureUsageFlags::Sampled, 3, 6);

	CHECK(spall::textureSubresourceCount(info) == 18);
	CHECK(spall::textureSubresourceIndex(info, 0, 0) == 0);
	CHECK(spall::textureSubresourceIndex(info, 0, 5) == 5);
	CHECK(spall::textureSubresourceIndex(info, 1, 0) == 6);
	CHECK(spall::textureSubresourceIndex(info, 2, 5) == 17);
}

TEST_CASE(
	"A texture sample count must be a power of two",
	"[texture][create][msaa]")
{
	CHECK(spall::isValidSampleCount(1));
	CHECK(spall::isValidSampleCount(2));
	CHECK(spall::isValidSampleCount(4));
	CHECK(spall::isValidSampleCount(64));
	CHECK_FALSE(spall::isValidSampleCount(0));
	CHECK_FALSE(spall::isValidSampleCount(3));
	CHECK_FALSE(spall::isValidSampleCount(6));
	CHECK_FALSE(spall::isValidSampleCount(128));

	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.SampleCount = 3;

	CHECK(spall::validateTextureCreateInfo(info) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A multisampled texture requires attachment usage",
	"[texture][create][msaa]")
{
	spall::TextureCreateInfo sampled = sampledTextureCreateInfo();
	sampled.SampleCount = 4;

	CHECK(spall::validateTextureCreateInfo(sampled) == spall::ERR_UNSUPPORTED_USAGE);

	spall::TextureCreateInfo attachment = sampledTextureCreateInfo();
	attachment.SampleCount = 4;
	attachment.Usage = spall::TextureUsageFlags::ColorAttachment;

	CHECK(spall::validateTextureCreateInfo(attachment) == spall::SUCCESS);
}

TEST_CASE(
	"A multisampled texture rejects mips, storage, and cubemaps",
	"[texture][create][msaa]")
{
	spall::TextureCreateInfo info = sampledTextureCreateInfo();
	info.SampleCount = 4;
	info.Usage = spall::TextureUsageFlags::ColorAttachment;

	spall::TextureCreateInfo mipped = info;
	mipped.MipLevels = 4;
	CHECK(spall::validateTextureCreateInfo(mipped) == spall::ERR_INVALID_SIZE);

	spall::TextureCreateInfo storage = info;
	storage.Usage = spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::Storage;
	CHECK(spall::validateTextureCreateInfo(storage) == spall::ERR_UNSUPPORTED_USAGE);

	spall::TextureCreateInfo cube = info;
	cube.Cubemap = true;
	cube.ArrayLayers = 6;
	CHECK(spall::validateTextureCreateInfo(cube) == spall::ERR_UNSUPPORTED_USAGE);
}
