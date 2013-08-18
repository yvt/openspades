//
//  AutoLocker.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "ILockable.h"

namespace spades {
	class AutoLocker {
		ILockable *lockable;
		
	public:
		AutoLocker(ILockable *l):
		lockable(l){
			if(lockable)
				lockable->Lock();
		}
		AutoLocker(const AutoLocker& l):
		lockable(l.lockable){
			if(lockable)
				lockable->Lock();
		}
		~AutoLocker() {
			if(lockable)
				lockable->Unlock();
		}
		ILockable *Release() {
			ILockable *l = lockable;
			lockable = 0;
			return l;
		}
		ILockable *GetLockable() {
			return lockable;
		}
		void operator =(const AutoLocker& l){
			if(l.lockable)
				l.lockable->Lock();
			if(lockable)
				lockable->Unlock();
			lockable = l.lockable;
		}
	};
}
