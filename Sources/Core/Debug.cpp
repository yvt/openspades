//
//  Debug.cpp
//  OpenSpades
//
//  Created by yvt on 7/16/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "Debug.h"
#include "../Core/Debug.h"
#include <map>
#include "../Imports/SDL.h"
#include <string>


#define SPADES_USE_TLS 1

#if SPADES_USE_TLS
#include "ThreadLocalStorage.h"
#endif // SPADES_USE_TLS

namespace spades {
	namespace reflection {
		Function::Function(const char *n,
						   const char *f,
						   int l):
		name(n), file(f), line(l){
			
		}
		
		BacktraceEntryAdder::BacktraceEntryAdder(const BacktraceEntry& entry){
			bt = Backtrace::GetGlobalBacktrace();
			if(bt)
				bt->Push(entry);
		}
		
		BacktraceEntryAdder::~BacktraceEntryAdder() {
			if(bt)
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
			if(!backtraceStarted)
				return NULL;
			Backtrace *b = backtraceTls;
			if(!b){
				b = new Backtrace;
				backtraceTls = b;
			}
			return b;
		}
		
		void Backtrace::ThreadExiting() {
		}
		
		void Backtrace::StartBacktrace(){
			backtraceStarted = true;
		}
		
#else
		
		static std::map<Uint32, Backtrace *> globalBacktrace;
		static Uint32 firstThread = 0;
		static Backtrace firstThreadBacktrace;
		
		Backtrace *Backtrace::GetGlobalBacktrace() {
			Uint32 thread = SDL_ThreadID();
			if(firstThread == 0)
				firstThread = thread;
			if(thread == firstThread){
				return &firstThreadBacktrace;
			}
			std::map<Uint32, Backtrace *>::iterator it = globalBacktrace.find(thread);
			if(it == globalBacktrace.end()){
				Backtrace *t = new Backtrace();
				globalBacktrace[thread] = t;
				return t;
			}else{
				return it->second;
			}
		}
		
		void Backtrace::ThreadExiting() {
			Uint32 thread = SDL_ThreadID();
			if(thread == firstThread)
				return;
			std::map<Uint32, Backtrace *>::iterator it = globalBacktrace.find(thread);
			if(it != globalBacktrace.end()){
				delete it->second;
				globalBacktrace.erase(it);
			}
		}
		
		void Backtrace::StartBacktrace(){
			
		}
		
#endif
		
		void Backtrace::Push(const spades::reflection::BacktraceEntry &entry) {
			entries.push_back(entry);
		}
		
		void Backtrace::Pop() {
			SPAssert(entries.size() > 0);
			entries.pop_back();
		}
		
		std::vector<BacktraceEntry> Backtrace::GetAllEntries() {
			return entries;
		}
		
		std::string Backtrace::ToString() const {
			std::string message;
			char buf[1024];
			if(entries.empty()){
				message += "(none)";
			}else{
				for(size_t i = 0; i < entries.size(); i++){
					const reflection::BacktraceEntry& ent = entries[entries.size() - 1 - i];
					const reflection::Function& func = ent.GetFunction();
					
					std::string fn = func.GetFileName();
					size_t ind = fn.find_last_of('/');
					if(ind != std::string::npos){
						fn = fn.substr(ind + 1);
					}
					
					sprintf(buf, "%s at %s:%d\n",
							func.GetName(),
							fn.c_str(),
							func.GetLineNumber());
					message += buf;
				}
			}
			return message;
		}
	}
}

