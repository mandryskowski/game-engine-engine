#pragma once
#define GEE_GL_VERSION_MAJOR 4
#define GEE_GL_VERSION_MINOR 0
// Based on Hazel platform detection
// https://github.com/TheCherno/Hazel

#if defined(_WIN32) || defined(_WIN64)
	#define GEE_OS_WINDOWS
#elif defined(__APPLE__) || defined(__MACH__)
	#include <TargetConditionals.h>
	#if TARGET_IPHONE_SIMULATOR == 1
		#error "IOS simulator is not supported!"
	#elif TARGET_OS_IPHONE == 1
		#define GEE_OS_IOS
		#error "IOS is not supported!"
	#elif TARGET_OS_MAC == 1
		#define GEE_OS_MACOS
		#error "MacOS is not supported!"
	#else
		#error "Unknown Apple platform!"
	#endif
	 /* We also have to check __ANDROID__ before __linux__
	  * since android is based on the linux kernel
	  * it has __linux__ defined */
#elif defined(__ANDROID__)
	#define GEE_OS_ANDROID
	#error "Android is not supported!"
#elif defined(__linux__)
	#define GEE_OS_LINUX
#else
	#error "Unknown operating system!"
#endif