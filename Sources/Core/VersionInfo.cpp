#if __linux__
#define OS_PLATFORM_LINUX
#elif TARGET_OS_MAC
#define OS_PLATFORM_MAC
#elif defined _WIN32 || defined _WIN64
#define OS_PLATFORM_WINDOWS
#include <Windows.h>
#include <sstream>
#endif

#include "VersionInfo.h"

std::string VersionInfo::GetVersionInfo() {
#if defined(OS_PLATFORM_LINUX)
	return std::string("Linux");
#elif defined(TARGET_OS_MAC)
	return std::string("Mac OS X");
#elif defined(OS_PLATFORM_WINDOWS)
	OSVERSIONINFO osv = {sizeof(osv), 0};
	if (GetVersionEx(&osv)) {
		if (5 == osv.dwMajorVersion) {
			switch (osv.dwMinorVersion) {
				case 0: return "Windows 2000";
				case 1: return "Windows XP";
				case 2: return "Windows XPx64";
				default: break;
			}
		} else if (6 == osv.dwMajorVersion) {
			switch (osv.dwMinorVersion) {
				case 0: return "Windows Vista";
				case 1: return "Windows 7";
				case 2: return "Windows 8";
				case 3: return "Windows 8.1";
				default: break;
			}
		}
		std::stringstream ss;
		ss << "Windows " << osv.dwMajorVersion << "." << osv.dwMinorVersion;
		return ss.str();
	}
	return "Windows ??";
#else
	return std::string("Unknown OS");
#endif
}
