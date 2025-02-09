#pragma once

#include "../log/log.h"

#ifdef _DEBUG
	#ifdef ENGINE_PLATFORM_WINDOWS
		#include <intrin.h>
		#define ENGINE_BREAK __debugbreak();
	#elif ENGINE_PLATFORM_LINUX
		#include <signal.h>
		#define ENGINE_BREAK raise(SIGTRAP);
    #else
        #define ENGINE_BREAK
    #endif

    #define ENGINE_BREAK_CONDITION(x) if (!(x)) { LOG_E("%s", #x); ENGINE_BREAK; }
    #define ENGINE_BREAK_CONDITION2(x, text) if (!(x)) { LOG_E("%s", text); ENGINE_BREAK; }
    #define ENGINE_BREAK_CONDITION_DO(x, todo) if (!(x)) {    \
        LOG_E("!(%s)", #x);                                               \
        ENGINE_BREAK;                                                       \
        todo;                                                               \
    }
#else
	#define ENGINE_BREAK
	#define ENGINE_BREAK_CONDITION(x)
    #define ENGINE_BREAK_CONDITION2(x, text)
    #define ENGINE_BREAK_CONDITION_DO(x, todo) if (!(x)) {    \
        LOG_E("!(%s)", #x);                                               \
        todo;                                                               \
    }
#endif