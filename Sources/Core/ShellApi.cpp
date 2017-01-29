/*
 Copyright (c) 2016 yvt

 This file is part of OpenSpades.

 OpenSpades is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 OpenSpades is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with OpenSpades.  If not, see <http://www.gnu.org/licenses/>.

 */

#include <cerrno>     // errno
#include <cstdlib>    // system
#include <cstring>    // strerror

#include "ShellApi.h"

#include <Core/Debug.h>
#include <Core/Strings.h>

#ifdef WIN32
#include <Imports/SDL.h>
#include <windows.h>

namespace {
	static std::wstring WStringFromUtf8(const char *s) {
		auto *ws = (wchar_t *)SDL_iconv_string("UCS-2-INTERNAL", "UTF-8", s, strlen(s) + 1);
		if (!ws)
			return L"";
		std::wstring wss(ws);
		SDL_free(ws);
		return wss;
	}
}
#else
#include <sys/wait.h> // WIFSIGNALED, ...
#endif

namespace spades {
#if defined(__APPLE__)
// macOS version of ShowDirectoryInShell is defined in ShellApi.mm.
#elif defined(WIN32)
	bool ShowDirectoryInShell(const std::string &directoryPath) {
		std::wstring widePath = WStringFromUtf8(directoryPath.c_str());

		// "The return value is cast as an HINSTANCE for backward compatibility with 16-bit Windows
		// applications. It is not a true HINSTANCE, however. It can be cast only to an int and
		// compared to either 32 or the following error codes below." --- MSDN Library
		//
		// Cast it to INT_PTR instead of int to get rid of "pointer truncation" warning.
		INT_PTR result = reinterpret_cast<INT_PTR>(
		  ShellExecuteW(nullptr, L"explore", widePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL));

		// "If the function succeeds, it returns a value greater than 32."
		if (result > 32) {
			return true;
		} else {
			SPLog("ShellExecuteW returned %u while trying to open '%s' with Explorer.",
			      static_cast<unsigned int>(result), directoryPath.c_str());
			return false;
		}
	}
	bool OpenURLInBrowser(const std::string &url) {
		std::wstring widePath = WStringFromUtf8(url.c_str());

		// "The return value is cast as an HINSTANCE for backward compatibility with 16-bit Windows
		// applications. It is not a true HINSTANCE, however. It can be cast only to an int and
		// compared to either 32 or the following error codes below." --- MSDN Library
		//
		// Cast it to INT_PTR instead of int to get rid of "pointer truncation" warning.
		INT_PTR result = reinterpret_cast<INT_PTR>(
		  ShellExecuteW(nullptr, L"open", widePath.c_str(), nullptr, nullptr, SW_SHOWNORMAL));

		// "If the function succeeds, it returns a value greater than 32."
		if (result > 32) {
			return true;
		} else {
			SPLog("ShellExecuteW returned %u while trying to open '%s' with a browser.",
			      static_cast<unsigned int>(result), url.c_str());
			return false;
		}
	}
#elif __unix || __unix__
	bool ShowDirectoryInShell(const std::string &directoryPath) {
		// FIXME: escape single quotes
		if (directoryPath.find("'") != std::string::npos) {
			SPLog("Cannot launch xdg-open (currently): the directory path '%s' contains a single "
			      "quotation mark",
			      directoryPath.c_str());
			return false;
		}

		// Assumption: system() uses the Bourne or any compatible shell internally.
		//             system() behaves like that of GNU C Library.
		int result = std::system(Format("xdg-open '{0}'", directoryPath).c_str());

		if (result == -1) {
			int no = errno;
			SPLog("system() failed while executing xdg-open '%s': %s", directoryPath.c_str(),
			      std::strerror(no));
			return false;
		}

		int status = WEXITSTATUS(result);
		if (status) {
			SPLog("xdg-open failed with the exit code %d.", status);
			return false;
		}

		return true;
	}
	
	bool OpenURLInBrowser(const std::string &url) {
		// FIXME: escape single quotes
		if (url.find("'") != std::string::npos) {
			SPLog("Cannot launch xdg-open (currently): the URL '%s' contains a single quotation mark",
			      url.c_str());
			return false;
		}

		// Assumption: system() uses the Bourne or any compatible shell internally.
		//             system() behaves like that of GNU C Library.
		int result = std::system(Format("xdg-open '{0}'", url).c_str());

		if (result == -1) {
			int no = errno;
			SPLog("system() failed while executing xdg-open '%s': %s", url.c_str(),
			      std::strerror(no));
			return false;
		}

		int status = WEXITSTATUS(result);
		if (status) {
			SPLog("xdg-open failed with the exit code %d.", status);
			return false;
		}

		return true;
	}
#endif
}
