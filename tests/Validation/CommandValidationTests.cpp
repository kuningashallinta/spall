#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/CommandValidation.h>
#include <tests/Support/Fakes.h>

#include <limits>

static FakeTexture mippedTexture()
{
	return FakeTexture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination});
}

static FakeTexture layeredTexture()
{
	return FakeTexture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.ArrayLayers = 6,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination});
}

static FakeBuffer uploadBuffer(
	std::uint32_t size = 64 * 64 * 4)
{
	return FakeBuffer(spall::BufferInfo {.Size = size, .Usage = spall::BufferUsageFlags::TransferSource});
}

static FakeTexture compressedTexture(
	spall::Format format = spall::Format::BC1RGBAUnorm)
{
	spall::TextureInfo info = {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = format,
		.Usage = spall::TextureUsageFlags::TransferDestination};

	return FakeTexture(info);
}

TEST_CASE(
	"A mip extent halves per level",
	"[command][mips]")
{
	CHECK(spall::mipLevelExtent(64, 0) == 64);
	CHECK(spall::mipLevelExtent(64, 1) == 32);
	CHECK(spall::mipLevelExtent(64, 3) == 8);
	CHECK(spall::mipLevelExtent(64, 6) == 1);
}

TEST_CASE(
	"Push-constant updates must match their declaration",
	"[command][pushconstants]")
{
	const spall::ShaderStageFlags graphicsStages = spall::ShaderStageFlags::Vertex | spall::ShaderStageFlags::Fragment;

	CHECK(spall::validatePushConstantUpdate(graphicsStages, 32, graphicsStages, 8, 16) == spall::SUCCESS);
	CHECK(spall::validatePushConstantUpdate(graphicsStages, 32, spall::ShaderStageFlags::Vertex, 8, 16) != spall::SUCCESS);
	CHECK(spall::validatePushConstantUpdate(graphicsStages, 32, graphicsStages, 2, 16) != spall::SUCCESS);
	CHECK(spall::validatePushConstantUpdate(graphicsStages, 32, graphicsStages, 8, 15) != spall::SUCCESS);
	CHECK(spall::validatePushConstantUpdate(graphicsStages, 32, graphicsStages, 24, 12) != spall::SUCCESS);
	CHECK(spall::validatePushConstantUpdate(spall::ShaderStageFlags::None, 0, graphicsStages, 0, 4) != spall::SUCCESS);
}

TEST_CASE(
	"A mip extent never collapses to zero",
	"[command][mips]")
{
	CHECK(spall::mipLevelExtent(1, 0) == 1);
	CHECK(spall::mipLevelExtent(1, 5) == 1);
	CHECK(spall::mipLevelExtent(8, 20) == 1);
	CHECK(spall::mipLevelExtent(64, 40) == 1);
}

TEST_CASE(
	"A non-power-of-two mip extent truncates",
	"[command][mips]")
{
	CHECK(spall::mipLevelExtent(100, 1) == 50);
	CHECK(spall::mipLevelExtent(100, 2) == 25);
	CHECK(spall::mipLevelExtent(3, 1) == 1);
}

TEST_CASE(
	"An upload to the base mip level is accepted",
	"[command][mips]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 64, 64}, buffer, 0, 64 * 4) == spall::SUCCESS);
}

TEST_CASE(
	"An upload row pitch is measured against the mip, not the base level",
	"[command][mips]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {3, 0, 0, 8, 8}, buffer, 0, 8 * 4) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {6, 0, 0, 1, 1}, buffer, 0, 1 * 4) == spall::SUCCESS);
}

TEST_CASE(
	"An upload row pitch below the mip width is rejected",
	"[command][mips]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {3, 0, 0, 8, 8}, buffer, 0, (8 * 4) - 1) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 64, 64}, buffer, 0, (64 * 4) - 1) != spall::SUCCESS);
}

TEST_CASE(
	"An upload to a mip level the texture lacks is rejected",
	"[command][mips]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {7, 0, 0, 1, 1}, buffer, 0, 4) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {99, 0, 0, 1, 1}, buffer, 0, 4) != spall::SUCCESS);
}

