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

#include "Thread.h"
#include "../Imports/SDL.h"
#include "AutoLocker.h"
#include "Debug.h"
#include "ThreadLocalStorage.h"

namespace spades {
	
	Thread::Thread():
	runnable(NULL){
		threadInfo = NULL;
	}
	
	Thread::Thread(IRunnable *r):
	runnable(r){
		threadInfo = NULL;
	}
	
	Thread::~Thread() {
		
	}
	
	void Thread::Start() {
		AutoLocker locker(&lock);
		if(threadInfo)
			return;
		
		threadInfo = SDL_CreateThread(InternalRunner, this);
	}
	
	void Thread::Join() {
		SDL_Thread *th = NULL;
		{
			AutoLocker locker(&lock);
			th = (SDL_Thread *)threadInfo;
			if(!th)
				return;
		}
		SDL_WaitThread(th, NULL);
	}
	
	bool Thread::IsAlive() {
		return threadInfo != NULL;
	}
	
	int Thread::InternalRunner(void *th) {
		try{
			SPADES_MARK_FUNCTION();
			Thread *self = (Thread *)th;
			self->Run();
			self->Quited();
		}catch(const std::exception& ex){
			printf("Exception in thread:\n%s\n", ex.what());
			throw;
		}catch(...){
			printf("Exception in thread: unknown\n");
			throw;
		}
		
		// call TLSs' destructors 
		::spades::ThreadExiting();

		::spades::reflection::Backtrace::ThreadExiting();
		return 0;
	}
	
	void Thread::Quited() {
		AutoLocker locker(&lock);
		threadInfo = NULL;
	}

	void Thread::Run() {
		if(runnable)
			runnable->Run();
	}
}

