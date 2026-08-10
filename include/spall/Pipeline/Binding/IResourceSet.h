// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/Common/Status/Status.h>
#include <spall/Pipeline/Binding/ResourceWrite.h>

#include <span>

namespace spall
{
	class IResourceSetLayout;

	/// Stores resources written against one resource-set layout.
	class IResourceSet : public IResource
	{
	public:
		virtual IResourceSetLayout& layout(void) const = 0;

		/// Updates resource bindings transactionally; validation failure leaves the current bindings unchanged.
		/// Do not update a set retained by recorded or submitted work; explicit backends reject this.
		virtual Status writeResources(std::span<const ResourceWrite> writes) = 0;
	};
} // namespace spall
