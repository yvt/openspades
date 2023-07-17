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

#include <exception>
#include <string>

#include "IRunnable.h"

namespace spades {
	struct SyncQueueEntry;
	class DispatchThread;
	class SynchronizedQueue;
	class ConcurrentDispatch;

	class DispatchQueue {
		friend class ConcurrentDispatch;
		SynchronizedQueue *internal;
		DispatchQueue();

	public:
		~DispatchQueue();
		static DispatchQueue *GetThreadQueue();
		void ProcessQueue();
		void EnterEventLoop() noexcept;

		void MarkSDLVideoThread();
	};

	class ConcurrentDispatch : public IRunnable {
		friend class DispatchThread;
		friend class DispatchQueue;
		friend struct SyncQueueEntry;

		std::string name;
		SyncQueueEntry *volatile entry;

		IRunnable *runnable;

		void Execute();
		void ExecuteProtected() noexcept;

		// disable
		ConcurrentDispatch(const ConcurrentDispatch &) {}
		void operator=(const ConcurrentDispatch &disp) {}

	public:
		ConcurrentDispatch();
		ConcurrentDispatch(std::string name);
		~ConcurrentDispatch();

		void Start();
		void StartOn(DispatchQueue *);
		void Join();

		/** abandons the ownership of this dispatch instance, and
		 * when the dispatch completes, this instance will be deleted. */
		void Release();

		void Run() override;

		void SetRunnable(IRunnable *r) { runnable = r; }
		IRunnable *GetRunnable() const { return runnable; }
	};

	template <class F> class FunctionDispatch : public ConcurrentDispatch {
		F f;

	public:
		FunctionDispatch(F f) : f(f) {}
		void Run() override { f(); }
	};
}
