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

#include <vector>

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "ThreadLocalStorage.h"

namespace spades {

	class ThreadLocalStorageImplInternal;

	static std::vector<ThreadLocalStorageImplInternal *> allTls;

	class ThreadLocalStorageImplInternal : public ThreadLocalStorageImpl {
	public:
		ThreadLocalStorageImplInternal() { allTls.push_back(this); }
		~ThreadLocalStorageImplInternal() {
			// TODO: remove this from allTls?
		}
		void ThreadExiting() {
			void *p = Get();
			if (p)
				destructor->Destruct(p);
		}
	};

	void ThreadExiting() {
		for (size_t i = 0; i < allTls.size(); i++) {
			allTls[i]->ThreadExiting();
		}
	}

#ifdef WIN32
	class Win32ThreadLocalStorageImpl : public ThreadLocalStorageImplInternal {
		DWORD key;

	public:
		Win32ThreadLocalStorageImpl() {
			key = TlsAlloc();
			if (key + 1 == 0) {
				SPRaise("Failed to create Windows TLS key");
			}
		}
		virtual ~Win32ThreadLocalStorageImpl() { TlsFree(key); }

		virtual void Set(void *value) { TlsSetValue(key, value); }

		virtual void *Get() { return TlsGetValue(key); }
	};

	ThreadLocalStorageImpl *ThreadLocalStorageImpl::Create() {
		return new Win32ThreadLocalStorageImpl();
	}

#else
	class PThreadThreadLocalStorageImpl : public ThreadLocalStorageImplInternal {
		pthread_key_t key;

	public:
		PThreadThreadLocalStorageImpl() {
			if (pthread_key_create(&key, NULL)) {
				SPRaise("Failed to create PThread TLS key");
			}
		}
		virtual ~PThreadThreadLocalStorageImpl() { pthread_key_delete(key); }

		virtual void Set(void *value) {
			if (pthread_setspecific(key, value)) {
				SPRaise("Failed to set TLS value");
			}
		}

		virtual void *Get() { return pthread_getspecific(key); }
	};

	ThreadLocalStorageImpl *ThreadLocalStorageImpl::Create() {
		return new PThreadThreadLocalStorageImpl();
	}
#endif
}
