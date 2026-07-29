#include <catch2/catch_test_macros.hpp>

#include <src/Backends/Vulkan/Common/VK_BindingMappings.h>
#include <src/Backends/Vulkan/Common/VK_FormatMappings.h>
#include <src/Backends/Vulkan/Common/VK_ImageMappings.h>
#include <src/Backends/Vulkan/Common/VK_VertexInputMappings.h>
#include <src/Validation/Common/FormatValidation.h>

#include <cstddef>
#include <optional>

namespace
{
	constexpr std::size_t LastFormatIndex = static_cast<std::size_t>(spall::Format::BC7RGBASrgb);

	struct Mapping
	{
		spall::Format Spall;
		VkFormat Vulkan;
	};

	const Mapping PortableMappings[] = {
		{spall::Format::R8Snorm, VK_FORMAT_R8_SNORM},
		{spall::Format::R8UInt, VK_FORMAT_R8_UINT},
		{spall::Format::R8SInt, VK_FORMAT_R8_SINT},
		{spall::Format::R16Unorm, VK_FORMAT_R16_UNORM},
		{spall::Format::R16Snorm, VK_FORMAT_R16_SNORM},
		{spall::Format::R16UInt, VK_FORMAT_R16_UINT},
		{spall::Format::R16SInt, VK_FORMAT_R16_SINT},
		{spall::Format::R16Float, VK_FORMAT_R16_SFLOAT},
		{spall::Format::R32UInt, VK_FORMAT_R32_UINT},
		{spall::Format::R32SInt, VK_FORMAT_R32_SINT},
		{spall::Format::RG8Unorm, VK_FORMAT_R8G8_UNORM},
		{spall::Format::RG8Snorm, VK_FORMAT_R8G8_SNORM},
		{spall::Format::RG8UInt, VK_FORMAT_R8G8_UINT},
		{spall::Format::RG8SInt, VK_FORMAT_R8G8_SINT},
		{spall::Format::RG16Unorm, VK_FORMAT_R16G16_UNORM},
		{spall::Format::RG16Snorm, VK_FORMAT_R16G16_SNORM},
		{spall::Format::RG16UInt, VK_FORMAT_R16G16_UINT},
		{spall::Format::RG16SInt, VK_FORMAT_R16G16_SINT},
		{spall::Format::RG32UInt, VK_FORMAT_R32G32_UINT},
		{spall::Format::RG32SInt, VK_FORMAT_R32G32_SINT},
		{spall::Format::RGBA8Snorm, VK_FORMAT_R8G8B8A8_SNORM},
		{spall::Format::RGBA8UInt, VK_FORMAT_R8G8B8A8_UINT},
		{spall::Format::RGBA8SInt, VK_FORMAT_R8G8B8A8_SINT},
		{spall::Format::RGBA16Unorm, VK_FORMAT_R16G16B16A16_UNORM},
		{spall::Format::RGBA16Snorm, VK_FORMAT_R16G16B16A16_SNORM},
		{spall::Format::RGBA16UInt, VK_FORMAT_R16G16B16A16_UINT},
		{spall::Format::RGBA16SInt, VK_FORMAT_R16G16B16A16_SINT},
		{spall::Format::RGBA32UInt, VK_FORMAT_R32G32B32A32_UINT},
		{spall::Format::RGBA32SInt, VK_FORMAT_R32G32B32A32_SINT},
		{spall::Format::RGB10A2Unorm, VK_FORMAT_A2B10G10R10_UNORM_PACK32},
		{spall::Format::RGB10A2UInt, VK_FORMAT_A2B10G10R10_UINT_PACK32},
		{spall::Format::RG11B10Float, VK_FORMAT_B10G11R11_UFLOAT_PACK32},
		{spall::Format::RGB9E5Float, VK_FORMAT_E5B9G9R9_UFLOAT_PACK32}};
} // namespace

TEST_CASE(
	"Vulkan maps sRGB formats in both directions",
	"[vulkan][format][srgb]")
{
	CHECK(spall::vk::toVkFormat(spall::Format::RGBA8Srgb) == VK_FORMAT_R8G8B8A8_SRGB);
	CHECK(spall::vk::toVkFormat(spall::Format::BGRA8Srgb) == VK_FORMAT_B8G8R8A8_SRGB);

	CHECK(spall::vk::toSpallFormat(VK_FORMAT_R8G8B8A8_SRGB) == spall::Format::RGBA8Srgb);
	CHECK(spall::vk::toSpallFormat(VK_FORMAT_B8G8R8A8_SRGB) == spall::Format::BGRA8Srgb);
}

