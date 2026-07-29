#pragma once

#include <cstdio>
#include <cstdlib>

namespace spall
{
	[[noreturn]] inline void reportFailedCheck(
		const char* expression,
		const char* file,
		int line)
	{
		std::fprintf(stderr, "Spall RHI check failed: %s\n\tat %s:%d\n", expression, file, line);
		std::fflush(stderr);
		std::abort();
	}
} // namespace spall

#define SPALL_VERIFY(...)                                                 \
	do                                                                    \
	{                                                                     \
		if (not(__VA_ARGS__))                                             \
		{                                                                 \
			::spall::reportFailedCheck(#__VA_ARGS__, __FILE__, __LINE__); \
		}                                                                 \
	} while (false)

#ifdef NDEBUG
	#define SPALL_ASSERT(...)                    \
		do                                       \
		{                                        \
			(void)sizeof((__VA_ARGS__) ? 0 : 0); \
		} while (false)
#else
	#define SPALL_ASSERT(...) SPALL_VERIFY(__VA_ARGS__)
#endif
