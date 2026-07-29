#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/FramebufferValidation.h>
#include <tests/Support/Fakes.h>

namespace
{
	using spall::tests::FakeTexture;
	using spall::tests::FakeTextureView;

	FakeTexture colorTexture()
	{
		return FakeTexture(spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment));
	}

	FakeTexture depthTexture()
	{
		spall::TextureInfo info = spall::tests::textureInfo(spall::TextureUsageFlags::DepthStencilAttachment);
		info.Format = spall::Format::Depth32Float;

		return FakeTexture(info);
	}

	FakeTexture sampledOnlyTexture()
	{
		return FakeTexture(spall::tests::textureInfo(spall::TextureUsageFlags::Sampled));
	}

	spall::FramebufferCreateInfo colorOnly(
		spall::ITextureView& view)
	{
		spall::FramebufferCreateInfo info = {};
		info.ColorAttachments[0] = &view;
		info.ColorAttachmentCount = 1;

		return info;
	}
} // namespace

TEST_CASE(
	"A framebuffer accepts a single color attachment",
	"[framebuffer]")
{
	FakeTexture texture = colorTexture();
	FakeTextureView view(texture, spall::TextureAspectFlags::Color);

	CHECK(spall::validateFramebufferCreateInfo(colorOnly(view)) == spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer accepts a depth attachment alone",
	"[framebuffer]")
{
	FakeTexture texture = depthTexture();
	FakeTextureView view(texture, spall::TextureAspectFlags::Depth);

	spall::FramebufferCreateInfo info = {};
	info.DepthAttachment = &view;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer accepts a combined depth-stencil aspect",
	"[framebuffer]")
{
	spall::TextureInfo textureInfo = spall::tests::textureInfo(spall::TextureUsageFlags::DepthStencilAttachment);
	textureInfo.Format = spall::Format::Depth24Stencil8;
	FakeTexture texture(textureInfo);
	FakeTextureView view(texture, spall::TextureAspectFlags::Depth | spall::TextureAspectFlags::Stencil);

	spall::FramebufferCreateInfo info = {};
	info.DepthAttachment = &view;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer requires at least one attachment",
	"[framebuffer]")
{
	const spall::FramebufferCreateInfo info = {};

	CHECK(spall::validateFramebufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer rejects more color attachments than the limit",
	"[framebuffer]")
{
	FakeTexture texture = colorTexture();
	FakeTextureView view(texture, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = colorOnly(view);
	info.ColorAttachmentCount = spall::MaxColorAttachments + 1;

	CHECK(spall::validateFramebufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer accepts exactly the color-attachment limit",
	"[framebuffer]")
{
	FakeTexture firstTexture = colorTexture();
	FakeTexture secondTexture = colorTexture();
	FakeTexture thirdTexture = colorTexture();
	FakeTexture fourthTexture = colorTexture();
	FakeTextureView first(firstTexture, spall::TextureAspectFlags::Color);
	FakeTextureView second(secondTexture, spall::TextureAspectFlags::Color);
	FakeTextureView third(thirdTexture, spall::TextureAspectFlags::Color);
	FakeTextureView fourth(fourthTexture, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = {};
	info.ColorAttachments[0] = &first;
	info.ColorAttachments[1] = &second;
	info.ColorAttachments[2] = &third;
	info.ColorAttachments[3] = &fourth;
	info.ColorAttachmentCount = spall::MaxColorAttachments;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer rejects a null color attachment",
	"[framebuffer]")
{
	spall::FramebufferCreateInfo info = {};
	info.ColorAttachmentCount = 1;

	CHECK(spall::validateFramebufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer color attachment must be a color-only view",
	"[framebuffer]")
{
	FakeTexture texture = colorTexture();
	FakeTextureView depthAspect(texture, spall::TextureAspectFlags::Depth);

	CHECK(spall::validateFramebufferCreateInfo(colorOnly(depthAspect)) != spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer color attachment requires color-attachment usage",
	"[framebuffer]")
{
	FakeTexture texture = sampledOnlyTexture();
	FakeTextureView view(texture, spall::TextureAspectFlags::Color);

	CHECK(spall::validateFramebufferCreateInfo(colorOnly(view)) != spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer color attachment requires a color format",
	"[framebuffer]")
{
	spall::TextureInfo textureInfo = spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment);
	textureInfo.Format = spall::Format::Depth32Float;
	FakeTexture texture(textureInfo);
	FakeTextureView view(texture, spall::TextureAspectFlags::Color);

	CHECK(spall::validateFramebufferCreateInfo(colorOnly(view)) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"A framebuffer depth attachment requires depth-stencil usage",
	"[framebuffer]")
{
	spall::TextureInfo textureInfo = spall::tests::textureInfo(spall::TextureUsageFlags::Sampled);
	textureInfo.Format = spall::Format::Depth32Float;
	FakeTexture texture(textureInfo);
	FakeTextureView view(texture, spall::TextureAspectFlags::Depth);

	spall::FramebufferCreateInfo info = {};
	info.DepthAttachment = &view;

	CHECK(spall::validateFramebufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer depth attachment requires a depth format",
	"[framebuffer]")
{
	FakeTexture texture(spall::tests::textureInfo(spall::TextureUsageFlags::DepthStencilAttachment));
	FakeTextureView view(texture, spall::TextureAspectFlags::Depth);

	spall::FramebufferCreateInfo info = {};
	info.DepthAttachment = &view;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::ERR_INVALID_USAGE_FLAGS);
}

TEST_CASE(
	"A framebuffer depth attachment must be a depth view",
	"[framebuffer]")
{
	FakeTexture texture = depthTexture();
	FakeTextureView colorAspect(texture, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = {};
	info.DepthAttachment = &colorAspect;

	CHECK(spall::validateFramebufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer rejects the same texture in two color slots",
	"[framebuffer]")
{
	FakeTexture texture = colorTexture();
	FakeTextureView first(texture, spall::TextureAspectFlags::Color);
	FakeTextureView second(texture, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = {};
	info.ColorAttachments[0] = &first;
	info.ColorAttachments[1] = &second;
	info.ColorAttachmentCount = 2;

	CHECK(spall::validateFramebufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer accepts distinct textures in each color slot",
	"[framebuffer]")
{
	FakeTexture firstTexture = colorTexture();
	FakeTexture secondTexture = colorTexture();
	FakeTextureView first(firstTexture, spall::TextureAspectFlags::Color);
	FakeTextureView second(secondTexture, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = {};
	info.ColorAttachments[0] = &first;
	info.ColorAttachments[1] = &second;
	info.ColorAttachmentCount = 2;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer accepts matching single-mip attachment views",
	"[framebuffer][mips]")
{
	spall::TextureInfo largeInfo = spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment, 7);
	spall::TextureInfo smallInfo = largeInfo;
	smallInfo.Width = 16;
	smallInfo.Height = 16;
	smallInfo.MipLevels = 5;
	FakeTexture largeTexture(largeInfo);
	FakeTexture smallTexture(smallInfo);
	FakeTextureView largeMip(largeTexture, spall::TextureAspectFlags::Color, 2, 1);
	FakeTextureView smallMip(smallTexture, spall::TextureAspectFlags::Color, 0, 1);

	spall::FramebufferCreateInfo info = {};
	info.ColorAttachments[0] = &largeMip;
	info.ColorAttachments[1] = &smallMip;
	info.ColorAttachmentCount = 2;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer rejects multi-mip and mismatched-mip attachment views",
	"[framebuffer][mips]")
{
	spall::TextureInfo textureInfo = spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment, 7);
	FakeTexture firstTexture(textureInfo);
	FakeTexture secondTexture(textureInfo);
	FakeTextureView multiMip(firstTexture, spall::TextureAspectFlags::Color, 0, 2);
	CHECK(spall::validateFramebufferCreateInfo(colorOnly(multiMip)) == spall::ERR_INVALID_RESOURCE);

	FakeTextureView baseMip(firstTexture, spall::TextureAspectFlags::Color, 0, 1);
	FakeTextureView smallerMip(secondTexture, spall::TextureAspectFlags::Color, 1, 1);
	spall::FramebufferCreateInfo mismatched = {};
	mismatched.ColorAttachments[0] = &baseMip;
	mismatched.ColorAttachments[1] = &smallerMip;
	mismatched.ColorAttachmentCount = 2;

	CHECK(spall::validateFramebufferCreateInfo(mismatched) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"A framebuffer rejects a null later color attachment",
	"[framebuffer]")
{
	FakeTexture texture = colorTexture();
	FakeTextureView view(texture, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = {};
	info.ColorAttachments[0] = &view;
	info.ColorAttachmentCount = 2;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"A framebuffer detects duplicate textures in later color slots",
	"[framebuffer]")
{
	FakeTexture firstTexture = colorTexture();
	FakeTexture secondTexture = colorTexture();
	FakeTextureView first(firstTexture, spall::TextureAspectFlags::Color);
	FakeTextureView second(secondTexture, spall::TextureAspectFlags::Color);
	FakeTextureView repeated(firstTexture, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = {};
	info.ColorAttachments[0] = &first;
	info.ColorAttachments[1] = &second;
	info.ColorAttachments[2] = &repeated;
	info.ColorAttachmentCount = 3;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"A framebuffer rejects a texture used as both color and depth",
	"[framebuffer]")
{
	spall::TextureInfo textureInfo = spall::tests::textureInfo(
		spall::TextureUsageFlags::ColorAttachment | spall::TextureUsageFlags::DepthStencilAttachment);
	FakeTexture texture(textureInfo);
	FakeTextureView colorView(texture, spall::TextureAspectFlags::Color);
	FakeTextureView depthView(texture, spall::TextureAspectFlags::Depth);

	spall::FramebufferCreateInfo info = colorOnly(colorView);
	info.DepthAttachment = &depthView;

	CHECK(spall::validateFramebufferCreateInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer attachment selects exactly one array layer",
	"[framebuffer][layers]")
{
	FakeTexture texture(spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment, 1, 6));
	FakeTextureView singleLayer(texture, spall::TextureAspectFlags::Color, 0, 1, 3, 1);
	FakeTextureView wholeArray(texture, spall::TextureAspectFlags::Color, 0, 1, 0, 6);

	CHECK(spall::validateFramebufferCreateInfo(colorOnly(singleLayer)) == spall::SUCCESS);
	CHECK(spall::validateFramebufferCreateInfo(colorOnly(wholeArray)) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"A framebuffer rejects a cubemap attachment",
	"[framebuffer][cubemap]")
{
	FakeTexture texture(spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment, 1, 6, true));
	FakeTextureView cubeView(texture, spall::TextureAspectFlags::Color, 0, 1, 0, 6, true);
	FakeTextureView faceView(texture, spall::TextureAspectFlags::Color, 0, 1, 2, 1);

	CHECK(spall::validateFramebufferCreateInfo(colorOnly(cubeView)) == spall::ERR_INVALID_RESOURCE);
	CHECK(spall::validateFramebufferCreateInfo(colorOnly(faceView)) == spall::SUCCESS);
}

TEST_CASE(
	"A multisampled framebuffer requires a resolve attachment",
	"[framebuffer][msaa]")
{
	spall::TextureInfo multisampledInfo = spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment);
	multisampledInfo.SampleCount = 4;
	FakeTexture multisampled(multisampledInfo);
	FakeTextureView colorView(multisampled, spall::TextureAspectFlags::Color);

	CHECK(spall::validateFramebufferCreateInfo(colorOnly(colorView)) == spall::ERR_INVALID_RESOURCE);

	FakeTexture resolveTexture = colorTexture();
	FakeTextureView resolveView(resolveTexture, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = colorOnly(colorView);
	info.ResolveAttachments[0] = &resolveView;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A single-sampled framebuffer rejects resolve attachments",
	"[framebuffer][msaa]")
{
	FakeTexture first = colorTexture();
	FakeTexture second = colorTexture();
	FakeTextureView colorView(first, spall::TextureAspectFlags::Color);
	FakeTextureView resolveView(second, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = colorOnly(colorView);
	info.ResolveAttachments[0] = &resolveView;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"Framebuffer attachments must share one sample count",
	"[framebuffer][msaa]")
{
	spall::TextureInfo multisampledInfo = spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment);
	multisampledInfo.SampleCount = 4;
	FakeTexture multisampled(multisampledInfo);
	FakeTextureView colorView(multisampled, spall::TextureAspectFlags::Color);

	FakeTexture singleSampledDepth = depthTexture();
	FakeTextureView depthView(singleSampledDepth, spall::TextureAspectFlags::Depth);

	FakeTexture resolveTexture = colorTexture();
	FakeTextureView resolveView(resolveTexture, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo info = colorOnly(colorView);
	info.ResolveAttachments[0] = &resolveView;
	info.DepthAttachment = &depthView;

	CHECK(spall::validateFramebufferCreateInfo(info) == spall::ERR_INVALID_RESOURCE);
}

TEST_CASE(
	"A resolve attachment must be single sampled and match its format",
	"[framebuffer][msaa]")
{
	spall::TextureInfo multisampledInfo = spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment);
	multisampledInfo.SampleCount = 4;
	FakeTexture multisampled(multisampledInfo);
	FakeTextureView colorView(multisampled, spall::TextureAspectFlags::Color);

	FakeTexture multisampledResolve(multisampledInfo);
	FakeTextureView multisampledResolveView(multisampledResolve, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo sampledResolve = colorOnly(colorView);
	sampledResolve.ResolveAttachments[0] = &multisampledResolveView;
	CHECK(spall::validateFramebufferCreateInfo(sampledResolve) == spall::ERR_INVALID_RESOURCE);

	spall::TextureInfo mismatchedInfo = spall::tests::textureInfo(spall::TextureUsageFlags::ColorAttachment);
	mismatchedInfo.Format = spall::Format::BGRA8;
	FakeTexture mismatched(mismatchedInfo);
	FakeTextureView mismatchedView(mismatched, spall::TextureAspectFlags::Color);

	spall::FramebufferCreateInfo mismatchedResolve = colorOnly(colorView);
	mismatchedResolve.ResolveAttachments[0] = &mismatchedView;
	CHECK(spall::validateFramebufferCreateInfo(mismatchedResolve) == spall::ERR_INVALID_FORMAT);
}