TEST_CASE(
	"Vulkan treats sRGB formats as four-byte color textures",
	"[vulkan][format][srgb]")
{
	CHECK(spall::vk::defaultAspects(spall::Format::RGBA8Srgb) == spall::TextureAspectFlags::Color);
	CHECK(spall::vk::defaultAspects(spall::Format::BGRA8Srgb) == spall::TextureAspectFlags::Color);

	REQUIRE(spall::vk::textureFormatInfo(spall::Format::RGBA8Srgb).has_value());
	REQUIRE(spall::vk::textureFormatInfo(spall::Format::BGRA8Srgb).has_value());
	CHECK(spall::vk::textureFormatInfo(spall::Format::RGBA8Srgb)->bytesPerBlock == 4);
	CHECK(spall::vk::textureFormatInfo(spall::Format::BGRA8Srgb)->bytesPerBlock == 4);
}

TEST_CASE(
	"Vulkan maps portable depth formats in both directions",
	"[vulkan][format][depth]")
{
	const Mapping mappings[] = {
		{spall::Format::Depth16Unorm, VK_FORMAT_D16_UNORM},
		{spall::Format::Depth24Stencil8, VK_FORMAT_D24_UNORM_S8_UINT},
		{spall::Format::Depth32Float, VK_FORMAT_D32_SFLOAT},
		{spall::Format::Depth32FloatStencil8, VK_FORMAT_D32_SFLOAT_S8_UINT}};

	for (const Mapping& mapping : mappings)
	{
		CHECK(spall::vk::toVkFormat(mapping.Spall) == mapping.Vulkan);
		CHECK(spall::vk::toSpallFormat(mapping.Vulkan) == mapping.Spall);
	}
}

TEST_CASE(
	"Vulkan assigns depth formats their native aspects",
	"[vulkan][format][depth]")
{
	CHECK(spall::vk::defaultAspects(spall::Format::Depth16Unorm) == spall::TextureAspectFlags::Depth);
	CHECK(spall::vk::defaultAspects(spall::Format::Depth32Float) == spall::TextureAspectFlags::Depth);
	CHECK(spall::vk::defaultAspects(spall::Format::Depth24Stencil8) ==
		(spall::TextureAspectFlags::Depth | spall::TextureAspectFlags::Stencil));
	CHECK(spall::vk::defaultAspects(spall::Format::Depth32FloatStencil8) ==
		(spall::TextureAspectFlags::Depth | spall::TextureAspectFlags::Stencil));
}

TEST_CASE(
	"Vulkan maps portable formats in both directions",
	"[vulkan][format]")
{
	for (const Mapping& mapping : PortableMappings)
	{
		CHECK(spall::vk::toVkFormat(mapping.Spall) == mapping.Vulkan);
		CHECK(spall::vk::toSpallFormat(mapping.Vulkan) == mapping.Spall);
	}
}

TEST_CASE(
	"Vulkan portable formats have color aspects and matching byte sizes",
	"[vulkan][format]")
{
	for (const Mapping& mapping : PortableMappings)
	{
		const std::optional<spall::vk::TextureFormatProperties> properties = spall::vk::textureFormatInfo(mapping.Spall);

		REQUIRE(properties.has_value());
		CHECK(spall::vk::defaultAspects(mapping.Spall) == spall::TextureAspectFlags::Color);
		CHECK(properties->bytesPerBlock == spall::formatBytesPerPixel(mapping.Spall));
		CHECK(properties->blockWidth == 1);
		CHECK(properties->blockHeight == 1);
	}
}

TEST_CASE(
	"Vulkan exposes applicable portable formats to vertex input",
	"[vulkan][format][vertex]")
{
	for (const Mapping& mapping : PortableMappings)
	{
		const std::optional<spall::vk::VertexFormatProperties> properties = spall::vk::vertexFormatInfo(mapping.Spall);

		if (mapping.Spall == spall::Format::RGB9E5Float)
		{
			CHECK_FALSE(properties.has_value());
			continue;
		}

		REQUIRE(properties.has_value());
		CHECK(properties->format == mapping.Vulkan);
		CHECK(properties->size == spall::formatBytesPerPixel(mapping.Spall));
	}
}

TEST_CASE(
	"Vulkan vertex mapping preserves existing vertex-only and texture-only formats",
	"[vulkan][format][vertex]")
{
	const std::optional<spall::vk::VertexFormatProperties> rgb32 = spall::vk::vertexFormatInfo(spall::Format::RGB32Float);

	REQUIRE(rgb32.has_value());
	CHECK(rgb32->format == VK_FORMAT_R32G32B32_SFLOAT);
	CHECK(rgb32->size == 12);
	CHECK_FALSE(spall::vk::vertexFormatInfo(spall::Format::RGBA8Srgb).has_value());
	CHECK_FALSE(spall::vk::vertexFormatInfo(spall::Format::BGRA8Srgb).has_value());
}

