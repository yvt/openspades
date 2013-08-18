//
//  DynamicLibrary.cpp
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#ifdef WIN32

#include "DynamicLibrary.h"
#include "Exception.h"
#include <windows.h>
#include "Debug.h"

namespace spades {
	DynamicLibrary::DynamicLibrary(const char *fn){
		SPADES_MARK_FUNCTION();
		
		name = fn;
		handle = (void *)LoadLibrary(fn);
		if(handle == NULL){
			DWORD err = GetLastError();
			SPRaise("Failed to dlload '%s': 0x%08x", fn, (int)err);
		}
	}
	
	DynamicLibrary::~DynamicLibrary() {
		SPADES_MARK_FUNCTION();
		
		FreeLibrary((HINSTANCE)handle);
	}
	
	void *DynamicLibrary::GetSymbolOrNull(const char *name){
		SPADES_MARK_FUNCTION();
		
		void *addr = (void*)GetProcAddress((HINSTANCE)handle, name);
		return addr;
	}
	
	void *DynamicLibrary::GetSymbol(const char *sname){
		SPADES_MARK_FUNCTION();
		
		void *v = GetSymbolOrNull(sname);
		if(v == NULL){
			DWORD err = GetLastError();
			SPRaise("Failed to find symbol '%s' in %s: 0x%08x", sname,
					name.c_str(), err);
		}
		return v;
	}
}


#else
#include "DynamicLibrary.h"
#include "Exception.h"
#include <dlfcn.h>
#include "Debug.h"

namespace spades {
	DynamicLibrary::DynamicLibrary(const char *fn){
		SPADES_MARK_FUNCTION();
		
		name = fn;
		handle = dlopen(fn, RTLD_LAZY);
		if(handle == NULL){
			std::string err = dlerror();
			SPRaise("Failed to dlload '%s': %s", fn, err.c_str());
		}
	}
	
	DynamicLibrary::~DynamicLibrary() {
		SPADES_MARK_FUNCTION();
		
		dlclose(handle);
	}
	
	void *DynamicLibrary::GetSymbolOrNull(const char *name){
		SPADES_MARK_FUNCTION();
		
		void *addr = dlsym(handle, name);
		return addr;
	}
	
	void *DynamicLibrary::GetSymbol(const char *sname){
		SPADES_MARK_FUNCTION();
		
		void *v = GetSymbolOrNull(sname);
		if(v == NULL){
			std::string err = dlerror();
			SPRaise("Failed to find symbol '%s' in %s: %s", sname,
					name.c_str(), err.c_str());
		}
		return v;
	}
}

#endif
