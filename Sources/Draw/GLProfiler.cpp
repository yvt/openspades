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
				device->Finish();
				watch = new Stopwatch;
			}
		}
		GLProfiler::~GLProfiler() {
			if(r_debugTiming) {
				SPAssert(levels.back() == this);
				levels.pop_back();
				
				timeNoFinish = watch->GetTime();
				device->Finish();
				time = watch->GetTime();
				std::string out = GetProfileMessage();
				
				if(!levels.empty()) {
					GLProfiler *parent = levels.back();
					parent->msg += out;
				}else {
					static int th = 0;
					SPLog("Renderer Profile [%8d]\n%s",
						  ++th, out.c_str());
					
				}
			}
		}
		std::string GLProfiler::GetProfileMessage() {
			char buf[4096];
			int indent = levels.size() * 2;
			for(int i = 0; i < indent; i++)
				buf[i] = ' ';
			sprintf(buf + indent, "%s - %.3fms (%.3fms w/o glFinish)\n",	name.c_str(),
					time * 1000.,
					timeNoFinish * 1000.);
			delete watch;
			
			std::string out = buf + msg;
			return out;
		}
	}
}
