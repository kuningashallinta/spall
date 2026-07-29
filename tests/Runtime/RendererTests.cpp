#include <catch2/catch_test_macros.hpp>

#include <spall/Runtime/Renderer.h>

#include <concepts>
#include <utility>

TEST_CASE(
	"Renderers are nullable move-only values",
	"[runtime][renderer]")
{
	STATIC_REQUIRE(std::movable<spall::Renderer>);
	STATIC_REQUIRE(not std::copyable<spall::Renderer>);

	spall::Renderer renderer;
	CHECK(not renderer);

	spall::Renderer moved = std::move(renderer);
	CHECK(not moved);
}

TEST_CASE(
	"Managed frames are nullable move-only values",
	"[runtime][frame]")
{
	STATIC_REQUIRE(std::movable<spall::Frame>);
	STATIC_REQUIRE(not std::copyable<spall::Frame>);

	spall::Frame frame;
	CHECK(not frame);

	spall::Frame moved = std::move(frame);
	CHECK(not moved);

	CHECK(moved.end() == spall::ERR_INVALID_STATE);
}

TEST_CASE(
	"A renderer reports a null output",
	"[runtime][renderer]")
{
	const spall::RendererCreateInfo info = {};

	CHECK(spall::Renderer::create(info, nullptr) == spall::ERR_INVALID_ARGUMENT);
}
