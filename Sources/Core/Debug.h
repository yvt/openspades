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

#pragma once

#include <vector>

#include "Exception.h"

namespace spades {
	namespace reflection {
		class Function {
			const char *name;
			const char *file;
			int line;

		public:
			Function(const char *name, const char *File, int line);

			const char *GetName() const { return name; }
			const char *GetFileName() const { return file; }
			int GetLineNumber() const { return line; }
		};

		class Backtrace;

		class BacktraceEntry {
			Function *function;

		public:
			BacktraceEntry() {}
			BacktraceEntry(Function *f) : function(f) {}

			const Function &GetFunction() const { return *function; }
		};

		class BacktraceEntryAdder {
			Backtrace *bt;

		public:
			BacktraceEntryAdder(const BacktraceEntry &);
			~BacktraceEntryAdder();
		};

		typedef std::vector<BacktraceEntry> BacktraceRecord;

		class Backtrace {
			std::vector<BacktraceEntry> entries;

		public:
			static Backtrace *GetGlobalBacktrace();
			static void ThreadExiting();
			static void StartBacktrace();

			void Push(const BacktraceEntry &);
			void Pop();

			BacktraceRecord GetAllEntries();
			BacktraceRecord GetRecord() { return GetAllEntries(); }

			std::string ToString() const;
		};

		std::string BacktraceRecordToString(const BacktraceRecord &);
	}
	void StartLog();

	void LogMessage(const char *file, int line, const char *format, ...)
#ifdef __GNUC__
	  __attribute__((format(printf, 3, 4)))
#endif
	  ;
}

#ifdef _MSC_VER
#define __PRETTY_FUNCTION__ __FUNCDNAME__
#endif

#define SPADES_MARK_FUNCTION()                                                                     \
	static ::spades::reflection::Function thisFunction(__PRETTY_FUNCTION__, __FILE__, __LINE__);   \
	::spades::reflection::BacktraceEntryAdder backtraceEntryAdder(                                 \
	  (::spades::reflection::BacktraceEntry(&thisFunction)))

#if NDEBUG
#define SPADES_MARK_FUNCTION_DEBUG()                                                               \
	do {                                                                                           \
	} while (0)
#else
#define SPADES_MARK_FUNCTION_DEBUG() SPADES_MARK_FUNCTION()
#endif

#if NDEBUG
#define SPAssert(cond)                                                                             \
	do {                                                                                           \
	} while (0)
#else
#define SPAssert(cond) ((!(cond)) ? SPRaise("SPAssertion failed: %s", #cond) : (void)0)
#endif

#ifdef _MSC_VER
#define SPLog(format, ...) ::spades::LogMessage(__FILE__, __LINE__, format, __VA_ARGS__)
#else
#define SPLog(format, args...) ::spades::LogMessage(__FILE__, __LINE__, format, ##args)
#endif

#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif
