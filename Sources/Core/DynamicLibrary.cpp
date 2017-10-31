/*
 Copyright (c) 2013 yvt

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

#include <Imports/SDL.h>

#ifdef WIN32

#include <windows.h>

#include "Debug.h"
#include "DynamicLibrary.h"
#include "Exception.h"

namespace spades {

	static std::string errToMsg(DWORD err) {
		LPSTR msgBuf = NULL;
		DWORD dwFmt = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
		              FORMAT_MESSAGE_IGNORE_INSERTS;
		size_t size = FormatMessageA(dwFmt, NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		                             (LPSTR)&msgBuf, 0, NULL);
		std::string message(msgBuf, size);
		LocalFree(msgBuf);
		return message;
	}

	DynamicLibrary::DynamicLibrary(const char *fn) {
		SPADES_MARK_FUNCTION();

		name = fn;
		handle = (void *)LoadLibrary(fn);
		if (handle == NULL) {
			DWORD err = GetLastError();
			std::string msg = errToMsg(err);
			SPRaise("Failed to dlload '%s': 0x%08x (%s)", fn, (int)err, msg.c_str());
		}
	}

	DynamicLibrary::~DynamicLibrary() {
		SPADES_MARK_FUNCTION();

		FreeLibrary((HINSTANCE)handle);
	}

	void *DynamicLibrary::GetSymbolOrNull(const char *name) {
		SPADES_MARK_FUNCTION();

		void *addr = (void *)GetProcAddress((HINSTANCE)handle, name);
		return addr;
	}

	void *DynamicLibrary::GetSymbol(const char *sname) {
		SPADES_MARK_FUNCTION();

		void *v = GetSymbolOrNull(sname);
		if (v == NULL) {
			DWORD err = GetLastError();
			std::string msg = errToMsg(err);
			SPRaise("Failed to find symbol '%s' in %s: 0x%08x (%s)", sname, name.c_str(), err,
			        msg.c_str());
		}
		return v;
	}
}

#else
#include "Debug.h"
#include "DynamicLibrary.h"
#include "Exception.h"
#include <dlfcn.h>

namespace spades {
	DynamicLibrary::DynamicLibrary(const char *fn) {
		SPADES_MARK_FUNCTION();

		if (fn == nullptr) {
			SPInvalidArgument("fn");
		}

		name = fn;
		handle = dlopen(fn, RTLD_LAZY);
#ifndef __OpenBSD__
		if (handle == nullptr && strchr(fn, '/') == nullptr && strchr(fn, '\\') == nullptr) {
			char *baseDir = SDL_GetBasePath();
			std::string newPath = baseDir;
			newPath += "/";
			newPath += fn;
			handle = dlopen(newPath.c_str(), RTLD_LAZY);
		}
#endif
		if (handle == nullptr) {
			std::string err = dlerror();
			SPRaise("Failed to dlload '%s': %s", fn, err.c_str());
		}
	}

	DynamicLibrary::~DynamicLibrary() {
		SPADES_MARK_FUNCTION();

		dlclose(handle);
	}

	void *DynamicLibrary::GetSymbolOrNull(const char *name) {
		SPADES_MARK_FUNCTION();

		void *addr = dlsym(handle, name);
		return addr;
	}

	void *DynamicLibrary::GetSymbol(const char *sname) {
		SPADES_MARK_FUNCTION();

		void *v = GetSymbolOrNull(sname);
		if (v == NULL) {
			std::string err = dlerror();
			SPRaise("Failed to find symbol '%s' in %s: %s", sname, name.c_str(), err.c_str());
		}
		return v;
	}
}

#endif
