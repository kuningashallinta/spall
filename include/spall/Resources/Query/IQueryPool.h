#pragma once

#include <spall/Common/Resource/IResource.h>

#include <spall/Resources/Query/QueryPoolInfo.h>

namespace spall
{
	class IQueryPool : public IResource
	{
	public:
		virtual QueryPoolInfo info(void) const = 0;
	};
} // namespace spall
