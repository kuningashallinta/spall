// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Assert.h>

#include <spall/Common/Resource/IResource.h>

#include <atomic>
#include <cstdint>

namespace spall
{
	template <typename T>
	class SharedObject : public T
	{
	public:
		~SharedObject(void) override = default;

		std::uint32_t addRef(void) override;
		std::uint32_t release(void) override;
		std::uint32_t referenceCount(void) const;

	private:
		std::atomic<std::uint32_t> m_RefCount {0};
	};
} // namespace spall

#include <spall/Common/Resource/SharedObject.inl>
