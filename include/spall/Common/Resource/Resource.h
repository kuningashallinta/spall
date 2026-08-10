// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Assert.h>

#include <spall/Common/Resource/IResource.h>

namespace spall
{
	/// Strong intrusive reference to an IResource implementation.
	/// Reference-count changes are thread-safe; access to the wrapper and resource still requires synchronization.
	template <typename T>
	class Resource
	{
	public:
		explicit Resource(T* resource);

		Resource(void) = default;
		Resource(const Resource& other);
		Resource(Resource&& other) noexcept;

		~Resource(void);

		Resource& operator=(const Resource& other);
		Resource& operator=(Resource&& other) noexcept;

		explicit operator bool(void) const;

		T& operator*(void) const;
		T* operator->(void) const;

		/// Gets the referenced object without changing its reference count.
		T* get(void) const;

		/// Replaces the referenced object.
		void reset(T* resource);
		void reset(void);

	private:
		T* m_Resource = nullptr;
	};
} // namespace spall

#include <spall/Common/Resource/Resource.inl>
