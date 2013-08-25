//
//  Debug.h
//  OpenSpades
//
//  Created by yvt on 7/16/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <vector>
#include "Exception.h"

namespace spades {
	namespace reflection {
		class Function{
			const char *name;
			const char *file;
			int line;
		public:
			Function(const char *name, const char *File,
					 int line);
			
			const char *GetName() const { return name; }
			const char *GetFileName() const { return file; }
			int GetLineNumber() const { return line; }
		};
		
		class Backtrace;
		
		class BacktraceEntry {
			Function *function;
		public:
			BacktraceEntry() {}
			BacktraceEntry(Function *f):
			function(f) {}
			
			const Function& GetFunction() const { return *function; }
		};
		
		class BacktraceEntryAdder {
			Backtrace *bt;
		public:
			BacktraceEntryAdder(const BacktraceEntry&);
			~BacktraceEntryAdder();
			
		};
		
		class Backtrace {
			std::vector<BacktraceEntry> entries;
		public:
			static Backtrace *GetGlobalBacktrace();
			static void ThreadExiting();
			static void StartBacktrace();
			
			void Push(const BacktraceEntry&);
			void Pop();
			
			std::vector<BacktraceEntry> GetAllEntries();
			
			std::string ToString() const;
		};
	}
	void StartLog();
	void LogMessage(const char *file, int line,
						   const char *format, ...);
}

#define SPADES_MARK_FUNCTION() \
static ::spades::reflection::Function thisFunction(__PRETTY_FUNCTION__, __FILE__, __LINE__); \
::spades::reflection::BacktraceEntryAdder backtraceEntryAdder((::spades::reflection::BacktraceEntry(&thisFunction)))

#if NDEBUG
#define SPADES_MARK_FUNCTION_DEBUG() do{}while(0)
#else
#define SPADES_MARK_FUNCTION_DEBUG() SPADES_MARK_FUNCTION()
#endif

#if NDEBUG
#define SPAssert(cond) do{}while(0)
#else
#define SPAssert(cond) ((!(cond)) ? SPRaise("SPAssertion failed: %s", #cond ) : (void)0)
#endif

#define SPLog(format, args...) ::spades::LogMessage(__FILE__, __LINE__, format, ##args )

#ifdef __GNUC__
#define DEPRECATED(func) func __attribute__ ((deprecated))
#elif defined(_MSC_VER)
#define DEPRECATED(func) __declspec(deprecated) func
#else
#pragma message("WARNING: You need to implement DEPRECATED for this compiler")
#define DEPRECATED(func) func
#endif
