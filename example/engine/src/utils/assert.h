#pragma once

#include "../log/log.h"

#include <cassert>
#include <cstdlib>

#ifdef _DEBUG
	#if defined(ENGINE_PLATFORM_WINDOWS)
		#include <intrin.h>
		#define ENGINE_BREAK() __debugbreak()
	#elif defined(ENGINE_PLATFORM_LINUX)
		#include <signal.h>
		#define ENGINE_BREAK() raise(SIGTRAP)
	#else
		#define ENGINE_BREAK() 
	#endif

	#define ENGINE_ASSERT(x) assert(x)
	#define ENGINE_ASSERT2(x, text) assert((x) && (text))

	#define ENGINE_BREAK_IF(x) do {                         \
		if (x) { LOG_E("%s", #x); ENGINE_BREAK(); }         \
	} while (0)

	#define ENGINE_BREAK_IF_NOT(x) do {                     \
		if (!(x)) { LOG_E("!(%s)", #x); ENGINE_BREAK(); }   \
	} while (0)

	#define ENGINE_BREAK_IF_NOT2(x, text) do {              \
		if (!(x)) { LOG_E("%s", text); ENGINE_BREAK(); }    \
	} while (0)

	#define ENGINE_BREAK_IF_NOT_AND_DO(x, todo) do {        \
		if (!(x)) {                                         \
			LOG_E("!(%s)", #x);                             \
			ENGINE_BREAK();                                 \
			todo;                                           \
		}                                                   \
	} while (0)

	#define ENGINE_BREAK_IF_NOT_AND_DO2(x, text, todo) do { \
		if (!(x)) {                                         \
			LOG_E("%s", text);                              \
			ENGINE_BREAK();                                 \
			todo;                                           \
		}                                                   \
	} while (0)

#else
	#define ENGINE_BREAK() 

	#define ENGINE_ASSERT(x) 
	#define ENGINE_ASSERT2(x, text) 

	#define ENGINE_BREAK_IF(x) 
	#define ENGINE_BREAK_IF_NOT(x) 
	#define ENGINE_BREAK_IF_NOT2(x, text) 

	#define ENGINE_BREAK_IF_NOT_AND_DO(x, todo) do {        \
		if (!(x)) {                                         \
			LOG_E("!(%s)", #x);                             \
			todo;                                           \
		}                                                   \
	} while (0)

	#define ENGINE_BREAK_IF_NOT_AND_DO2(x, text, todo) do { \
		if (!(x)) {                                         \
			LOG_E("%s", text);                              \
			todo;                                           \
		}                                                   \
	} while (0)

#endif

#define ENGINE_CHECK(x) ENGINE_BREAK_IF_NOT_AND_DO2((x), "ENGINE_CHECK: " #x, std::abort())
#define ENGINE_CHECK2(x, text) ENGINE_BREAK_IF_NOT_AND_DO2((x), "ENGINE_CHECK: " #x " : " text, std::abort())