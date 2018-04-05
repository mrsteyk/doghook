#pragma once

// For determining the platform that code is being compiled on
// These should be used anywhere where conditional compilation depends
// on the platform.

// clang-format off
#define doghook_platform_clang() false
#define doghook_platform_msvc() false
#define doghook_platform_gcc() false

#if defined(__clang__)
	#undef doghook_platform_clang
	#define doghook_platform_clang() true
#elif defined(_MSC_VER)
	#undef doghook_platform_msvc
	#define doghook_platform_msvc() true
#elif defined(__GNUC__)
	#undef doghook_platform_gcc
	#define doghook_platform_gcc() true
#else
    #error Unknown compiler: fix me!
#endif

#define doghook_platform_windows() false
#define doghook_platform_linux() false
#define doghook_platform_osx() false

#if defined(_WIN32)
	#undef doghook_platform_windows
	#define doghook_platform_windows() true
#elif defined(__unix__) && !defined(__APPLE__)
	#undef doghook_platform_linux
	#define doghook_platform_linux() true
#elif defined(__APPLE__)
	#undef doghook_platform_osx
	#define doghook_platform_osx() true
#else
    #error Unknown platform: fix me!
#endif

#define doghook_platform_debug() false

#if defined(_DEBUG) || defined(DEBUG) && !defined(NDEBUG)
    #undef doghook_platform_debug
    #define doghook_platform_debug() true
#endif
// clang-format on

namespace doghook_platform {
inline constexpr bool windows() { return doghook_platform_windows(); }
inline constexpr bool linux() { return doghook_platform_linux(); }
inline constexpr bool osx() { return doghook_platform_osx(); }

inline constexpr bool msvc() { return doghook_platform_msvc(); }
inline constexpr bool clang() { return doghook_platform_clang(); }
inline constexpr bool gcc() { return doghook_platform_gcc(); }
} // namespace doghook_platform

#include "types.hh"

#if doghook_platform_windows()
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

#if !doghook_platform_windows()
// Only windows has these
#define __fastcall
#define __stdcall
#define __cdecl
#define __thiscall

// TODO: make a function wrapper please
#define _stricmp strcasecmp

#include <signal.h>

#endif