TEST_CASE(
	"An upload from a buffer too small for the mip is rejected",
	"[command][mips]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer tiny = uploadBuffer(64);

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 64, 64}, tiny, 0, 64 * 4) != spall::SUCCESS);
}

TEST_CASE(
	"An upload exactly filling the buffer is accepted",
	"[command][mips]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer exact = uploadBuffer(64 * 64 * 4);

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 64, 64}, exact, 0, 64 * 4) == spall::SUCCESS);
}

TEST_CASE(
	"A partial mip readback accepts padding and checks the final row",
	"[command][copy][readback]")
{
	const FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferSource});
	const FakeBuffer exact(spall::BufferInfo {.Size = 72, .Usage = spall::BufferUsageFlags::TransferDestination});
	const FakeBuffer short1(spall::BufferInfo {.Size = 71, .Usage = spall::BufferUsageFlags::TransferDestination});
	const spall::TextureRegion region = {3, 2, 1, 4, 3};

	CHECK(spall::validateTextureBufferCopyArguments(texture, region, exact, 8, 24) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, region, short1, 8, 24) != spall::SUCCESS);
}

TEST_CASE(
	"An upload with a zero row pitch or an out-of-range offset is rejected",
	"[command][mips]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 64, 64}, buffer, 0, 0) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 64, 64}, buffer, 64 * 64 * 4, 64 * 4) != spall::SUCCESS);
}

TEST_CASE(
	"A zero extent selects the rest of the mip",
	"[command][region]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {3}, buffer, 0, 8 * 4) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {3}, buffer, 0, (8 * 4) - 1) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 60, 0, 0, 0}, buffer, 0, 4 * 4) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 64, 0, 0, 0}, buffer, 0, 4) != spall::SUCCESS);
}

TEST_CASE(
	"A texture region resolves against its mip level",
	"[command][region]")
{
	const spall::TextureInfo info = {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination};

	const spall::TextureRegion fullMip = spall::resolveTextureRegion(info, spall::TextureRegion {3});
	CHECK(fullMip.Width == 8);
	CHECK(fullMip.Height == 8);

	const spall::TextureRegion remainder = spall::resolveTextureRegion(info, spall::TextureRegion {0, 48, 16, 0, 0});
	CHECK(remainder.Width == 16);
	CHECK(remainder.Height == 48);

	const spall::TextureRegion explicitRegion = spall::resolveTextureRegion(info, spall::TextureRegion {1, 4, 4, 5, 6});
	CHECK(explicitRegion.Width == 5);
	CHECK(explicitRegion.Height == 6);

	const spall::TextureRegion edge = spall::resolveTextureRegion(info, spall::TextureRegion {0, 64, 0, 0, 1});
	CHECK(edge.Width == 0);
}

TEST_CASE(
	"A volume region resolves its depth extent against its mip",
	"[command][region]")
{
	spall::TextureInfo info = {
		.Width = 64,
		.Height = 64,
		.Depth = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination};

	const spall::TextureRegion fullMip = spall::resolveTextureRegion(info, spall::TextureRegion {3});
	CHECK(fullMip.Depth == 8);

	spall::TextureRegion slab = {};
	slab.Z = 16;
	const spall::TextureRegion remainder = spall::resolveTextureRegion(info, slab);
	CHECK(remainder.Depth == 48);

	spall::TextureRegion explicitSlab = {};
	explicitSlab.Depth = 4;
	const spall::TextureRegion explicitRegion = spall::resolveTextureRegion(info, explicitSlab);
	CHECK(explicitRegion.Depth == 4);

	const spall::TextureRegion planar = spall::resolveTextureRegion(
		spall::TextureInfo {
			.Width = 64,
			.Height = 64,
			.MipLevels = 7,
			.Format = spall::Format::RGBA8,
			.Usage = spall::TextureUsageFlags::TransferDestination},
		spall::TextureRegion {3});
	CHECK(planar.Depth == 1);
}

