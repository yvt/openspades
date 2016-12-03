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

#include <typeinfo>

#include <Imports/SDL.h>

#include "AutoLocker.h"
#include "ConcurrentDispatch.h"
#include "Debug.h"
#include "Thread.h"
#include "ThreadLocalStorage.h"

namespace spades {

	Thread::Thread() : runnable(NULL), autoDelete(false) { threadInfo = NULL; }

	Thread::Thread(IRunnable *r) : runnable(r), autoDelete(false) {
		threadInfo = NULL;
		threadId = 0;
	}

	class ThreadCleanuper : public ConcurrentDispatch {
		std::vector<SDL_Thread *> threads;
		Mutex mutex;

	public:
		void Add(SDL_Thread *thread) {
			AutoLocker locker(&mutex);
			threads.push_back(thread);
		}
		void Cleanup() {
			AutoLocker locker(&mutex);
			for (size_t i = 0; i < threads.size(); i++)
				SDL_WaitThread(threads[i], NULL);
			threads.clear();
		}
	};

	static ThreadCleanuper *cleanuper;

	void Thread::InitThreadSystem() { cleanuper = new ThreadCleanuper(); }

	void Thread::CleanupExitedThreads() { cleanuper->Cleanup(); }

	Thread::~Thread() {
		SDL_Thread *th = NULL;
		{
			AutoLocker locker(&lock);
			th = (SDL_Thread *)threadInfo;
			if (!th)
				return;
		}

		// we have to ensure thread handle is destroyed.
		if (SDL_ThreadID() == threadId) {
			// thread is deleting itself.
			// SDL_WaitThread would cause deadlock.
			cleanuper->Add(th);
		} else {
			SDL_WaitThread(th, NULL);
		}
	}

	void Thread::Start() {
		AutoLocker locker(&lock);
		if (threadInfo)
			return;

		const std::type_info &info = typeid(*this);
		const char *name = info.name();
		if (name == nullptr)
			name = "(null)";

		threadId = 0;
		threadInfo = SDL_CreateThread(InternalRunner, name, this);
	}

	void Thread::Join() {
		if (autoDelete) {
			SPRaise("Attempted join a thread that is marked for auto deletion.");
		}
		SDL_Thread *th = NULL;
		{
			AutoLocker locker(&lock);
			th = (SDL_Thread *)threadInfo;
			if (!th)
				return;
		}
		SDL_WaitThread(th, NULL);
	}

	bool Thread::IsAlive() {
		if (autoDelete) {
			SPRaise("Attempted query the state of a thread that is marked for auto deletion.");
		}
		return threadInfo != NULL;
	}

	int Thread::InternalRunner(void *th) {
		Thread *self = (Thread *)th;
		self->threadId = SDL_ThreadID();
		try {
			SPADES_MARK_FUNCTION();
			self->Run();
			self->Quited();
		} catch (const std::exception &ex) {
			printf("Exception in thread:\n%s\n", ex.what());
			throw;
		} catch (...) {
			printf("Exception in thread: unknown\n");
			throw;
		}

		// call TLSs' destructors
		::spades::ThreadExiting();

		::spades::reflection::Backtrace::ThreadExiting();
		return 0;
	}

	void Thread::Quited() {
		lock.Lock();
		threadInfo = NULL;
		if (autoDelete) {
			delete this;
			return;
		}
		lock.Unlock();
	}

	void Thread::Run() {
		if (runnable)
			runnable->Run();
	}

	void Thread::MarkForAutoDeletion() {
		lock.Lock();
		if (threadInfo == NULL) {
			// already exited
			delete this;
			return;
		}
		autoDelete = true;
		lock.Unlock();
	}
}
