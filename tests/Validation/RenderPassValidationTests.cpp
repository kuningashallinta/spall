#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/RenderPassValidation.h>
#include <tests/Support/Fakes.h>

#include <limits>

static spall::FramebufferInfo colorFramebufferInfo()
{
	spall::FramebufferInfo info = {};
	info.ColorFormats[0] = spall::Format::RGBA8;
	info.ColorFormatCount = 1;

	return info;
}

static spall::FramebufferInfo depthFramebufferInfo(
	spall::Format depthFormat = spall::Format::Depth32Float)
{
	spall::FramebufferInfo info = {};
	info.DepthFormat = depthFormat;

	return info;
}

static spall::RenderPassBeginInfo colorPass(
	spall::IFramebuffer& framebuffer)
{
	spall::RenderPassBeginInfo info = {};
	info.Framebuffer = &framebuffer;
	info.ColorAttachments[0].LoadAction = spall::LoadAction::Clear;
	info.ColorAttachments[0].StoreAction = spall::StoreAction::Store;

	return info;
}

static spall::RenderPassBeginInfo depthPass(
	spall::IFramebuffer& framebuffer)
{
	spall::RenderPassBeginInfo info = {};
	info.Framebuffer = &framebuffer;
	info.DepthAttachment.DepthLoadAction = spall::LoadAction::Clear;
	info.DepthAttachment.DepthStoreAction = spall::StoreAction::Store;
	info.DepthAttachment.StencilLoadAction = spall::LoadAction::DontCare;
	info.DepthAttachment.StencilStoreAction = spall::StoreAction::DontCare;
	info.DepthAttachment.ClearDepth = 1.0f;

	return info;
}