TEST_CASE(
	"An upload region must stay within the mip",
	"[command][region]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 33, 0, 32, 32}, buffer, 0, 32 * 4) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 33, 32, 32}, buffer, 0, 32 * 4) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 32, 32, 32, 32}, buffer, 0, 32 * 4) == spall::SUCCESS);
}

TEST_CASE(
	"An upload region is measured against its mip, not the base level",
	"[command][region]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {3, 4, 4, 4, 4}, buffer, 0, 4 * 4) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {3, 5, 5, 4, 4}, buffer, 0, 4 * 4) != spall::SUCCESS);
}

TEST_CASE(
	"A partial upload measures its row pitch against the region",
	"[command][region]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 4, 4, 8, 8}, buffer, 0, 8 * 4) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 4, 4, 8, 8}, buffer, 0, (8 * 4) - 1) != spall::SUCCESS);
}

TEST_CASE(
	"A partial upload sizes the source by the region, not the texture",
	"[command][region]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer exact = uploadBuffer(8 * (8 * 4));
	const FakeBuffer short1 = uploadBuffer(8 * (8 * 4) - 1);

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 8, 8}, exact, 0, 8 * 4) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 8, 8}, short1, 0, 8 * 4) != spall::SUCCESS);
}

TEST_CASE(
	"An upload region offset cannot wrap around",
	"[command][region]")
{
	const FakeTexture texture = mippedTexture();
	const FakeBuffer buffer = uploadBuffer();
	const std::uint32_t maximum = (std::numeric_limits<std::uint32_t>::max)();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, maximum, 0, 2, 2}, buffer, 0, 2 * 4) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, maximum, 2, 2}, buffer, 0, 2 * 4) != spall::SUCCESS);
}

TEST_CASE(
	"A texture copy requires matching mip counts",
	"[command][copy]")
{
	const FakeTexture source(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferSource});
	const FakeTexture destination(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination});
	const FakeTexture flat(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination});

	CHECK(spall::validateCopyTextureArguments(destination, source) == spall::SUCCESS);
	CHECK(spall::validateCopyTextureArguments(flat, source) != spall::SUCCESS);
}

TEST_CASE(
	"A texture copy rejects a shared source and destination",
	"[command][copy]")
{
	const FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferSource | spall::TextureUsageFlags::TransferDestination});

	CHECK(spall::validateCopyTextureArguments(texture, texture) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"A texture copy requires transfer-compatible usage",
	"[command][copy]")
{
	const FakeTexture source(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferSource});
	const FakeTexture destination(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination});
	const FakeTexture sampled(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled});

	CHECK(spall::validateCopyTextureArguments(destination, sampled) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateCopyTextureArguments(sampled, source) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"A texture copy requires matching dimensions and formats",
	"[command][copy]")
{
	spall::TextureInfo sourceInfo = {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferSource};
	const FakeTexture source(sourceInfo);

	spall::TextureInfo widthInfo = {
		.Width = 32,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination};
	const FakeTexture differentWidth(widthInfo);

	spall::TextureInfo heightInfo = {
		.Width = 64,
		.Height = 32,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination};
	const FakeTexture differentHeight(heightInfo);

	spall::TextureInfo formatInfo = {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::BGRA8,
		.Usage = spall::TextureUsageFlags::TransferDestination};
	const FakeTexture differentFormat(formatInfo);

	CHECK(spall::validateCopyTextureArguments(differentWidth, source) == spall::ERR_INVALID_RESOURCE);
	CHECK(spall::validateCopyTextureArguments(differentHeight, source) == spall::ERR_INVALID_RESOURCE);
	CHECK(spall::validateCopyTextureArguments(differentFormat, source) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"Mipmap generation accepts a compatible color texture",
	"[command][mips]")
{
	constexpr spall::TextureUsageFlags usage =
		spall::TextureUsageFlags::Sampled |
		spall::TextureUsageFlags::TransferSource |
		spall::TextureUsageFlags::TransferDestination;
	const FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = usage});

	CHECK(spall::validateGenerateMipsArguments(texture) == spall::SUCCESS);
}

