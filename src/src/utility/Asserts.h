#pragma once

// Based on Hazel assert system
// https://github.com/TheCherno/Hazel

#include <utility/OperatingSystem.h>
#include <filesystem>
#define GEE_DEBUG

#ifdef GEE_DEBUG
	#if defined(GEE_OS_WINDOWS)
		#define GEE_DEBUGBREAK() __debugbreak()
	#elif defined(GEE_OS_LINUX)
		#include <signal.h>
		#define GEE_DEBUGBREAK() raise(SIGTRAP)
	#else
		#error "Debugbreak does not work on this operating system yet."
	#endif

	#define GEE_ENABLE_ASSERTS
#else
	#define GEE_DEBUGBREAK()
#endif

#define GEE_EXPAND_MACRO(x) x
#define GEE_STRINGIFY_MACRO(x) #x



#ifdef GEE_ENABLE_ASSERTS

	#define GEE_INTERNAL_ASSERT_IMPL(type, check, msg, ...) { if(!(check)) { /*GEE##type##ERROR(msg, ##__VA_ARGS__);*/ GEE_DEBUGBREAK(); } }
	#define GEE_INTERNAL_ASSERT_WITH_MSG(type, check, ...) GEE_INTERNAL_ASSERT_IMPL(type, check, "Assertion failed: {0}", ##__VA_ARGS__)
	#define GEE_INTERNAL_ASSERT_NO_MSG(type, check) GEE_INTERNAL_ASSERT_IMPL(type, check, "Assertion '{0}' failed at {1}:{2}", GEE_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

	#define GEE_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
	#define GEE_INTERNAL_ASSERT_GET_MACRO(...) GEE_EXPAND_MACRO( GEE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, GEE_INTERNAL_ASSERT_WITH_MSG, GEE_INTERNAL_ASSERT_NO_MSG) )

	#define GEE_ASSERT(...) GEE_EXPAND_MACRO(GEE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, ##__VA_ARGS__))
	#define GEE_CORE_ASSERT(...) GEE_EXPAND_MACRO(GEE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_CORE_, ##__VA_ARGS__))
#else
	#define GEE_ASSERT(...)
	#define GEE_CORE_ASSERT(...)
#endif