//
//  Mutex.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once
#include "ILockable.h"

namespace spades {
	/** Recursive mutex. */
	class Mutex: public ILockable {
		void *priv;
	public:
		Mutex();
		~Mutex();
		
		virtual void Lock();
		virtual void Unlock();
	};
}
