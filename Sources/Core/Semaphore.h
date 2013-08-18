//
//  Semaphore.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once
#include "ILockable.h"

namespace spades {
	class Semaphore: public ILockable {
		void *priv;
	public:
		Semaphore(int initial = 1);
		~Semaphore();
		
		void Post();
		void Wait();
		
		virtual void Lock() {Wait();}
		virtual void Unlock() {Post();}
	};
}