TEST_CASE(
	"Mipmap generation requires a mip chain",
	"[command][mips]")
{
	constexpr spall::TextureUsageFlags usage =
		spall::TextureUsageFlags::Sampled |
		spall::TextureUsageFlags::TransferSource |
		spall::TextureUsageFlags::TransferDestination;

	const FakeTexture texture(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = usage});

	CHECK(spall::validateGenerateMipsArguments(texture) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"Mipmap generation requires sampled and bidirectional transfer usage",
	"[command][mips]")
{
	const FakeTexture missingSampled(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferSource | spall::TextureUsageFlags::TransferDestination});

	const FakeTexture missingSource(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::TransferDestination});

	const FakeTexture missingDestination(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::Sampled | spall::TextureUsageFlags::TransferSource});

	CHECK(spall::validateGenerateMipsArguments(missingSampled) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateGenerateMipsArguments(missingSource) == spall::ERR_INVALID_USAGE_FLAGS);
	CHECK(spall::validateGenerateMipsArguments(missingDestination) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"A block-compressed upload measures its row pitch in blocks",
	"[command][copy][compressed]")
{
	const FakeTexture bc1 = compressedTexture();
	const FakeTexture bc7 = compressedTexture(spall::Format::BC7RGBAUnorm);
	const FakeBuffer buffer = uploadBuffer(64 * 64 * 4);
	const spall::TextureRegion baseMip = {0, 0, 0, 64, 64};

	CHECK(spall::validateTextureBufferCopyArguments(bc1, baseMip, buffer, 0, 16 * 8) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(bc1, baseMip, buffer, 0, (16 * 8) - 1) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(bc7, baseMip, buffer, 0, 16 * 16) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(bc7, baseMip, buffer, 0, (16 * 16) - 1) != spall::SUCCESS);
}

TEST_CASE(
	"A block-compressed upload sizes its buffer by rows of blocks",
	"[command][copy][compressed]")
{
	const FakeTexture texture = compressedTexture();
	const FakeBuffer exact = uploadBuffer(16 * 16 * 8);
	const FakeBuffer short1 = uploadBuffer((16 * 16 * 8) - 1);
	const spall::TextureRegion baseMip = {0, 0, 0, 64, 64};

	CHECK(spall::validateTextureBufferCopyArguments(texture, baseMip, exact, 0, 16 * 8) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, baseMip, short1, 0, 16 * 8) != spall::SUCCESS);
}

TEST_CASE(
	"A block-compressed region starts and ends on block boundaries",
	"[command][copy][compressed]")
{
	const FakeTexture texture = compressedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 4, 4, 8, 8}, buffer, 0, 2 * 8) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 2, 4, 8, 8}, buffer, 0, 2 * 8) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 4, 2, 8, 8}, buffer, 0, 2 * 8) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 6, 8}, buffer, 0, 2 * 8) != spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 8, 6}, buffer, 0, 2 * 8) != spall::SUCCESS);
}

TEST_CASE(
	"A block-compressed region may end short of a block at the mip edge",
	"[command][copy][compressed]")
{
	const FakeTexture texture = compressedTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {5, 0, 0, 2, 2}, buffer, 0, 8) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {6, 0, 0, 1, 1}, buffer, 0, 8) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {5}, buffer, 0, 8) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {5, 0, 0, 2, 2}, buffer, 0, 7) != spall::SUCCESS);
}

TEST_CASE(
	"Mipmap generation rejects block-compressed textures",
	"[command][mips][compressed]")
{
	constexpr spall::TextureUsageFlags usage =
		spall::TextureUsageFlags::Sampled |
		spall::TextureUsageFlags::TransferSource |
		spall::TextureUsageFlags::TransferDestination;

	spall::TextureInfo info = {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::BC3RGBAUnorm,
		.Usage = usage};
	const FakeTexture texture(info);

	CHECK(spall::validateGenerateMipsArguments(texture) == spall::ERR_UNSUPPORTED_FORMAT);
}

