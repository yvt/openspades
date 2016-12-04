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

#include <string>

#include <Imports/SDL.h>

#ifdef WIN32
#include <wchar.h>
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "FltkPreferenceImporter.h"
#include "SdlFileStream.h"
#include <Core/Debug.h>
#include <Core/Strings.h>

namespace spades {

#ifdef WIN32
	static std::string SdlReceiveString(char *ptr) {
		if (!ptr) {
			return "";
		}
		std::string s(ptr);
		SDL_free(ptr);
		return s;
	}
	static std::wstring Utf8ToWString(const char *s) {
		auto *ws =
		  (WCHAR *)SDL_iconv_string("UCS-2-INTERNAL", "UTF-8", (char *)(s), SDL_strlen(s) + 1);
		if (!ws)
			return L"";
		std::wstring wss(ws);
		SDL_free(ws);
		return wss;
	}
#endif

	static std::string GetFltkPreferencePath() {
#if defined(WIN32)
		std::string path = SdlReceiveString(SDL_GetPrefPath("yvt.jp", "OpenSpades.prefs"));
		if (path.back() == '\\')
			path.resize(path.size() - 1);
		return path;
#elif defined(__APPLE__)
		const char *homeptr = getenv("HOME");
		std::string home = homeptr ? homeptr : "";
		home += "/Library/Preferences/yvt.jp/OpenSpades.prefs";
		return home;
#else
		const char *homeptr = getenv("HOME");
		std::string home = homeptr ? homeptr : "";
		home += "/.fltk/yvt.jp/OpenSpades.prefs";
		return home;
#endif
	}

	std::vector<std::pair<std::string, std::string>> ImportFltkPreference() {
		std::vector<std::pair<std::string, std::string>> ret;

		auto path = GetFltkPreferencePath();
		SPLog("Checking for legacy preference file: %s", path.c_str());

		auto *rw = SDL_RWFromFile(path.c_str(), "rb");
		if (rw == nullptr) {
			SPLog("Legacy preference file wasn't found.");
			return ret;
		}

		SdlFileStream stream(rw, true);
		std::string text = stream.ReadAllBytes();

		SPLog("Parsing %d bytes of legacy preference file", static_cast<int>(text.size()));

		auto lines = SplitIntoLines(text);
		std::string buf;
		auto flush = [&] {
			if (buf.empty())
				return;

			auto idx = buf.find(':');
			if (idx != std::string::npos) {
				auto name = buf.substr(0, idx);
				auto value = buf.substr(idx + 1);

				if (name.size() > 0)
					ret.emplace_back(name, value);
			}

			buf.clear();
		};

		for (auto &line : lines) {
			{
				auto idx = line.find(';');
				if (idx != std::string::npos) {
					line.resize(idx);
				}
			}
			if (line.size() == 0)
				continue;
			if (line[0] == '[')
				continue; // group is not used
			if (line[0] == '+') {
				// continuation
				buf.append(line, 1, line.size() - 1);
			} else {
				flush();
				buf = line;
			}
		}
		flush();

		return ret;
	}

	void DeleteFltkPreference() {
		auto path = GetFltkPreferencePath();
		/*
#ifdef WIN32
		DeleteFileW(Utf8ToWString(path.c_str()).c_str());
#else
		unlink(path.c_str());
#endif
		*/

		// afraid of removing the pref file completely
		SPLog("Moving %s to %s", path.c_str(), (path + "-old").c_str());
#ifdef WIN32
		auto s = Utf8ToWString(path.c_str());
		MoveFileW(s.c_str(), (s + L"-old").c_str());
#else
		rename(path.c_str(), (path + "-old").c_str());
#endif
	}
}
