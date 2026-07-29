#include <catch2/catch_test_macros.hpp>

#include <spall/Device/IDevice.h>
#include <src/Validation/Common/DeviceValidation.h>

#include <concepts>

TEST_CASE(
	"Device capability queries are part of the device interface",
	"[device][capabilities][interface]")
{
	STATIC_REQUIRE(requires(const spall::IDevice& device, spall::FormatCapabilities* capabilities) {
		{ device.limits() } -> std::same_as<const spall::DeviceLimits&>;
		{ device.queryFormatCapabilities(spall::Format::RGBA8, capabilities) } -> std::same_as<spall::Status>;
	});
}

TEST_CASE(
	"Format capability query validation rejects invalid arguments",
	"[device][capabilities][validation]")
{
	spall::FormatCapabilities capabilities = {};

	CHECK(spall::validateFormatCapabilityQuery(spall::Format::RGBA8, nullptr) == spall::ERR_INVALID_ARGUMENT);
	CHECK(spall::validateFormatCapabilityQuery(spall::Format::Unknown, &capabilities) == spall::ERR_INVALID_FORMAT);
	CHECK(spall::validateFormatCapabilityQuery(spall::Format::RGBA8, &capabilities) == spall::SUCCESS);
	CHECK(spall::validateFormatCapabilityQuery(spall::Format::RGB32Float, &capabilities) == spall::SUCCESS);
}

TEST_CASE(
	"Device capability values default conservatively",
	"[device][capabilities]")
{
	const spall::DeviceLimits limits = {};
	const spall::FormatCapabilities capabilities = {};

	CHECK(limits.MaxTexture2DDimension == 0);
	CHECK(limits.MaxComputeStorageTextures == 0);
	CHECK(limits.MaxSamplerAnisotropy == 1.0f);
	CHECK(limits.MinLineWidth == 1.0f);
	CHECK(limits.MaxLineWidth == 1.0f);
	CHECK_FALSE(limits.SupportsTimestampQueries);
	CHECK_FALSE(limits.SupportsInlineRayTracing);
	CHECK_FALSE(limits.SupportsRayTracingPipeline);
	CHECK(limits.MaxRayRecursionDepth == 0);
	CHECK(capabilities.SupportedTextureUsages == spall::TextureUsageFlags::None);
	CHECK_FALSE(capabilities.SupportsVertexInput);
	CHECK_FALSE(capabilities.SupportsLinearFiltering);
	CHECK_FALSE(capabilities.SupportsBlending);
}