TEST_CASE(
	"Mipmap generation rejects depth textures",
	"[command][mips]")
{
	constexpr spall::TextureUsageFlags usage =
		spall::TextureUsageFlags::Sampled |
		spall::TextureUsageFlags::TransferSource |
		spall::TextureUsageFlags::TransferDestination;

	spall::TextureInfo info = {
		.Width = 64,
		.Height = 64,
		.MipLevels = 7,
		.Format = spall::Format::Depth32Float,
		.Usage = usage};
	const FakeTexture texture(info);

	CHECK(spall::validateGenerateMipsArguments(texture) == spall::ERR_INVALID_FORMAT);
}

TEST_CASE(
	"A texture upload rejects zero-sized destinations",
	"[command][copy]")
{
	spall::TextureInfo zeroWidthInfo = {
		.Height = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination};
	const FakeTexture zeroWidth(zeroWidthInfo);

	spall::TextureInfo zeroHeightInfo = {
		.Width = 64,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination};
	const FakeTexture zeroHeight(zeroHeightInfo);

	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(zeroWidth, {}, buffer, 0, 4) == spall::ERR_INVALID_SIZE);
	CHECK(spall::validateTextureBufferCopyArguments(zeroHeight, {}, buffer, 0, 4) == spall::ERR_INVALID_SIZE);
}

TEST_CASE(
	"A texture upload rejects formats without a pixel size",
	"[command][copy]")
{
	spall::TextureInfo info = {
		.Width = 64,
		.Height = 64,
		.Format = spall::Format::RGB32Float,
		.Usage = spall::TextureUsageFlags::TransferDestination};
	const FakeTexture texture(info);
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, {}, buffer, 0, 64 * 4) == spall::ERR_INVALID_FORMAT);
}

