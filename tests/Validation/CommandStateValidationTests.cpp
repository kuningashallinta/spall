#include <catch2/catch_test_macros.hpp>

#include <src/Validation/Common/CommandStateValidation.h>

#include <span>

namespace
{
	int VertexBufferA = 0;
	int VertexBufferB = 0;

	spall::VertexBindingInfo binding(
		std::uint32_t slot,
		std::uint32_t stride)
	{
		spall::VertexBindingInfo info = {};
		info.Binding = slot;
		info.Stride = stride;

		return info;
	}

	spall::BoundVertexBuffer bound(
		const void* resource,
		std::uint32_t stride)
	{
		spall::BoundVertexBuffer slot = {};
		slot.Resource = resource;
		slot.Stride = stride;

		return slot;
	}

	spall::Status drawState(
		std::span<const spall::VertexBindingInfo> pipelineBindings,
		std::span<const spall::BoundVertexBuffer> boundSlots,
		bool hasIndexBuffer = true,
		bool indexed = false)
	{
		return spall::validateDrawState(true, true, true, true, pipelineBindings, boundSlots, hasIndexBuffer, indexed);
	}
} // namespace

TEST_CASE(
	"Recording state must match what the call expects",
	"[commandstate]")
{
	CHECK(spall::validateRecordingState(true, true) == spall::SUCCESS);
	CHECK(spall::validateRecordingState(false, false) == spall::SUCCESS);
	CHECK(spall::validateRecordingState(false, true) != spall::SUCCESS);
	CHECK(spall::validateRecordingState(true, false) != spall::SUCCESS);
}

TEST_CASE(
	"Debug labels must contain text",
	"[commandstate][debugmarker]")
{
	CHECK(spall::validateDebugLabel("Frame") == spall::SUCCESS);
	CHECK(spall::validateDebugLabel("") == spall::ERR_INVALID_ARGUMENT);
	CHECK(spall::validateDebugLabel(nullptr) == spall::ERR_INVALID_ARGUMENT);
}

TEST_CASE(
	"A dispatch cannot happen inside a render pass",
	"[commandstate]")
{
	CHECK(spall::validateDispatchState(true, true) != spall::SUCCESS);
	CHECK(spall::validateDispatchState(false, true) == spall::SUCCESS);
}

TEST_CASE(
	"A dispatch requires a compute pipeline",
	"[commandstate]")
{
	CHECK(spall::validateDispatchState(false, false) != spall::SUCCESS);
}

TEST_CASE(
	"A draw requires an active render pass and a graphics pipeline",
	"[commandstate]")
{
	const std::span<const spall::VertexBindingInfo> noBindings;
	const std::span<const spall::BoundVertexBuffer> noSlots;

	CHECK(spall::validateDrawState(false, true, true, true, noBindings, noSlots, true, false) != spall::SUCCESS);
	CHECK(spall::validateDrawState(true, false, true, true, noBindings, noSlots, true, false) != spall::SUCCESS);
	CHECK(spall::validateDrawState(true, true, true, true, noBindings, noSlots, true, false) == spall::SUCCESS);
}

TEST_CASE(
	"A draw requires both a viewport and a scissor",
	"[commandstate]")
{
	const std::span<const spall::VertexBindingInfo> noBindings;
	const std::span<const spall::BoundVertexBuffer> noSlots;

	CHECK(spall::validateDrawState(true, true, false, true, noBindings, noSlots, true, false) != spall::SUCCESS);
	CHECK(spall::validateDrawState(true, true, true, false, noBindings, noSlots, true, false) != spall::SUCCESS);
	CHECK(spall::validateDrawState(true, true, false, false, noBindings, noSlots, true, false) != spall::SUCCESS);
}

