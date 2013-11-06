#include "VersionInfo.h"

std::string VersionInfo::GetVersionInfo()
{
	#if defined __linux__
	return std::string("Linux");
	#elif TARGET_OS_MAC
	return std::string("Mac OS X");
	#elif defined _WIN32 || defined _WIN64
	return std::string("Windows");
	#else
	return std::string("Unknown OS");
	#endif
}