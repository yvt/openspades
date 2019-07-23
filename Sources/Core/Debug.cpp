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

#include <cstdarg>
#include <ctime>
#include <deque>
#include <map>
#include <string>

#include "Debug.h"
#include "FileManager.h"
#include "IStream.h"
#include "Math.h"
#include "Strings.h"
#include <Core/Debug.h>
#include <Imports/SDL.h>

#define SPADES_USE_TLS 1

#if SPADES_USE_TLS
#include "ThreadLocalStorage.h"
#endif // SPADES_USE_TLS

namespace spades {
	namespace reflection {
		BacktraceEntryAdder::BacktraceEntryAdder(const BacktraceEntry &entry) {
			bt = Backtrace::GetGlobalBacktrace();
			if (bt)
				bt->Push(entry);
		}

		BacktraceEntryAdder::~BacktraceEntryAdder() {
			if (bt)
				bt->Pop();
		}

#if SPADES_USE_TLS

		static AutoDeletedThreadLocalStorage<Backtrace> backtraceTls("backtraceTls");

		// TLS is initialized as global constructor and
		// some constructors are called before initialization of TLS.
		// this prevents backtrace to be used until all constructors are
		// called.
		static bool backtraceStarted = false;

		Backtrace *Backtrace::GetGlobalBacktrace() {
			if (!backtraceStarted)
				return NULL;
			Backtrace *b = backtraceTls;
			if (!b) {
				b = new Backtrace;
				backtraceTls = b;
			}
			return b;
		}

		void Backtrace::ThreadExiting() {}

		void Backtrace::StartBacktrace() { backtraceStarted = true; }

#else

		static std::map<Uint32, Backtrace *> globalBacktrace;
		static Uint32 firstThread = 0;
		static Backtrace firstThreadBacktrace;

		Backtrace *Backtrace::GetGlobalBacktrace() {
			Uint32 thread = SDL_ThreadID();
			if (firstThread == 0)
				firstThread = thread;
			if (thread == firstThread) {
				return &firstThreadBacktrace;
			}
			std::map<Uint32, Backtrace *>::iterator it = globalBacktrace.find(thread);
			if (it == globalBacktrace.end()) {
				Backtrace *t = new Backtrace();
				globalBacktrace[thread] = t;
				return t;
			} else {
				return it->second;
			}
		}

		void Backtrace::ThreadExiting() {
			Uint32 thread = SDL_ThreadID();
			if (thread == firstThread)
				return;
			std::map<Uint32, Backtrace *>::iterator it = globalBacktrace.find(thread);
			if (it != globalBacktrace.end()) {
				delete it->second;
				globalBacktrace.erase(it);
			}
		}

		void Backtrace::StartBacktrace() {}

#endif

		void Backtrace::Push(const spades::reflection::BacktraceEntry &entry) {
			entries.push_back(entry);
		}

		void Backtrace::Pop() {
			SPAssert(entries.size() > 0);
			entries.pop_back();
		}

		std::vector<BacktraceEntry> Backtrace::GetAllEntries() { return entries; }

		std::string Backtrace::ToString() const { return BacktraceRecordToString(entries); }
		std::string BacktraceRecordToString(const BacktraceRecord &entries) {
			std::string message;
			char buf[1024];
			if (entries.empty()) {
				message += "(none)";
			} else {
				for (size_t i = 0; i < entries.size(); i++) {
					const reflection::BacktraceEntry &ent = entries[entries.size() - 1 - i];
					const reflection::Function &func = ent.GetFunction();

					std::string fn = func.GetFileName();
					size_t ind = fn.find_last_of('/');
					if (ind != std::string::npos) {
						fn = fn.substr(ind + 1);
					}

					sprintf(buf, "%s at %s:%d\n", func.GetName(), fn.c_str(), func.GetLineNumber());
					message += buf;
				}
			}
			return message;
		}
	} // namespace reflection

#pragma mark -

	namespace {
		class BoundedLogBuffer {
		public:
			void Push(std::string &&line) {
				lines.push_back(std::move(line));

				if (lines.size() > 1000) {
					++overflow;
					lines.pop_front();
				}
			}

			void MergeFrom(BoundedLogBuffer &&other) {
				for (auto &line : other.lines) {
					Push(std::move(line));
				}
				other.lines.clear();
			}

			/** Flush log lines. Not safe to call `Push` while in progress. */
			template <class C> void Flush(C &&cb) {
				if (overflow > 0) {
					cb(Format("[{} log lines were lost]", overflow));
					overflow = 0;
				}
				for (auto &line : lines) {
					cb(std::move(line));
				}
				lines.clear();
			}

		private:
			std::deque<std::string> lines;
			std::size_t overflow = 0;
		};

		/** Stores log lines to be displayed in the internal console. */
		BoundedLogBuffer g_consoleLogBuffer;
	} // namespace

	static std::unique_ptr<IStream> logStream;
	static bool attemptedToInitializeLog = false;
	static std::string accumlatedLog;

	void StartLog() {
		attemptedToInitializeLog = true;
		logStream = FileManager::OpenForWriting("SystemMessages.log");

		logStream->Write(accumlatedLog);
		accumlatedLog.clear();
	}

	void GetBufferedLogLines(stmp::dyn_function<void(std::string)> &&cb) {
		BoundedLogBuffer tmp;

		// Swap log buffers because `Push` is not safe to call while
		// `Flush` is in progress
		std::swap(tmp, g_consoleLogBuffer);

		tmp.Flush(cb);

		// Swap them back
		std::swap(tmp, g_consoleLogBuffer);

		// Process log lines recoded while we were doing this
		g_consoleLogBuffer.MergeFrom(std::move(tmp));
	}

	// TODO: What happened to thread-safety here

	void LogMessage(const char *file, int line, const char *format, ...) {
		char buf[4096];
		va_list va;
		va_start(va, format);
		vsnprintf(buf, sizeof(buf), format, va);
		va_end(va);
		std::string str = buf;
		std::string fn = file;
		if (fn.rfind('/') != std::string::npos)
			fn = fn.substr(fn.rfind('/') + 1);

		time_t t;
		struct tm tm;
		time(&t);
		tm = *localtime(&t);

		// lm: using \r\n instead of \n so that some shitty windows editors (notepad f.e.) can parse
		// this file aswell (all decent editors should ignore it anyway)
		snprintf(buf, sizeof(buf), "%04d/%02d/%02d %02d:%02d:%02d [%s:%d] %s\r\n",
		         tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec,
		         fn.c_str(), line, str.c_str());
		buf[sizeof(buf) - 1] = 0;

		// Log messages are outputted to three destinations.

		// (1) stdout
		std::string outStr = EscapeControlCharacters(buf);
		printf("%s", outStr.c_str());

		// (2) The log file a.k.a. `SystemMessages.log`
		if (logStream || !attemptedToInitializeLog) {

			if (attemptedToInitializeLog) {
				logStream->Write(outStr);
				logStream->Flush();
			} else
				accumlatedLog += buf;
		}

		// (3) The internal console window
		std::string consoleLogLine{buf, strlen(buf) - 2}; // Remove `\r\n`
		g_consoleLogBuffer.Push(std::move(consoleLogLine));
	}
} // namespace spades
