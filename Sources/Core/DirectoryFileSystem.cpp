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

#include <sys/stat.h>

#ifdef WIN32
#include <io.h>
#include <windows.h>
#ifdef _MSC_VER
#include <direct.h>
#define mkdir _mkdir
#endif
#else
#include <dirent.h>
#endif

#include "DirectoryFileSystem.h"

#include "Debug.h"
#include "Exception.h"
#include "SdlFileStream.h"

namespace spades {

	DirectoryFileSystem::DirectoryFileSystem(const std::string &r, bool canWrite)
	    : rootPath(r), canWrite(canWrite) {
		SPADES_MARK_FUNCTION();
		SPLog("Directory File System Initialized: %s (%s)", r.c_str(),
		      canWrite ? "Read/Write" : "Read-only");
	}

	DirectoryFileSystem::~DirectoryFileSystem() { SPADES_MARK_FUNCTION(); }

	std::string DirectoryFileSystem::physicalPath(const std::string &lg) {
		// TODO: check ".."?
		return rootPath + '/' + lg;
	}

#ifdef WIN32
	static std::wstring Utf8ToWString(const char *s) {
		auto *ws =
		  (WCHAR *)SDL_iconv_string("UCS-2-INTERNAL", "UTF-8", (char *)(s), SDL_strlen(s) + 1);
		if (!ws)
			return L"";
		std::wstring wss(ws);
		SDL_free(ws);
		return wss;
	}
	static std::string Utf8FromWString(const wchar_t *ws) {
		auto *s =
		  (char *)SDL_iconv_string("UTF-8", "UCS-2-INTERNAL", (char *)(ws), wcslen(ws) * 2 + 2);
		if (!s)
			return "";
		std::string ss(s);
		SDL_free(s);
		return ss;
	}
#endif

	std::vector<std::string> DirectoryFileSystem::EnumFiles(const char *p) {
		SPADES_MARK_FUNCTION();
#ifdef WIN32
		WIN32_FIND_DATAW fd;
		HANDLE h;
		std::vector<std::string> ret;
		std::wstring filePath;

		std::wstring path = Utf8ToWString(physicalPath(p).c_str());
		// open the Win32 find handle.
		h = FindFirstFileExW((path + L"\\*").c_str(), FindExInfoStandard, &fd,
		                     FindExSearchNameMatch, NULL, 0);

		if (h == INVALID_HANDLE_VALUE) {
			// couldn't open. return the empty vector.
			return ret;
		}

		do {
			// is it a directory?
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				// "." and ".." mustn't be included.
				if (wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L"..")) {
					filePath = fd.cFileName;
					ret.push_back(Utf8FromWString(filePath.c_str()));
				}
			} else {
				// usual file.
				filePath = fd.cFileName;
				ret.push_back(Utf8FromWString(filePath.c_str()));
			}

			// iterate!
		} while (FindNextFileW(h, &fd));

		// close the handle.
		FindClose(h);

		return ret;
#else
		// open the directory.

		std::string path = physicalPath(p);
		DIR *dir = opendir(path.c_str());
		struct dirent *ent;

		std::vector<std::string> ret;
		std::string filePath;

		// if couldn't open the directory, return the empty vector.
		if (!dir)
			return ret;

		// read an entry.
		while ((ent = readdir(dir))) {
			if (ent->d_name[0] == '.')
				continue;

			// make it full-path.
			filePath = ent->d_name;

			// add to the result vector.
			ret.push_back(filePath);
		}

		// close the directory.
		closedir(dir);

		return ret;
#endif
	}

	IStream *DirectoryFileSystem::OpenForReading(const char *fn) {
		SPADES_MARK_FUNCTION();

		std::string path = physicalPath(fn);
		SDL_RWops *f = SDL_RWFromFile(path.c_str(), "rb");
		if (f == NULL) {
			SPRaise("I/O error while opening %s for reading: %s", fn, SDL_GetError());
		}
		return new SdlFileStream(f, true);
	}

	IStream *DirectoryFileSystem::OpenForWriting(const char *fn) {
		SPADES_MARK_FUNCTION();
		if (!canWrite) {
			SPRaise("Writing prohibited for root path '%s'", rootPath.c_str());
		}

		std::string path = physicalPath(fn);

		// create required directory
		if (path.find_first_of("/\\") != std::string::npos) {
			size_t pos = path.find_first_of("/\\") + 1;
			while (pos < path.size()) {
				size_t nextPos = pos;
				while (nextPos < path.size() && path[nextPos] != '/' && path[nextPos] != '\\')
					nextPos++;
				if (nextPos == path.size())
					break;
#ifdef WIN32
				CreateDirectoryW(Utf8ToWString(path.substr(0, nextPos).c_str()).c_str(), nullptr);
#else
				mkdir(path.substr(0, nextPos).c_str(), 0774);
#endif
				pos = nextPos + 1;
			}
		}

		SDL_RWops *f = SDL_RWFromFile(path.c_str(), "wb+");
		if (f == NULL) {
			SPRaise("I/O error while opening %s for writing", fn);
		}
		return new SdlFileStream(f, true);
	}

	// TODO: open for appending?

	bool DirectoryFileSystem::FileExists(const char *fn) {
		SPADES_MARK_FUNCTION();
		std::string path = physicalPath(fn);
		SDL_RWops *f = SDL_RWFromFile(path.c_str(), "rb");
		if (f) {
			SDL_RWclose(f);
			return true;
		}
		return false;
	}
}
