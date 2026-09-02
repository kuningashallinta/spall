#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/TextureValidation.h>
#include <tests/Support/Fakes.h>

static spall::Texture3DCreateInfo sampledVolumeCreateInfo()
{
	spall::Texture3DCreateInfo info = {};
	info.Width = 64;
	info.Height = 64;
	info.Depth = 8;
	info.Format = spall::Format::RGBA8;
	info.Usage = spall::TextureUsageFlags::Sampled;

	return info;
}

static spall::Texture2DCreateInfo sampledTextureCreateInfo()
{
	spall::Texture2DCreateInfo info = {};
	info.Width = 64;
	info.Height = 64;
	info.Format = spall::Format::RGBA8;
	info.Usage = spall::TextureUsageFlags::Sampled;

	return info;
}

TEST_CASE(
	"The mip chain runs down to a single pixel",
	"[texture][mips]")
{
	CHECK(spall::maxTextureMipLevels(1, 1, 1) == 1);
	CHECK(spall::maxTextureMipLevels(2, 2, 1) == 2);
	CHECK(spall::maxTextureMipLevels(64, 64, 1) == 7);
	CHECK(spall::maxTextureMipLevels(256, 256, 1) == 9);
}

TEST_CASE(
	"The mip chain follows the larger dimension",
	"[texture][mips]")
{
	CHECK(spall::maxTextureMipLevels(64, 1, 1) == 7);
	CHECK(spall::maxTextureMipLevels(1, 64, 1) == 7);
	CHECK(spall::maxTextureMipLevels(64, 16, 1) == 7);
}

TEST_CASE(
	"A non-power-of-two mip chain halves down to one",
	"[texture][mips]")
{
	CHECK(spall::maxTextureMipLevels(100, 100, 1) == 7);
	CHECK(spall::maxTextureMipLevels(3, 3, 1) == 2);
	CHECK(spall::maxTextureMipLevels(5, 5, 1) == 3);
}

