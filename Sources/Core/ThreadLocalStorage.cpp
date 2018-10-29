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

#include <algorithm>
#include <mutex>
#include <unordered_set>

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "ThreadLocalStorage.h"

namespace spades {

	namespace {
		class ThreadLocalStorageImplInternal;

		class TlsManager
		{
		public:
			void AddTls(ThreadLocalStorageImplInternal *tls) {
				std::lock_guard<std::mutex> lock{m_mutex};

				m_allTlses.insert(tls);
			}
			void RemoveTls(ThreadLocalStorageImplInternal *tls) {
				std::lock_guard<std::mutex> lock{m_mutex};

				auto it = m_allTlses.find(tls);
				SPAssert(it != m_allTlses.end());
				m_allTlses.erase(it);
			}

			void ThreadExiting();

			static TlsManager &GetInstance() {
				// This object will NEVER be destroyed (until the program really exits)
				static TlsManager *instance = new TlsManager();
				return *instance;
			}

		private:
			std::unordered_set<ThreadLocalStorageImplInternal *> m_allTlses;
			std::mutex m_mutex;
		};


		class ThreadLocalStorageImplInternal : public ThreadLocalStorageImpl {
		public:
			ThreadLocalStorageImplInternal() {
				// Be careful: this function can be called during static storage object construction
				TlsManager::GetInstance().AddTls(this);
			}
			~ThreadLocalStorageImplInternal() {
				TlsManager::GetInstance().RemoveTls(this);
			}
			void ThreadExiting() {
				void *p = Get();
				if (p)
					destructor->Destruct(p);
			}
		};
	}

	void TlsManager::ThreadExiting() {
		std::lock_guard<std::mutex> lock{m_mutex};

		for (auto *tls: m_allTlses) {
			tls->ThreadExiting();
		}
	}

	void ThreadExiting() {
		// Be careful: this function can be called during static storage object destruction
		TlsManager::GetInstance().ThreadExiting();
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
		~Win32ThreadLocalStorageImpl() { TlsFree(key); }

		void Set(void *value) override { TlsSetValue(key, value); }

		void *Get() override { return TlsGetValue(key); }
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
		~PThreadThreadLocalStorageImpl() { pthread_key_delete(key); }

		void Set(void *value) override {
			if (pthread_setspecific(key, value)) {
				SPRaise("Failed to set TLS value");
			}
		}

		void *Get() override { return pthread_getspecific(key); }
	};

	ThreadLocalStorageImpl *ThreadLocalStorageImpl::Create() {
		return new PThreadThreadLocalStorageImpl();
	}
#endif
}
