// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

namespace spall
{
	template <typename T, std::size_t Extent>
		requires std::is_trivially_copyable_v<T> and (not std::is_same_v<std::remove_cv_t<T>, std::byte>)
	inline Status ICommandList::setPushConstants(
		ShaderStageFlags stages,
		std::uint32_t offset,
		std::span<const T, Extent> data)
	{
		return setPushConstants(stages, offset, std::as_bytes(data));
	}

	template <typename T>
		requires std::is_trivially_copyable_v<T>
	inline Status ICommandList::setPushConstants(
		ShaderStageFlags stages,
		std::uint32_t offset,
		const T& data)
	{
		return setPushConstants(stages, offset, std::as_bytes(std::span {&data, 1}));
	}
} // namespace spall
