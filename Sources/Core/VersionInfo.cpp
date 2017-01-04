#if __linux__
	#define OS_PLATFORM_LINUX
#elif TARGET_OS_MAC
	#define OS_PLATFORM_MAC
#elif defined _WIN32 || defined _WIN64
	#define OS_PLATFORM_WINDOWS
	#include <Windows.h>
	#include <sstream>
	#include <VersionHelpers.h> // Requires windows 8.1 sdk at least
#endif

#include "VersionInfo.h"

std::string VersionInfo::GetVersionInfo() {
#if defined(OS_PLATFORM_LINUX)
	return std::string("Linux");
#elif defined(TARGET_OS_MAC)
	return std::string("Mac OS X");
#elif defined(OS_PLATFORM_WINDOWS)
		// TODO: IsWindowsServer()

		if((IsWindowsXPOrGreater()) && !(IsWindowsVistaOrGreater())) {
			return std::string("Windows XP");
		}
		
		if((IsWindowsVistaOrGreater()) && !(IsWindows7OrGreater())) {
			return std::string("Windows Vista");
		}
		
		if((IsWindows7OrGreater()) && !(IsWindows8OrGreater())) {
			return std::string("Windows 7");
		}
		
		if((IsWindows8OrGreater()) && !(IsWindows8Point1OrGreater())) {
			return std::string("Windows 8");
		}
		
		if((IsWindows8Point1OrGreater()) && !(IsWindows10OrGreater())) {
			return std::string("Windows 8.1");
		}
		
		if((IsWindows10OrGreater())) {
			return std::string("Windows 10");
			
			//   Might be a greater version, but the new Microsoft
			// API doesn't support checking for specific versions.            
		}
	}
	
	return std::string("Windows ??");
#else
	return std::string("Unknown OS");
#endif
}
