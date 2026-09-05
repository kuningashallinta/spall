// SPDX-FileCopyrightText: 2026 King Hallinta
// SPDX-License-Identifier: Apache-2.0

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
