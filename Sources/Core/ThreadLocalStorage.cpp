//
//  ThreadLocalStorage.cpp
//  OpenSpades
//
//  Created by yvt on 7/29/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "ThreadLocalStorage.h"
#include <vector>

#ifdef WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace spades {
	
	class ThreadLocalStorageImplInternal;
	
	static std::vector<ThreadLocalStorageImplInternal *> allTls;
	
	class ThreadLocalStorageImplInternal: public ThreadLocalStorageImpl{
	public:
		ThreadLocalStorageImplInternal(){
			allTls.push_back(this);
		}
		~ThreadLocalStorageImplInternal(){
			// TODO: remove this from allTls?
		}
		void ThreadExiting() {
			void *p = Get();
			if(p)
				destructor->Destruct(p);
		}
	};
	
	void ThreadExiting() {
		for(size_t i = 0; i < allTls.size(); i++){
			allTls[i]->ThreadExiting();
		}
	}
	
#ifdef WIN32
	class Win32ThreadLocalStorageImpl: public ThreadLocalStorageImplInternal{
		DWORD key;
	public:
		Win32ThreadLocalStorageImpl() {
			key = TlsAlloc();
			if(key + 1 == 0){
				SPRaise("Failed to create Windows TLS key");
			}
		}
		virtual ~Win32ThreadLocalStorageImpl(){
			TlsFree(key);
		}
		
		virtual void Set(void *value){
			TlsSetValue(key, value);
		}
		
		virtual void *Get() {
			return TlsGetValue(key);
		}
	};
	
	ThreadLocalStorageImpl *ThreadLocalStorageImpl::Create() {
		return new Win32ThreadLocalStorageImpl();
	}
	
#else
	class PThreadThreadLocalStorageImpl: public ThreadLocalStorageImplInternal{
		pthread_key_t key;
	public:
		PThreadThreadLocalStorageImpl() {
			if(pthread_key_create(&key, NULL)){
				SPRaise("Failed to create PThread TLS key");
			}
		}
		virtual ~PThreadThreadLocalStorageImpl(){
			pthread_key_delete(key);
		}
		
		virtual void Set(void *value){
			if(pthread_setspecific(key, value)){
				SPRaise("Failed to set TLS value");
			}
		}
		
		virtual void *Get() {
			return pthread_getspecific(key);
		}
	};
	
	ThreadLocalStorageImpl *ThreadLocalStorageImpl::Create() {
		return new PThreadThreadLocalStorageImpl();
	}
#endif
	
}