TEST_CASE(
	"A draw requires every vertex buffer the pipeline binds",
	"[commandstate]")
{
	const spall::VertexBindingInfo bindings[] = {binding(0, 8)};
	const spall::BoundVertexBuffer slots[] = {bound(&VertexBufferA, 8)};
	const spall::BoundVertexBuffer emptySlot[] = {bound(nullptr, 8)};

	CHECK(drawState(bindings, slots) == spall::SUCCESS);
	CHECK(drawState(bindings, emptySlot) != spall::SUCCESS);
}

TEST_CASE(
	"A draw rejects a binding beyond the bound slots",
	"[commandstate]")
{
	const spall::VertexBindingInfo bindings[] = {binding(3, 8)};
	const spall::BoundVertexBuffer slots[] = {bound(&VertexBufferA, 8)};

	CHECK(drawState(bindings, slots) != spall::SUCCESS);
}

TEST_CASE(
	"A draw rejects a vertex stride the pipeline does not expect",
	"[commandstate]")
{
	const spall::VertexBindingInfo bindings[] = {binding(0, 8)};
	const spall::BoundVertexBuffer wrongStride[] = {bound(&VertexBufferA, 12)};

	CHECK(drawState(bindings, wrongStride) != spall::SUCCESS);
}

TEST_CASE(
	"A draw checks every binding, not only the first",
	"[commandstate]")
{
	const spall::VertexBindingInfo bindings[] = {binding(0, 8), binding(1, 12)};
	const spall::BoundVertexBuffer good[] = {bound(&VertexBufferA, 8), bound(&VertexBufferB, 12)};
	const spall::BoundVertexBuffer secondWrong[] = {bound(&VertexBufferA, 8), bound(&VertexBufferB, 4)};
	const spall::BoundVertexBuffer secondMissing[] = {bound(&VertexBufferA, 8), bound(nullptr, 12)};

	CHECK(drawState(bindings, good) == spall::SUCCESS);
	CHECK(drawState(bindings, secondWrong) != spall::SUCCESS);
	CHECK(drawState(bindings, secondMissing) != spall::SUCCESS);
}

TEST_CASE(
	"An indexed draw requires an index buffer",
	"[commandstate]")
{
	const std::span<const spall::VertexBindingInfo> noBindings;
	const std::span<const spall::BoundVertexBuffer> noSlots;

	CHECK(drawState(noBindings, noSlots, false, true) != spall::SUCCESS);
	CHECK(drawState(noBindings, noSlots, true, true) == spall::SUCCESS);
}

TEST_CASE(
	"A non-indexed draw does not need an index buffer",
	"[commandstate]")
{
	const std::span<const spall::VertexBindingInfo> noBindings;
	const std::span<const spall::BoundVertexBuffer> noSlots;

	CHECK(drawState(noBindings, noSlots, false, false) == spall::SUCCESS);
}

TEST_CASE(
	"A render pass may sample textures it does not attach",
	"[commandstate]")
{
	const void* sampled[] = {&VertexBufferA};
	const void* attachments[] = {&VertexBufferB};

	CHECK(spall::validateNoSampledAttachmentAliasing(sampled, attachments) == spall::SUCCESS);
}

TEST_CASE(
	"A render pass cannot sample a texture it attaches",
	"[commandstate]")
{
	const void* sampled[] = {&VertexBufferA};
	const void* attachments[] = {&VertexBufferB, &VertexBufferA};

	CHECK(spall::validateNoSampledAttachmentAliasing(sampled, attachments) != spall::SUCCESS);
}

TEST_CASE(
	"Aliasing ignores empty sampled slots",
	"[commandstate]")
{
	const void* sampled[] = {nullptr};
	const void* attachments[] = {nullptr};

	CHECK(spall::validateNoSampledAttachmentAliasing(sampled, attachments) == spall::SUCCESS);
}

TEST_CASE(
	"Aliasing is checked across every pairing",
	"[commandstate]")
{
	const void* sampled[] = {&VertexBufferA, &VertexBufferB};
	const void* attachments[] = {&VertexBufferB};

	CHECK(spall::validateNoSampledAttachmentAliasing(sampled, attachments) != spall::SUCCESS);
}