TEST_CASE(
	"Vulkan reports portable color format capabilities",
	"[vulkan][format][capabilities]")
{
	VkFormatProperties properties = {};
	properties.bufferFeatures = VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
	properties.optimalTilingFeatures =
		VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
		VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
		VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
		VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
		VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

	const spall::FormatCapabilities capabilities = spall::vk::formatCapabilities(spall::Format::RGBA8, properties);

	CHECK((capabilities.SupportedTextureUsages & spall::TextureUsageFlags::TransferSource) != spall::TextureUsageFlags::None);
	CHECK((capabilities.SupportedTextureUsages & spall::TextureUsageFlags::TransferDestination) != spall::TextureUsageFlags::None);
	CHECK((capabilities.SupportedTextureUsages & spall::TextureUsageFlags::ColorAttachment) != spall::TextureUsageFlags::None);
	CHECK((capabilities.SupportedTextureUsages & spall::TextureUsageFlags::Sampled) != spall::TextureUsageFlags::None);
	CHECK((capabilities.SupportedTextureUsages & spall::TextureUsageFlags::Storage) != spall::TextureUsageFlags::None);
	CHECK(capabilities.SupportsVertexInput);
	CHECK(capabilities.SupportsLinearFiltering);
	CHECK(capabilities.SupportsBlending);
}

TEST_CASE(
	"Vulkan filters native support through Spall RHI format semantics",
	"[vulkan][format][capabilities]")
{
	VkFormatProperties properties = {};
	properties.bufferFeatures = VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
	properties.optimalTilingFeatures =
		VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
		VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
		VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT |
		VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BLEND_BIT |
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT |
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
		VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT |
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

	const spall::FormatCapabilities vertexOnly = spall::vk::formatCapabilities(spall::Format::RGB32Float, properties);
	CHECK(vertexOnly.SupportsVertexInput);
	CHECK(vertexOnly.SupportedTextureUsages == spall::TextureUsageFlags::None);

	const spall::FormatCapabilities depth = spall::vk::formatCapabilities(spall::Format::Depth32Float, properties);
	CHECK((depth.SupportedTextureUsages & spall::TextureUsageFlags::DepthStencilAttachment) != spall::TextureUsageFlags::None);
	CHECK((depth.SupportedTextureUsages & spall::TextureUsageFlags::Sampled) == spall::TextureUsageFlags::None);
	CHECK((depth.SupportedTextureUsages & spall::TextureUsageFlags::Storage) == spall::TextureUsageFlags::None);
	CHECK_FALSE(depth.SupportsLinearFiltering);
	CHECK_FALSE(depth.SupportsBlending);
}

TEST_CASE(
	"Vulkan maps block-compressed formats in both directions",
	"[vulkan][format][compressed]")
{
	const Mapping mappings[] = {
		{spall::Format::BC1RGBAUnorm, VK_FORMAT_BC1_RGBA_UNORM_BLOCK},
		{spall::Format::BC1RGBASrgb, VK_FORMAT_BC1_RGBA_SRGB_BLOCK},
		{spall::Format::BC2RGBAUnorm, VK_FORMAT_BC2_UNORM_BLOCK},
		{spall::Format::BC2RGBASrgb, VK_FORMAT_BC2_SRGB_BLOCK},
		{spall::Format::BC3RGBAUnorm, VK_FORMAT_BC3_UNORM_BLOCK},
		{spall::Format::BC3RGBASrgb, VK_FORMAT_BC3_SRGB_BLOCK},
		{spall::Format::BC4RUnorm, VK_FORMAT_BC4_UNORM_BLOCK},
		{spall::Format::BC4RSnorm, VK_FORMAT_BC4_SNORM_BLOCK},
		{spall::Format::BC5RGUnorm, VK_FORMAT_BC5_UNORM_BLOCK},
		{spall::Format::BC5RGSnorm, VK_FORMAT_BC5_SNORM_BLOCK},
		{spall::Format::BC6HRGBUFloat, VK_FORMAT_BC6H_UFLOAT_BLOCK},
		{spall::Format::BC6HRGBSFloat, VK_FORMAT_BC6H_SFLOAT_BLOCK},
		{spall::Format::BC7RGBAUnorm, VK_FORMAT_BC7_UNORM_BLOCK},
		{spall::Format::BC7RGBASrgb, VK_FORMAT_BC7_SRGB_BLOCK}};

	for (const Mapping& mapping : mappings)
	{
		CHECK(spall::vk::toVkFormat(mapping.Spall) == mapping.Vulkan);
		CHECK(spall::vk::toSpallFormat(mapping.Vulkan) == mapping.Spall);
	}
}

