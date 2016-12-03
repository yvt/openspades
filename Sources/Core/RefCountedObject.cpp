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

#include "RefCountedObject.h"
#include <ScriptBindings/ScriptManager.h>
#include "AutoLocker.h"
#include "Exception.h"

namespace spades {
	RefCountedObject::RefCountedObject() { refCount = 1; }

	RefCountedObject::~RefCountedObject() {}

	void RefCountedObject::AddRef() { asAtomicInc(refCount); }

	void RefCountedObject::Release() {
#if DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE
		AutoLocker guard(&releaseInfoMutex);
#endif
		int cnt = asAtomicDec(refCount);
		if (cnt == 0) {
#if DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE

#else
			delete this;
#endif
		} else if (cnt < 0)
#if DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE
			SPRaise("Attempted to release already destroyed object\n===== LAST RELEASE BACKTRACE "
			        "=====\n%s\n===== SECOND LAST RELEASE BACKTRACE =====\n%s\n===== LAST RELEASE "
			        "BACKTRACE ENDS =====",
			        reflection::BacktraceRecordToString(lastRelease).c_str(),
			        reflection::BacktraceRecordToString(secondLastRelease).c_str());
#else
			SPRaise("Attempted to release already destroyed object");
#endif

#if DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE
		secondLastRelease = std::move(lastRelease);
		lastRelease = std::move(reflection::Backtrace::GetGlobalBacktrace()->GetRecord());
#endif
	}
}
