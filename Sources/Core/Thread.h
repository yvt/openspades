//
//  Thread.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "Mutex.h"
#include "IRunnable.h"

namespace spades {
	
	class Thread {
		void *threadInfo;
		Mutex lock;
		IRunnable *runnable;
		
		static int InternalRunner(void *);
		void Quited();
	public:
		Thread();
		Thread(IRunnable *r);
		virtual ~Thread();
		
		virtual void Run();
		
		void Start();
		void Join();
		
		bool IsAlive();
	};
	

}
