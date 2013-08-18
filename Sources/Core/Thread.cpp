//
//  Thread.cpp
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

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