TEST_CASE(
	"Vulkan describes block-compressed formats by their block geometry",
	"[vulkan][format][compressed]")
{
	const std::optional<spall::vk::TextureFormatProperties> bc1 = spall::vk::textureFormatInfo(spall::Format::BC1RGBAUnorm);
	const std::optional<spall::vk::TextureFormatProperties> bc7 = spall::vk::textureFormatInfo(spall::Format::BC7RGBASrgb);

	REQUIRE(bc1.has_value());
	REQUIRE(bc7.has_value());
	CHECK(bc1->bytesPerBlock == 8);
	CHECK(bc1->blockWidth == 4);
	CHECK(bc1->blockHeight == 4);
	CHECK(bc7->bytesPerBlock == 16);
	CHECK(bc7->blockWidth == 4);
	CHECK(bc7->blockHeight == 4);

	CHECK(spall::vk::defaultAspects(spall::Format::BC1RGBAUnorm) == spall::TextureAspectFlags::Color);
	CHECK(spall::vk::defaultAspects(spall::Format::BC7RGBASrgb) == spall::TextureAspectFlags::Color);
	CHECK_FALSE(spall::vk::vertexFormatInfo(spall::Format::BC1RGBAUnorm).has_value());
}

TEST_CASE(
	"Vulkan reports block-compressed formats as sampled-only textures",
	"[vulkan][format][compressed][capabilities]")
{
	VkFormatProperties properties = {};
	properties.bufferFeatures = VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT;
	properties.optimalTilingFeatures =
		VK_FORMAT_FEATURE_TRANSFER_SRC_BIT |
		VK_FORMAT_FEATURE_TRANSFER_DST_BIT |
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT |
		VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT;

	const spall::FormatCapabilities capabilities = spall::vk::formatCapabilities(spall::Format::BC7RGBAUnorm, properties);

	CHECK((capabilities.SupportedTextureUsages & spall::TextureUsageFlags::Sampled) != spall::TextureUsageFlags::None);
	CHECK((capabilities.SupportedTextureUsages & spall::TextureUsageFlags::TransferDestination) != spall::TextureUsageFlags::None);
	CHECK((capabilities.SupportedTextureUsages & spall::TextureUsageFlags::ColorAttachment) == spall::TextureUsageFlags::None);
	CHECK((capabilities.SupportedTextureUsages & spall::TextureUsageFlags::Storage) == spall::TextureUsageFlags::None);
	CHECK_FALSE(capabilities.SupportsVertexInput);
	CHECK(capabilities.SupportsLinearFiltering);
}

TEST_CASE(
	"Vulkan maps every portable format in both directions",
	"[vulkan][format]")
{
	{
		INFO("Format gained enumerators past BC7RGBASrgb; update LastFormatIndex to the new last enumerator");
		REQUIRE_FALSE(spall::vk::toVkFormat(static_cast<spall::Format>(LastFormatIndex + 1)).has_value());
	}

	for (std::size_t index = 1; index <= LastFormatIndex; ++index)
	{
		const spall::Format format = static_cast<spall::Format>(index);
		const std::optional<VkFormat> native = spall::vk::toVkFormat(format);

		INFO("spall::Format enumerator index " << index);
		REQUIRE(native.has_value());
		CHECK(spall::vk::toSpallFormat(*native) == format);
	}
}

TEST_CASE(
	"Vulkan maps storage textures to storage images",
	"[vulkan][binding][storage][texture]")
{
	CHECK(spall::vk::vulkanDescriptorType(spall::ResourceBindingType::StorageTexture) == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	CHECK((spall::vk::vulkanImageUsageFlags(spall::TextureUsageFlags::Storage) & VK_IMAGE_USAGE_STORAGE_BIT) != 0);

	const std::optional<spall::vk::TextureStateInfo> state = spall::vk::vulkanTextureState(spall::ResourceStateFlags::UnorderedAccess);
	REQUIRE(state.has_value());
	CHECK(state->layout == VK_IMAGE_LAYOUT_GENERAL);
	CHECK((state->access & VK_ACCESS_SHADER_WRITE_BIT) != 0);
	CHECK((state->stage & VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT) != 0);
}
