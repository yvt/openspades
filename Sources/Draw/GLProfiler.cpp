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
