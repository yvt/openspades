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

#include "ILockable.h"

namespace spades {
	class AutoLocker {
		ILockable *lockable;

	public:
		AutoLocker(ILockable *l) : lockable(l) {
			if (lockable)
				lockable->Lock();
		}
		AutoLocker(const AutoLocker &l) : lockable(l.lockable) {
			if (lockable)
				lockable->Lock();
		}
		~AutoLocker() {
			if (lockable)
				lockable->Unlock();
		}
		ILockable *Release() {
			ILockable *l = lockable;
			lockable = 0;
			return l;
		}
		ILockable *GetLockable() { return lockable; }
		void operator=(const AutoLocker &l) {
			if (l.lockable)
				l.lockable->Lock();
			if (lockable)
				lockable->Unlock();
			lockable = l.lockable;
		}
	};
}
