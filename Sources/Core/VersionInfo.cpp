#include "OpenSpadesPlus.h"

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
	return std::string("UNIX/Linux");

#elif defined(TARGET_OS_MAC)
	return std::string("MacOS");

#elif defined(OS_PLATFORM_WINDOWS)
	std::string windowsVersion;
	if (IsWindowsXPOrGreater() && !IsWindowsVistaOrGreater()) {
		windowsVersion = "Windows XP";
	} else if (IsWindowsVistaOrGreater() && !IsWindows7OrGreater()) {
		windowsVersion = "Windows Vista";
	} else if (IsWindows7OrGreater() && !IsWindows8OrGreater()) {
		windowsVersion = "Windows 7";
	} else if (IsWindows8OrGreater() && !IsWindows8Point1OrGreater()) {
		windowsVersion = "Windows 8";
	} else if (IsWindows8Point1OrGreater()) {
		windowsVersion = "Windows 10"; // Nobody uses 8.1... right?
	} else {
		windowsVersion = "Windows 11";
	}
	windowsVersion += " - OpenSpades+ Revision "; // i think this works
	windowsVersion += std::to_string(osPlusVersion);

	// Might be a greater version, but the new Microsoft
	// API doesn't support checking for specific versions.

	if (IsWindowsServer())
		windowsVersion += " Server";
	return windowsVersion;
#elif defined(__FreeBSD__)
	return std::string("FreeBSD");
#elif defined(__OpenBSD__)
	return std::string("OpenBSD");
#elif defined(__NetBSD__)
	return std::string("NetBSD");
#elif defined(__HAIKU__)
	return std::string("Haiku");
#else
	return std::string("MacOS");
#endif
}