TEST_CASE(
	"A render pass requires a framebuffer",
	"[renderpass]")
{
	const spall::RenderPassBeginInfo info = {};

	CHECK(spall::validatePassBeginInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A color render pass is accepted",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(colorFramebufferInfo());

	CHECK(spall::validatePassBeginInfo(colorPass(framebuffer)) == spall::SUCCESS);
}

TEST_CASE(
	"A framebuffer with no attachments is rejected",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(spall::FramebufferInfo {});

	spall::RenderPassBeginInfo info = {};
	info.Framebuffer = &framebuffer;

	CHECK(spall::validatePassBeginInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A render pass rejects an invalid color load action",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(colorFramebufferInfo());

	spall::RenderPassBeginInfo info = colorPass(framebuffer);
	info.ColorAttachments[0].LoadAction = static_cast<spall::LoadAction>(99);

	CHECK(spall::validatePassBeginInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A render pass rejects an invalid color store action",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(colorFramebufferInfo());

	spall::RenderPassBeginInfo info = colorPass(framebuffer);
	info.ColorAttachments[0].StoreAction = static_cast<spall::StoreAction>(99);

	CHECK(spall::validatePassBeginInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A cleared color attachment must have a finite clear color",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(colorFramebufferInfo());

	spall::RenderPassBeginInfo info = colorPass(framebuffer);
	info.ColorAttachments[0].ClearColor.G = std::numeric_limits<float>::infinity();

	CHECK(spall::validatePassBeginInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A non-finite clear color is ignored when the attachment is not cleared",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(colorFramebufferInfo());

	spall::RenderPassBeginInfo info = colorPass(framebuffer);
	info.ColorAttachments[0].LoadAction = spall::LoadAction::Load;
	info.ColorAttachments[0].ClearColor.G = std::numeric_limits<float>::quiet_NaN();

	CHECK(spall::validatePassBeginInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A depth render pass is accepted",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(depthFramebufferInfo());

	CHECK(spall::validatePassBeginInfo(depthPass(framebuffer)) == spall::SUCCESS);
}

TEST_CASE(
	"A render pass clear depth stays within zero and one",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(depthFramebufferInfo());

	spall::RenderPassBeginInfo tooDeep = depthPass(framebuffer);
	tooDeep.DepthAttachment.ClearDepth = 1.5f;
	CHECK(spall::validatePassBeginInfo(tooDeep) != spall::SUCCESS);

	spall::RenderPassBeginInfo negative = depthPass(framebuffer);
	negative.DepthAttachment.ClearDepth = -0.1f;
	CHECK(spall::validatePassBeginInfo(negative) != spall::SUCCESS);

	spall::RenderPassBeginInfo notFinite = depthPass(framebuffer);
	notFinite.DepthAttachment.ClearDepth = std::numeric_limits<float>::infinity();
	CHECK(spall::validatePassBeginInfo(notFinite) != spall::SUCCESS);
}

TEST_CASE(
	"A clear depth outside the range is ignored when depth is not cleared",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(depthFramebufferInfo());

	spall::RenderPassBeginInfo info = depthPass(framebuffer);
	info.DepthAttachment.DepthLoadAction = spall::LoadAction::Load;
	info.DepthAttachment.ClearDepth = 5.0f;

	CHECK(spall::validatePassBeginInfo(info) == spall::SUCCESS);
}

TEST_CASE(
	"A depth-only format cannot clear stencil",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(depthFramebufferInfo(spall::Format::Depth32Float));

	spall::RenderPassBeginInfo info = depthPass(framebuffer);
	info.DepthAttachment.StencilLoadAction = spall::LoadAction::Clear;

	CHECK(spall::validatePassBeginInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A depth-stencil format may clear stencil",
	"[renderpass]")
{
	FakeFramebuffer depth24Framebuffer(depthFramebufferInfo(spall::Format::Depth24Stencil8));
	FakeFramebuffer depth32Framebuffer(depthFramebufferInfo(spall::Format::Depth32FloatStencil8));

	spall::RenderPassBeginInfo depth24Pass = depthPass(depth24Framebuffer);
	depth24Pass.DepthAttachment.StencilLoadAction = spall::LoadAction::Clear;
	spall::RenderPassBeginInfo depth32Pass = depthPass(depth32Framebuffer);
	depth32Pass.DepthAttachment.StencilLoadAction = spall::LoadAction::Clear;

	CHECK(spall::validatePassBeginInfo(depth24Pass) == spall::SUCCESS);
	CHECK(spall::validatePassBeginInfo(depth32Pass) == spall::SUCCESS);
}

TEST_CASE(
	"A render pass rejects an invalid depth store action",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(depthFramebufferInfo());

	spall::RenderPassBeginInfo info = depthPass(framebuffer);
	info.DepthAttachment.DepthStoreAction = static_cast<spall::StoreAction>(99);

	CHECK(spall::validatePassBeginInfo(info) != spall::SUCCESS);
}

TEST_CASE(
	"A render pass rejects invalid depth and stencil load actions",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(depthFramebufferInfo());

	spall::RenderPassBeginInfo invalidDepth = depthPass(framebuffer);
	invalidDepth.DepthAttachment.DepthLoadAction = static_cast<spall::LoadAction>(99);

	spall::RenderPassBeginInfo invalidStencil = depthPass(framebuffer);
	invalidStencil.DepthAttachment.StencilLoadAction = static_cast<spall::LoadAction>(99);

	CHECK(spall::validatePassBeginInfo(invalidDepth) == spall::ERR_INVALID_ARGUMENT);
	CHECK(spall::validatePassBeginInfo(invalidStencil) == spall::ERR_INVALID_ARGUMENT);
}

TEST_CASE(
	"A render pass rejects an invalid stencil store action",
	"[renderpass]")
{
	FakeFramebuffer framebuffer(depthFramebufferInfo());

	spall::RenderPassBeginInfo info = depthPass(framebuffer);
	info.DepthAttachment.StencilStoreAction = static_cast<spall::StoreAction>(99);

	CHECK(spall::validatePassBeginInfo(info) == spall::ERR_INVALID_ARGUMENT);
}

TEST_CASE(
	"A render pass rejects more color attachments than the limit",
	"[renderpass]")
{
	spall::FramebufferInfo framebufferInfo = colorFramebufferInfo();
	framebufferInfo.ColorFormatCount = spall::MaxColorAttachments + 1;
	FakeFramebuffer framebuffer(framebufferInfo);

	spall::RenderPassBeginInfo info = colorPass(framebuffer);

	CHECK(spall::validatePassBeginInfo(info) == spall::ERR_INVALID_RESOURCE_STATE);
}