TEST_CASE(
	"A texture accepts every mip level its dimensions imply",
	"[texture][mips]")
{
	spall::Texture2DCreateInfo info = sampledTextureCreateInfo();
	info.MipLevels = 7;

	CHECK(spall::validateTexture2DCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A texture rejects one mip level beyond its dimensions",
	"[texture][mips]")
{
	spall::Texture2DCreateInfo info = sampledTextureCreateInfo();
	info.MipLevels = 8;

	CHECK(spall::validateTexture2DCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A texture rejects zero mip levels",
	"[texture][mips]")
{
	spall::Texture2DCreateInfo info = sampledTextureCreateInfo();
	info.MipLevels = 0;

	CHECK(spall::validateTexture2DCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A texture defaults to a single mip level",
	"[texture][mips]")
{
	const spall::Texture2DCreateInfo info = sampledTextureCreateInfo();

	CHECK(info.MipLevels == 1);
	CHECK(spall::validateTexture2DCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A texture requires nonzero dimensions",
	"[texture][create]")
{
	spall::Texture2DCreateInfo zeroWidth = sampledTextureCreateInfo();
	zeroWidth.Width = 0;

	spall::Texture2DCreateInfo zeroHeight = sampledTextureCreateInfo();
	zeroHeight.Height = 0;

	CHECK(spall::validateTexture2DCreateInfo(zeroWidth) == spall::ERR_INVALID_SIZE);
	CHECK(spall::validateTexture2DCreateInfo(zeroHeight) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A texture requires a texture-capable format",
	"[texture][create]")
{
	spall::Texture2DCreateInfo unknown = sampledTextureCreateInfo();
	unknown.Format = spall::Format::Unknown;

	spall::Texture2DCreateInfo vertexOnly = sampledTextureCreateInfo();
	vertexOnly.Format = spall::Format::RGB32Float;

	CHECK(spall::validateTexture2DCreateInfo(unknown) == spall::ERR_INVALID_FORMAT);
	CHECK(spall::validateTexture2DCreateInfo(vertexOnly) == spall::ERR_INVALID_FORMAT);
}

TEST_CASE(
	"Textures accept sRGB formats for sampling and color attachments",
	"[texture][create][srgb]")
{
	spall::Texture2DCreateInfo rgba = sampledTextureCreateInfo();
	rgba.Format = spall::Format::RGBA8Srgb;
	rgba.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::ColorAttachment;

	spall::Texture2DCreateInfo bgra = rgba;
	bgra.Format = spall::Format::BGRA8Srgb;

	CHECK(spall::validateTexture2DCreateInfo(rgba) == spall::SUCCESS);
	CHECK(spall::validateTexture2DCreateInfo(bgra) == spall::SUCCESS);
}

TEST_CASE(
	"A texture requires known usage flags",
	"[texture][create]")
{
	spall::Texture2DCreateInfo noUsage = sampledTextureCreateInfo();
	noUsage.Usage = spall::TextureUsageFlags::None;

	spall::Texture2DCreateInfo unknownUsage = sampledTextureCreateInfo();
	unknownUsage.Usage = spall::TextureUsageFlags::Sampled | static_cast<spall::TextureUsageFlags>(1u << 20);

	CHECK(spall::validateTexture2DCreateInfo(noUsage) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTexture2DCreateInfo(unknownUsage) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"Texture attachment usage must match the format",
	"[texture][create]")
{
	spall::Texture2DCreateInfo depthAsColor = sampledTextureCreateInfo();
	depthAsColor.Format = spall::Format::Depth32Float;
	depthAsColor.Usage = spall::TextureUsageFlags::ColorAttachment;

	spall::Texture2DCreateInfo colorAsDepth = sampledTextureCreateInfo();
	colorAsDepth.Usage = spall::TextureUsageFlags::DepthStencilAttachment;

	spall::Texture2DCreateInfo bothAttachmentKinds = sampledTextureCreateInfo();
	bothAttachmentKinds.Usage = spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::DepthStencilAttachment;

	CHECK(spall::validateTexture2DCreateInfo(depthAsColor) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTexture2DCreateInfo(colorAsDepth) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateTexture2DCreateInfo(bothAttachmentKinds) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"Sampled depth textures report an unsupported usage",
	"[texture][create]")
{
	spall::Texture2DCreateInfo info = sampledTextureCreateInfo();
	info.Format = spall::Format::Depth32Float;
	info.Usage = spall::TextureUsageFlags::DepthStencilAttachment | spall::TextureUsageFlags::Sampled;

	CHECK(spall::validateTexture2DCreateInfo(info) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"Storage usage is limited to color textures",
	"[texture][create][storage]")
{
	spall::Texture2DCreateInfo color = sampledTextureCreateInfo();
	color.Usage = spall::TextureUsageFlags::Storage;

	spall::Texture2DCreateInfo depth = color;
	depth.Format = spall::Format::Depth32Float;

	CHECK(spall::validateTexture2DCreateInfo(color) == spall::SUCCESS);
	CHECK(spall::validateTexture2DCreateInfo(depth) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"Block-compressed textures are accepted for sampling and transfers",
	"[texture][create][compressed]")
{
	spall::Texture2DCreateInfo info = sampledTextureCreateInfo();
	info.Format = spall::Format::BC7RGBASrgb;
	info.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::TransferDestination;
	info.MipLevels = 7;

	CHECK(spall::validateTexture2DCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"Block-compressed textures reject attachment and storage usage",
	"[texture][create][compressed]")
{
	spall::Texture2DCreateInfo colorAttachment = sampledTextureCreateInfo();
	colorAttachment.Format = spall::Format::BC1RGBAUnorm;
	colorAttachment.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::ColorAttachment;

	spall::Texture2DCreateInfo storage = sampledTextureCreateInfo();
	storage.Format = spall::Format::BC1RGBAUnorm;
	storage.Usage = spall::TextureUsageFlags::Storage;

	CHECK(spall::validateTexture2DCreateInfo(colorAttachment) == spall::ERR_UNSUPPORTED_USAGE);
	CHECK(spall::validateTexture2DCreateInfo(storage) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"Block-compressed texture dimensions cover whole blocks",
	"[texture][create][compressed]")
{
	spall::Texture2DCreateInfo aligned = sampledTextureCreateInfo();
	aligned.Format = spall::Format::BC3RGBAUnorm;
	aligned.Width = 68;
	aligned.Height = 4;

	spall::Texture2DCreateInfo unalignedWidth = aligned;
	unalignedWidth.Width = 66;

	spall::Texture2DCreateInfo unalignedHeight = aligned;
	unalignedHeight.Height = 6;

	CHECK(spall::validateTexture2DCreateInfo(aligned) == spall::SUCCESS);
	CHECK(spall::validateTexture2DCreateInfo(unalignedWidth) == spall::ERR_INVALID_SIZE);
	CHECK(spall::validateTexture2DCreateInfo(unalignedHeight) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A texture accepts initial states backed by its usage",
	"[texture][create][state]")
{
	spall::Texture2DCreateInfo sampled = sampledTextureCreateInfo();
	sampled.InitialState = spall::ResourceStateFlags::ShaderResource;

	spall::Texture2DCreateInfo color = sampledTextureCreateInfo();
	color.Usage = spall::TextureUsageFlags::ColorAttachment;
	color.InitialState = spall::ResourceStateFlags::RenderTarget;

	spall::Texture2DCreateInfo depth = sampledTextureCreateInfo();
	depth.Format = spall::Format::Depth32Float;
	depth.Usage = spall::TextureUsageFlags::DepthStencilAttachment;
	depth.InitialState = spall::ResourceStateFlags::DepthWrite;

	spall::Texture2DCreateInfo depthRead = depth;
	depthRead.InitialState = spall::ResourceStateFlags::DepthRead;

	spall::Texture2DCreateInfo source = sampledTextureCreateInfo();
	source.Usage = spall::TextureUsageFlags::TransferSource;
	source.InitialState = spall::ResourceStateFlags::CopySource;

	spall::Texture2DCreateInfo destination = sampledTextureCreateInfo();
	destination.Usage = spall::TextureUsageFlags::TransferDestination;
	destination.InitialState = spall::ResourceStateFlags::CopyDest;

	spall::Texture2DCreateInfo storage = sampledTextureCreateInfo();
	storage.Usage = spall::TextureUsageFlags::Storage;
	storage.InitialState = spall::ResourceStateFlags::UnorderedAccess;

	CHECK(spall::validateTexture2DCreateInfo(sampled) == spall::SUCCESS);
	CHECK(spall::validateTexture2DCreateInfo(color) == spall::SUCCESS);
	CHECK(spall::validateTexture2DCreateInfo(depth) == spall::SUCCESS);
	CHECK(spall::validateTexture2DCreateInfo(depthRead) == spall::SUCCESS);
	CHECK(spall::validateTexture2DCreateInfo(source) == spall::SUCCESS);
	CHECK(spall::validateTexture2DCreateInfo(destination) == spall::SUCCESS);
	CHECK(spall::validateTexture2DCreateInfo(storage) == spall::SUCCESS);
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
		spall::Texture2DCreateInfo info = sampledTextureCreateInfo();
		info.InitialState = state;

		CHECK(spall::validateTexture2DCreateInfo(info) == spall::ERR_INVALID_RESOURCE_STATE);
	}
}

TEST_CASE(
	"A texture initial state contains exactly one state",
	"[texture][create][state]")
{
	spall::Texture2DCreateInfo unknown = sampledTextureCreateInfo();
	unknown.InitialState = spall::ResourceStateFlags::Unknown;

	spall::Texture2DCreateInfo combined = sampledTextureCreateInfo();
	combined.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::TransferSource;
	combined.InitialState = spall::ResourceStateFlags::ShaderResource | spall::ResourceStateFlags::CopySource;

	CHECK(spall::validateTexture2DCreateInfo(unknown) == spall::ERR_INVALID_RESOURCE_STATE);
	CHECK(spall::validateTexture2DCreateInfo(combined) == spall::ERR_INVALID_RESOURCE_STATE);
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
	FakeTexture color(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
	spall::TextureViewCreateInfo colorView = {};
	colorView.Texture = &color;

	spall::TextureInfo depthInfo = {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::Depth24Stencil8,
		.Usage = spall::TextureUsageFlags::DepthStencilAttachment};
	FakeTexture depth(depthInfo);
	spall::TextureViewCreateInfo depthView = {};
	depthView.Texture = &depth;

	CHECK(spall::validateTextureViewCreateInfo(colorView) == spall::SUCCESS);
	CHECK(spall::validateTextureViewCreateInfo(depthView) == spall::SUCCESS);
}

TEST_CASE(
	"A texture view accepts a mip subresource range",
	"[texture][view][mips]")
{
	FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
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
	FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
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
	FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
	spall::TextureViewCreateInfo info = {};
	info.Texture = &texture;
	info.Format = spall::Format::BGRA8;

	CHECK(spall::validateTextureViewCreateInfo(info) == spall::ERR_UNSUPPORTED_FORMAT);
}

TEST_CASE(
	"A texture view rejects aspects absent from its format",
	"[texture][view]")
{
	FakeTexture color(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
	spall::TextureViewCreateInfo colorAsDepth = {};
	colorAsDepth.Texture = &color;
	colorAsDepth.Aspects = spall::TextureAspectFlags::Depth;

	spall::TextureInfo depthInfo = {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::Depth32Float,
		.Usage = spall::TextureUsageFlags::DepthStencilAttachment};
	FakeTexture depth(depthInfo);
	spall::TextureViewCreateInfo depthAsColor = {};
	depthAsColor.Texture = &depth;
	depthAsColor.Aspects = spall::TextureAspectFlags::Color;

	spall::TextureViewCreateInfo depthAsStencil = {};
	depthAsStencil.Texture = &depth;
	depthAsStencil.Aspects = spall::TextureAspectFlags::Stencil;

	spall::TextureInfo combinedInfo = {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::Depth32FloatStencil8,
		.Usage = spall::TextureUsageFlags::DepthStencilAttachment};
	FakeTexture combined(combinedInfo);
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
	FakeTexture transferOnly(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferSource});
	spall::TextureViewCreateInfo colorView = {};
	colorView.Texture = &transferOnly;
	colorView.Aspects = spall::TextureAspectFlags::Color;

	spall::TextureInfo depthInfo = {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::Depth32Float,
		.Usage = spall::TextureUsageFlags::TransferSource};
	FakeTexture depth(depthInfo);
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
	spall::Texture2DCreateInfo info = sampledTextureCreateInfo();
	info.ArrayLayers = 0;

	CHECK(spall::validateTexture2DCreateInfo(info) == spall::ERR_INVALID_SIZE);

	info.ArrayLayers = 8;
	CHECK(spall::validateTexture2DCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A cubemap requires square faces in multiples of six",
	"[texture][create][cubemap]")
{
	spall::Texture2DCreateInfo faces = sampledTextureCreateInfo();
	faces.Cubemap = true;
	faces.ArrayLayers = 6;

	CHECK(spall::validateTexture2DCreateInfo(faces) == spall::SUCCESS);

	faces.ArrayLayers = 12;
	CHECK(spall::validateTexture2DCreateInfo(faces) == spall::SUCCESS);

	faces.ArrayLayers = 5;
	CHECK(spall::validateTexture2DCreateInfo(faces) == spall::ERR_INVALID_SIZE);

	spall::Texture2DCreateInfo oblong = sampledTextureCreateInfo();
	oblong.Cubemap = true;
	oblong.ArrayLayers = 6;
	oblong.Height = 32;

	CHECK(spall::validateTexture2DCreateInfo(oblong) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A volume texture accepts a depth extent",
	"[texture][create][volume]")
{
	spall::Texture3DCreateInfo info = sampledVolumeCreateInfo();

	CHECK(spall::validateTexture3DCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A volume texture rejects a zero depth extent",
	"[texture][create][volume]")
{
	spall::Texture3DCreateInfo info = sampledVolumeCreateInfo();
	info.Depth = 0;

	CHECK(spall::validateTexture3DCreateInfo(info) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A volume texture rejects block-compressed formats",
	"[texture][create][volume]")
{
	spall::Texture3DCreateInfo info = sampledVolumeCreateInfo();
	info.Format = spall::Format::BC1RGBAUnorm;

	CHECK(spall::validateTexture3DCreateInfo(info) == spall::ERR_UNSUPPORTED_USAGE);
}

TEST_CASE(
	"A volume texture rejects depth-stencil usage",
	"[texture][create][volume]")
{
	spall::Texture3DCreateInfo info = sampledVolumeCreateInfo();
	info.Format = spall::Format::Depth32Float;
	info.Usage = spall::TextureUsageFlags::DepthStencilAttachment;

	CHECK(spall::validateTexture3DCreateInfo(info) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"A texture view accepts a layer subresource range",
	"[texture][view][layers]")
{
	FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.ArrayLayers = 8,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
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
	FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.ArrayLayers = 8,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
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
	FakeTexture array(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.ArrayLayers = 12,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
	spall::TextureViewCreateInfo arrayAsCube = {};
	arrayAsCube.Texture = &array;
	arrayAsCube.Cubemap = true;

	CHECK(spall::validateTextureViewCreateInfo(arrayAsCube) == spall::ERR_UNSUPPORTED_USAGE);

	FakeTexture cube(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.ArrayLayers = 12,
		.Cubemap = true,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});
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
	const spall::TextureInfo info = {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.ArrayLayers = 6,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled};

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
	const spall::TextureInfo info = {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.ArrayLayers = 6,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled};

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
	const spall::TextureInfo info = {
		.Width = 64,
		.Height = 64,
		.MipLevels = 3,
		.ArrayLayers = 6,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled};

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

	spall::Texture2DCreateInfo info = sampledTextureCreateInfo();
	info.SampleCount = 3;

	CHECK(spall::validateTexture2DCreateInfo(info) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A multisampled texture requires attachment usage",
	"[texture][create][msaa]")
{
	spall::Texture2DCreateInfo sampled = sampledTextureCreateInfo();
	sampled.SampleCount = 4;

	CHECK(spall::validateTexture2DCreateInfo(sampled) == spall::ERR_UNSUPPORTED_USAGE);

	spall::Texture2DCreateInfo attachment = sampledTextureCreateInfo();
	attachment.SampleCount = 4;
	attachment.Usage = spall::TextureUsageFlags::ColorAttachment;

	CHECK(spall::validateTexture2DCreateInfo(attachment) == spall::SUCCESS);
}

TEST_CASE(
	"A multisampled texture rejects mips, storage, and cubemaps",
	"[texture][create][msaa]")
{
	spall::Texture2DCreateInfo info = sampledTextureCreateInfo();
	info.SampleCount = 4;
	info.Usage = spall::TextureUsageFlags::ColorAttachment;

	spall::Texture2DCreateInfo mipped = info;
	mipped.MipLevels = 4;
	CHECK(spall::validateTexture2DCreateInfo(mipped) == spall::ERR_INVALID_SIZE);

	spall::Texture2DCreateInfo storage = info;
	storage.Usage = spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::Storage;
	CHECK(spall::validateTexture2DCreateInfo(storage) == spall::ERR_UNSUPPORTED_USAGE);

	spall::Texture2DCreateInfo cube = info;
	cube.Cubemap = true;
	cube.ArrayLayers = 6;
	CHECK(spall::validateTexture2DCreateInfo(cube) == spall::ERR_UNSUPPORTED_USAGE);
}
