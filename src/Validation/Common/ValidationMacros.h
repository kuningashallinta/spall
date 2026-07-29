#pragma once

#include <spall/Common/Status/Status.h>

#define SPALL_TRY(expression)                         \
	do                                                \
	{                                                 \
		::spall::Status spallTryError = (expression); \
		if (spallTryError != ::spall::SUCCESS)        \
		{                                             \
			return spallTryError;                     \
		}                                             \
	} while (false)
