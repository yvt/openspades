//
//  ThreadLocalStorage.h
//  OpenSpades
//
//  Created by yvt on 7/29/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "Exception.h"
#include <string>

namespace spades {
	
	class ThreadLocalStorageImpl {
	public:
		class IValueDestructor {
		public:
			virtual void Destruct(void *) = 0;
		};
		IValueDestructor *destructor;
		virtual ~ThreadLocalStorageImpl(){}
		static ThreadLocalStorageImpl *Create();
		virtual void Set(void *) = 0;
		virtual void *Get() = 0;
	};
	
	template<typename T>
	class ThreadLocalStorage: public ThreadLocalStorageImpl::IValueDestructor {
		ThreadLocalStorageImpl *internal;
		std::string name;
	public:
		ThreadLocalStorage():name("(unnamed)"){
			internal = ThreadLocalStorageImpl::Create();
			internal->destructor = this;
		}
		ThreadLocalStorage(std::string name):name(name){
			internal = ThreadLocalStorageImpl::Create();
			internal->destructor = this;
		}
		virtual ~ThreadLocalStorage(){
			// FIXME: should support deleting tls?
		}
		void operator =(T *ptr) {
			internal->Set(reinterpret_cast<void*>(ptr));
		}
		T *GetPointer() {
			return reinterpret_cast<T *>(internal->Get());
		}
		T& operator *() {
			T *v = GetPointer();
			if(v == NULL)
				SPRaise("Attempted to get a reference to the null value of TLS '%s'", name.c_str());
			return *v;
		}
		T *operator ->(){
			return GetPointer();
		}
		operator T *(){
			return GetPointer();
		}
		virtual void Destruct(void *) {
		}
	};
		
	template<typename T>
	class AutoDeletedThreadLocalStorage: public ThreadLocalStorage<T> {
	public:
		AutoDeletedThreadLocalStorage():
		ThreadLocalStorage<T>(){}
		AutoDeletedThreadLocalStorage(std::string name):
		ThreadLocalStorage<T>(name){}
		virtual void Destruct(void *v) {
			delete reinterpret_cast<T *>(v);
		}
		void operator =(T *ptr) {
			*static_cast<ThreadLocalStorage<T> *>(this) = ptr;
		}
	};
		
	
	void ThreadExiting();
}
