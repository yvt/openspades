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

#include "Debug.h"
#include "Mutex.h"

#define DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE 0

namespace spades {

	class RefCountedObject {
		int refCount;
#if DEBUG_REFCOUNTED_OBJECT_LAST_RELEASE
		reflection::BacktraceRecord lastRelease;
		reflection::BacktraceRecord secondLastRelease;
		Mutex releaseInfoMutex;
#endif
	protected:
		virtual ~RefCountedObject();

	public:
		RefCountedObject();

		void AddRef();
		void Release();
	};

	template <typename T> class Handle {
		T *ptr;

	public:
		Handle(T *ptr, bool add = true) : ptr(ptr) {
			if (ptr && add)
				ptr->AddRef();
		}
		Handle() : ptr(0) {}
		Handle(const Handle<T> &h) : ptr(h.ptr) {
			if (ptr)
				ptr->AddRef();
		}
		~Handle() {
			if (ptr)
				ptr->Release();
		}
		T *operator->() {
			SPAssert(ptr != NULL);
			return ptr;
		}
		const T *operator->() const {
			SPAssert(ptr != NULL);
			return ptr;
		}
		T &operator*() {
			SPAssert(ptr != NULL);
			return *ptr;
		}
		const T &operator*() const {
			SPAssert(ptr != NULL);
			return *ptr;
		}
		void Set(T *p, bool add = true) {
			if (p == ptr) {
				if ((!add) && ptr != nullptr)
					ptr->Release();
				return;
			}
			T *old = ptr;
			ptr = p;
			if (add && ptr)
				ptr->AddRef();
			if (old)
				old->Release();
		}
		void operator=(T *p) { Set(p); }
		void operator=(const Handle<T> &h) { Set(h.ptr, true); }
		operator T *() { return ptr; }
		T *Unmanage() {
			SPAssert(ptr != NULL);
			T *p = ptr;
			ptr = NULL;
			return p;
		}
		operator bool() { return ptr != NULL; }
	};
}
