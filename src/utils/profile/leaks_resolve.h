#pragma once

#ifdef CHECK_MEMORY_LEAKS

#include <cstdint>

extern thread_local const char* __file__;
extern thread_local uint32_t __line__;

#ifndef check_leaks_new
//#define check_leaks_new (__file__=__FILE__, __line__=__LINE__) && 0 ? NULL : new
//#define new check_leaks_new // - can use this on target cpp file
#define check_leaks_new (__file__=__FILE__) && (__line__=__LINE__) && 0 ? nullptr : new
#define new check_leaks_new
#endif

#endif