TEST_CASE(
	"A buffer copy rejects a shared source and destination",
	"[command][copy]")
{
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateCopyBufferArguments(buffer, 0, buffer, 0, 16) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer copy rejects a zero byte size",
	"[command][copy]")
{
	const FakeBuffer destination = uploadBuffer();
	const FakeBuffer source = uploadBuffer();

	CHECK(spall::validateCopyBufferArguments(destination, 0, source, 0, 0) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer copy rejects a range past either end",
	"[command][copy]")
{
	const FakeBuffer destination = uploadBuffer(256);
	const FakeBuffer source = uploadBuffer(256);

	CHECK(spall::validateCopyBufferArguments(destination, 250, source, 0, 16) != spall::SUCCESS);
	CHECK(spall::validateCopyBufferArguments(destination, 0, source, 250, 16) != spall::SUCCESS);
	CHECK(spall::validateCopyBufferArguments(destination, 0, source, 0, 257) != spall::SUCCESS);
}

TEST_CASE(
	"A buffer copy accepts a range that exactly fits",
	"[command][copy]")
{
	const FakeBuffer destination = uploadBuffer(256);
	const FakeBuffer source = uploadBuffer(256);

	CHECK(spall::validateCopyBufferArguments(destination, 0, source, 0, 256) == spall::SUCCESS);
	CHECK(spall::validateCopyBufferArguments(destination, 128, source, 128, 128) == spall::SUCCESS);
}

TEST_CASE(
	"A viewport must be finite",
	"[viewport]")
{
	const float infinity = std::numeric_limits<float>::infinity();
	const float notANumber = std::numeric_limits<float>::quiet_NaN();

	spall::Viewport viewport = {};
	viewport.Width = 64.0f;
	viewport.Height = 64.0f;
	viewport.MaxDepth = 1.0f;

	CHECK(spall::validateViewport(viewport) == spall::SUCCESS);

	spall::Viewport notFinite = viewport;
	notFinite.X = infinity;
	CHECK(spall::validateViewport(notFinite) != spall::SUCCESS);

	notFinite = viewport;
	notFinite.Y = notANumber;
	CHECK(spall::validateViewport(notFinite) != spall::SUCCESS);

	notFinite = viewport;
	notFinite.MinDepth = notANumber;
	CHECK(spall::validateViewport(notFinite) != spall::SUCCESS);
}

TEST_CASE(
	"A viewport must have positive dimensions",
	"[viewport]")
{
	spall::Viewport viewport = {};
	viewport.Width = 64.0f;
	viewport.Height = 64.0f;
	viewport.MaxDepth = 1.0f;

	spall::Viewport zeroWidth = viewport;
	zeroWidth.Width = 0.0f;
	CHECK(spall::validateViewport(zeroWidth) != spall::SUCCESS);

	spall::Viewport negativeHeight = viewport;
	negativeHeight.Height = -1.0f;
	CHECK(spall::validateViewport(negativeHeight) != spall::SUCCESS);
}

TEST_CASE(
	"A viewport rejects bounds that overflow to infinity",
	"[viewport]")
{
	spall::Viewport viewport = {};
	viewport.X = (std::numeric_limits<float>::max)();
	viewport.Width = (std::numeric_limits<float>::max)();
	viewport.Height = 64.0f;
	viewport.MaxDepth = 1.0f;

	CHECK(spall::validateViewport(viewport) != spall::SUCCESS);
}

TEST_CASE(
	"A viewport depth range stays within zero and one",
	"[viewport]")
{
	spall::Viewport viewport = {};
	viewport.Width = 64.0f;
	viewport.Height = 64.0f;
	viewport.MaxDepth = 1.0f;

	spall::Viewport negativeMin = viewport;
	negativeMin.MinDepth = -0.1f;
	CHECK(spall::validateViewport(negativeMin) != spall::SUCCESS);

	spall::Viewport tooDeep = viewport;
	tooDeep.MaxDepth = 1.1f;
	CHECK(spall::validateViewport(tooDeep) != spall::SUCCESS);

	spall::Viewport inverted = viewport;
	inverted.MinDepth = 0.9f;
	inverted.MaxDepth = 0.1f;
	CHECK(spall::validateViewport(inverted) != spall::SUCCESS);

	spall::Viewport flat = viewport;
	flat.MinDepth = 0.5f;
	flat.MaxDepth = 0.5f;
	CHECK(spall::validateViewport(flat) == spall::SUCCESS);
}

TEST_CASE(
	"A scissor offset must be nonnegative",
	"[scissor]")
{
	spall::Scissor scissor = {};
	scissor.Width = 64;
	scissor.Height = 64;

	CHECK(spall::validateScissor(scissor) == spall::SUCCESS);

	spall::Scissor negativeX = scissor;
	negativeX.X = -1;
	CHECK(spall::validateScissor(negativeX) != spall::SUCCESS);

	spall::Scissor negativeY = scissor;
	negativeY.Y = -1;
	CHECK(spall::validateScissor(negativeY) != spall::SUCCESS);
}

TEST_CASE(
	"A scissor rejects bounds past the coordinate maximum",
	"[scissor]")
{
	const std::int32_t maximum = (std::numeric_limits<std::int32_t>::max)();

	spall::Scissor scissor = {};
	scissor.X = maximum;
	scissor.Width = 1;
	scissor.Height = 1;
	CHECK(spall::validateScissor(scissor) != spall::SUCCESS);

	spall::Scissor tall = {};
	tall.Y = maximum;
	tall.Width = 1;
	tall.Height = 1;
	CHECK(spall::validateScissor(tall) != spall::SUCCESS);
}

TEST_CASE(
	"A scissor reaching exactly the coordinate maximum is accepted",
	"[scissor]")
{
	const std::int32_t maximum = (std::numeric_limits<std::int32_t>::max)();

	spall::Scissor scissor = {};
	scissor.X = maximum - 8;
	scissor.Width = 8;
	scissor.Height = 8;

	CHECK(spall::validateScissor(scissor) == spall::SUCCESS);
}

TEST_CASE(
	"An empty scissor is accepted",
	"[scissor]")
{
	const spall::Scissor scissor = {};

	CHECK(spall::validateScissor(scissor) == spall::SUCCESS);
}

TEST_CASE(
	"An upload selects an array layer",
	"[command][layers]")
{
	const FakeTexture texture = layeredTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {0, 0, 0, 64, 64, 5}, buffer, 0, 64 * 4) == spall::SUCCESS);
	CHECK(spall::validateTextureBufferCopyArguments(texture, spall::TextureRegion {3, 0, 0, 8, 8, 5}, buffer, 0, 8 * 4) == spall::SUCCESS);
}

