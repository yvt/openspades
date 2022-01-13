// todo: rewrite this honestly shitty as fuck code
// Don't Repeat Yourself

#include <Client/OpenSpadesPlus.h>

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

std::string buffer;

#if defined(OS_PLATFORM_LINUX)
	buffer = "UNIX/Linux";
	buffer += " | OpenSpades+ Revision "; // i think this works
	buffer += std::to_string(spades::client::plusVersion);
	
	return std::string(buffer);

#elif defined(TARGET_OS_MAC)
	buffer = "Mac OS";
	buffer += " | OpenSpades+ Revision "; // i think this works
	buffer += std::to_string(spades::client::plusVersion);
	
	return std::string(buffer);

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
	windowsVersion += " | OpenSpades+ Revision "; // i think this works
	windowsVersion += std::to_string(spades::client::plusVersion);

	// Might be a greater version, but the new Microsoft
	// API doesn't support checking for specific versions.

	if (IsWindowsServer())
		windowsVersion += " Server";
	return windowsVersion;
#elif defined(__FreeBSD__)
	buffer = "FreeBSD";
	buffer += " | OpenSpades+ Revision "; // i think this works
	buffer += std::to_string(spades::client::plusVersion);
	
	return std::string(buffer);
#elif defined(__OpenBSD__)
	buffer = "OpenBSD";
	buffer += " | OpenSpades+ Revision "; // i think this works
	buffer += std::to_string(spades::client::plusVersion);
	
	return std::string(buffer);
#elif defined(__NetBSD__)
	buffer = "NetBSD";
	buffer += " | OpenSpades+ Revision "; // i think this works
	buffer += std::to_string(spades::client::plusVersion);
	
	return std::string(buffer);
#elif defined(__HAIKU__)
	buffer = "Haiku OS";
	buffer += " | OpenSpades+ Revision "; // i think this works
	buffer += std::to_string(spades::client::plusVersion);
	
	return std::string(buffer);
#else
	buffer = "Mac OS";
	buffer += " | OpenSpades+ Revision "; // i think this works
	buffer += std::to_string(spades::client::plusVersion);
	
	return std::string(buffer); // I honestly dont know anything else that would fall into Unknown. I do, however, know that on modern macs or something TARGET_OS_MAC supposedly fails or something??? Update libs or something.
#endif
}
