//
//  ConcurrentDispatch.h
//  OpenSpades
//
//  Created by yvt on 7/27/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IRunnable.h"
#include <string>
#include <exception>

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
		void EnterEventLoop() throw();
		
		void MarkSDLVideoThread();
	};
	
	class ConcurrentDispatch: public IRunnable {
		friend class DispatchThread;
		friend class DispatchQueue;
		friend struct SyncQueueEntry;
		
		std::string name;
		SyncQueueEntry * volatile entry;
		
		IRunnable *runnable;
		
		void Execute();
		void ExecuteProtected() throw();
		
		// disable
		ConcurrentDispatch(const ConcurrentDispatch&){}
		void operator =(const ConcurrentDispatch& disp){}
	public:
		ConcurrentDispatch();
		ConcurrentDispatch(std::string name);
		virtual ~ConcurrentDispatch();
		
		void Start();
		void StartOn(DispatchQueue *);
		void Join();
		
		/** abandons the ownership of this dispatch instance, and
		 * when the dispatch completes, this instance will be deleted. */
		void Release();
		
		virtual void Run();
		
		void SetRunnable(IRunnable * r) {runnable = r;}
		IRunnable *GetRunnable() const { return runnable; }
		
	};
}