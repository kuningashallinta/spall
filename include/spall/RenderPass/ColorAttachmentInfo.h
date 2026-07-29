#pragma once

#include <spall/Common/Color/Color.h>
#include <spall/Common/Enums/ResourceEnums.h>

namespace spall
{
	/// Describes color attachment operations for a render pass.
	struct ColorAttachmentInfo
	{
		LoadAction LoadAction = LoadAction::Load;
		StoreAction StoreAction = StoreAction::Store;

		/// Used only when LoadAction is Clear.
		Color ClearColor = {};
	};
} // namespace spall
