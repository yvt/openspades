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

#include <string>

#include "Exception.h"

namespace spades {

	class ThreadLocalStorageImpl {
	public:
		class IValueDestructor {
		public:
			virtual void Destruct(void *) = 0;
		};
		IValueDestructor *destructor;
		virtual ~ThreadLocalStorageImpl() {}
		static ThreadLocalStorageImpl *Create();
		virtual void Set(void *) = 0;
		virtual void *Get() = 0;
	};

	template <typename T>
	class ThreadLocalStorage : public ThreadLocalStorageImpl::IValueDestructor {
		ThreadLocalStorageImpl *internal;
		std::string name;

	public:
		ThreadLocalStorage() : name("(unnamed)") {
			internal = ThreadLocalStorageImpl::Create();
			internal->destructor = this;
		}
		ThreadLocalStorage(std::string name) : name(name) {
			internal = ThreadLocalStorageImpl::Create();
			internal->destructor = this;
		}
		~ThreadLocalStorage() {
			// FIXME: should support deleting tls?
		}
		void operator=(T *ptr) { internal->Set(reinterpret_cast<void *>(ptr)); }
		T *GetPointer() { return reinterpret_cast<T *>(internal->Get()); }
		T &operator*() {
			T *v = GetPointer();
			if (v == NULL)
				SPRaise("Attempted to get a reference to the null value of TLS '%s'", name.c_str());
			return *v;
		}
		T *operator->() { return GetPointer(); }
		operator T *() { return GetPointer(); }
		void Destruct(void *) override {}
	};

	template <typename T> class AutoDeletedThreadLocalStorage : public ThreadLocalStorage<T> {
	public:
		AutoDeletedThreadLocalStorage() : ThreadLocalStorage<T>() {}
		AutoDeletedThreadLocalStorage(std::string name) : ThreadLocalStorage<T>(name) {}
		void Destruct(void *v) override { delete reinterpret_cast<T *>(v); }
		void operator=(T *ptr) { *static_cast<ThreadLocalStorage<T> *>(this) = ptr; }
	};

	void ThreadExiting();
}