TEST_CASE(
	"An upload rejects an array layer beyond the texture",
	"[command][layers]")
{
	const FakeTexture texture = layeredTexture();
	const FakeBuffer buffer = uploadBuffer();

	CHECK(
		spall::validateTextureBufferCopyArguments(
			texture,
			spall::TextureRegion {0, 0, 0, 64, 64, 6},
			buffer,
			0,
			64 * 4) == spall::ERR_INVALID_RANGE);

	const FakeTexture flat = mippedTexture();
	CHECK(spall::validateTextureBufferCopyArguments(flat, spall::TextureRegion {0, 0, 0, 64, 64, 1}, buffer, 0, 64 * 4) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"A texture copy requires matching layer counts",
	"[command][layers]")
{
	const FakeTexture source(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.ArrayLayers = 6,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferSource});
	const FakeTexture matching(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.ArrayLayers = 6,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination});
	const FakeTexture mismatched(spall::TextureInfo {
		.Width = 64,
		.Height = 64,
		.ArrayLayers = 3,
		.Format = spall::Format::RGBA8,
		.Usage = spall::TextureUsageFlags::TransferDestination});

	CHECK(spall::validateCopyTextureArguments(matching, source) == spall::SUCCESS);
	CHECK(spall::validateCopyTextureArguments(mismatched, source) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"Indirect arguments require an indirect buffer",
	"[command][indirect]")
{
	const FakeBuffer indirect(spall::BufferInfo {.Size = 256, .Usage = spall::BufferUsageFlags::Indirect});
	const FakeBuffer storage(spall::BufferInfo {.Size = 256, .Usage = spall::BufferUsageFlags::Storage});

	CHECK(spall::validateIndirectArguments(indirect, 0, sizeof(spall::DrawIndirectCommand)) == spall::SUCCESS);
	CHECK(spall::validateIndirectArguments(storage, 0, sizeof(spall::DrawIndirectCommand)) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"Indirect arguments must be four-byte aligned",
	"[command][indirect]")
{
	const FakeBuffer indirect(spall::BufferInfo {.Size = 256, .Usage = spall::BufferUsageFlags::Indirect});

	CHECK(spall::validateIndirectArguments(indirect, 4, sizeof(spall::DrawIndirectCommand)) == spall::SUCCESS);
	CHECK(spall::validateIndirectArguments(indirect, 16, sizeof(spall::DrawIndirectCommand)) == spall::SUCCESS);
	CHECK(spall::validateIndirectArguments(indirect, 1, sizeof(spall::DrawIndirectCommand)) == spall::ERR_INVALID_RANGE);
	CHECK(spall::validateIndirectArguments(indirect, 6, sizeof(spall::DrawIndirectCommand)) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"Indirect arguments stay inside the argument buffer",
	"[command][indirect]")
{
	const FakeBuffer indirect(spall::BufferInfo {.Size = 32, .Usage = spall::BufferUsageFlags::Indirect});
	const std::uint32_t maximum = (std::numeric_limits<std::uint32_t>::max)();

	CHECK(spall::validateIndirectArguments(indirect, 16, sizeof(spall::DrawIndirectCommand)) == spall::SUCCESS);
	CHECK(spall::validateIndirectArguments(indirect, 20, sizeof(spall::DrawIndirectCommand)) == spall::ERR_INVALID_RANGE);
	CHECK(spall::validateIndirectArguments(indirect, maximum - 3, sizeof(spall::DrawIndirectCommand)) == spall::ERR_INVALID_RANGE);
}

TEST_CASE(
	"Indirect argument layouts are the sizes both backends expect",
	"[command][indirect]")
{
	CHECK(sizeof(spall::DrawIndirectCommand) == 16);
	CHECK(sizeof(spall::DrawIndexedIndirectCommand) == 20);
	CHECK(sizeof(spall::DispatchIndirectCommand) == 12);
	CHECK(spall::IndirectArgumentAlignment == 4);
}
