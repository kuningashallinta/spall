#pragma once

#include <cstdint>

namespace spall
{
	using Status = std::uint32_t;

	inline constexpr Status SUCCESS = 0x0000;

	inline constexpr Status ERR_OUT_OF_MEMORY = 0x1000;
	inline constexpr Status ERR_NOT_READY = 0x1001;
	inline constexpr Status ERR_BACKEND_FAILURE = 0x1002;
	inline constexpr Status ERR_DEVICE_LOST = 0x1003;
	inline constexpr Status ERR_INVALID_ARGUMENT = 0x1004;
	inline constexpr Status ERR_INVALID_BINDING = 0x1005;
	inline constexpr Status ERR_INVALID_FORMAT = 0x1006;
	inline constexpr Status ERR_INVALID_RANGE = 0x1007;
	inline constexpr Status ERR_INVALID_RESOURCE = 0x1008;
	inline constexpr Status ERR_INVALID_RESOURCE_STATE = 0x1009;
	inline constexpr Status ERR_INVALID_RESOURCE_TYPE = 0x100A;
	inline constexpr Status ERR_INVALID_SHADER_BYTECODE = 0x100B;
	inline constexpr Status ERR_INVALID_SHADER_STAGE = 0x100C;
	inline constexpr Status ERR_INVALID_SIZE = 0x100D;
	inline constexpr Status ERR_INVALID_STATE = 0x100E;
	inline constexpr Status ERR_INVALID_USAGE_FLAGS = 0x100F;
	inline constexpr Status ERR_INVALID_WINDOW = 0x1010;
	inline constexpr Status ERR_UNSUPPORTED = 0x1011;
	inline constexpr Status ERR_UNSUPPORTED_BACKEND = 0x1012;
	inline constexpr Status ERR_UNSUPPORTED_FORMAT = 0x1013;
	inline constexpr Status ERR_UNSUPPORTED_MEMORY_TYPE = 0x1014;
	inline constexpr Status ERR_UNSUPPORTED_SHADER_STAGE = 0x1015;
	inline constexpr Status ERR_UNSUPPORTED_USAGE = 0x1016;
	inline constexpr Status ERR_RENDER_PASS_CREATION_FAILED = 0x1017;
	inline constexpr Status ERR_SWAP_CHAIN_ACQUIRE_FAILED = 0x1018;
	inline constexpr Status ERR_SWAP_CHAIN_CREATION_FAILED = 0x1019;
	inline constexpr Status ERR_SWAP_CHAIN_OUT_OF_DATE = 0x101A;
	inline constexpr Status ERR_SWAP_CHAIN_PRESENT_FAILED = 0x101B;
} // namespace spall
