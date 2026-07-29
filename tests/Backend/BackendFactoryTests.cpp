#include <catch2/catch_test_macros.hpp>

#include <spall/Backend/BackendFactory.h>

#include <memory>

TEST_CASE(
	"The factory creates every enabled backend",
	"[backend]")
{
#if SPALL_HAS_D3D12
	const std::unique_ptr<spall::IBackend> d3d12 = spall::createBackend(spall::RenderBackendType::D3D12);
	REQUIRE(d3d12);
	CHECK(d3d12->backendType() == spall::RenderBackendType::D3D12);
#endif

#if SPALL_HAS_VULKAN
	const std::unique_ptr<spall::IBackend> vulkan = spall::createBackend(spall::RenderBackendType::Vulkan);
	REQUIRE(vulkan);
	CHECK(vulkan->backendType() == spall::RenderBackendType::Vulkan);
#endif
}

TEST_CASE(
	"The factory rejects an unknown backend",
	"[backend]")
{
	std::unique_ptr<spall::IBackend> backend;
	const spall::Status status = spall::createBackend(static_cast<spall::RenderBackendType>(255), &backend);

	CHECK(status == spall::ERR_UNSUPPORTED_BACKEND);
	CHECK(not backend);
}

TEST_CASE(
	"The factory reports a null backend output",
	"[backend]")
{
	const spall::Status status = spall::createBackend(spall::RenderBackendType::D3D12, nullptr);

	CHECK(status == spall::ERR_INVALID_ARGUMENT);
}

TEST_CASE(
	"The factory rejects disabled backends",
	"[backend]")
{
#if SPALL_HAS_VULKAN == 0
	CHECK(not spall::createBackend(spall::RenderBackendType::Vulkan));
#endif

	SUCCEED();
}
