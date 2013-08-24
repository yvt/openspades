//
//  GLProfiler.cpp
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/25/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLProfiler.h"
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include "../Core/Stopwatch.h"
#include "../Core/Settings.h"
#include "../Core/Debug.h"
#include "IGLDevice.h"

SPADES_SETTING(r_debugTiming, "0");

namespace spades {
	namespace draw {
		static std::vector<GLProfiler *> levels;
		void GLProfiler::ResetLevel() {
			levels.clear();
		}
		GLProfiler::GLProfiler(IGLDevice *device, const char *format, ...) {
			if(r_debugTiming) {
				levels.push_back(this);
				
				char buf[2048];
				va_list va;
				va_start(va, format);
				vsprintf(buf, format, va);
				va_end(va);
				
				name = buf;
				
				this->device = device;
				watch = new Stopwatch;
			}
		}
		GLProfiler::~GLProfiler() {
			if(r_debugTiming) {
				SPAssert(levels.back() == this);
				levels.pop_back();
				
				device->Finish();
				std::string out = GetProfileMessage();
				
				if(!levels.empty()) {
					GLProfiler *parent = levels.back();
					parent->msg += out;
				}else {
					static int th = 0;
					printf("----- Renderer Profile [%8d] -----\n",
						   ++th);
					puts(out.c_str());
				}
			}
		}
		std::string GLProfiler::GetProfileMessage() {
			char buf[4096];
			int indent = levels.size() * 2;
			for(int i = 0; i < indent; i++)
				buf[i] = ' ';
			sprintf(buf + indent, "%s - %.3fms\n", name.c_str(),
					watch->GetTime() * 1000.);
			delete watch;
			
			std::string out = buf + msg;
			return out;
		}
	}
}
