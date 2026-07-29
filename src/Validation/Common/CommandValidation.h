#pragma once

#include <spall/CommandList/IndirectCommands.h>
#include <spall/Common/Enums/ShaderStageFlags.h>
#include <spall/Common/Scissor/Scissor.h>
#include <spall/Common/Status/Status.h>
#include <spall/Common/Viewport/Viewport.h>
#include <spall/Resources/Buffer/IBuffer.h>
#include <spall/Resources/Texture/ITexture.h>
#include <spall/Resources/Texture/TextureRegion.h>
#include <src/Validation/Common/FormatValidation.h>
#include <src/Validation/Common/TextureValidation.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace spall
{
	inline Status validatePushConstantUpdate(
		ShaderStageFlags declaredStages,
		std::uint32_t declaredSize,
		ShaderStageFlags stages,
		std::uint32_t offset,
		std::size_t size);

	inline Status validateViewport(
		const Viewport& viewport);

	inline Status validateScissor(
		const Scissor& scissor);

	inline Status validateCopyBufferArguments(
		const IBuffer& destination,
		std::uint32_t destinationOffset,
		const IBuffer& source,
		std::uint32_t sourceOffset,
		std::uint32_t byteSize);

	inline Status validateIndirectArguments(
		const IBuffer& argumentBuffer,
		std::uint32_t offset,
		std::uint32_t argumentSize);

	inline TextureRegion resolveTextureRegion(
		const TextureInfo& info,
		const TextureRegion& region);

	inline Status validateTextureBufferCopyArguments(
		const ITexture& texture,
		const TextureRegion& region,
		const IBuffer& buffer,
		std::uint32_t bufferOffset,
		std::uint32_t bufferRowPitch);

	inline Status validateCopyTextureArguments(
		const ITexture& destination,
		const ITexture& source);

	inline Status validateGenerateMipsArguments(
		const ITexture& texture);
} // namespace spall

#include <src/Validation/Common/CommandValidation.inl>
