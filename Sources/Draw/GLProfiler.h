//
//  GLProfiler.h
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/25/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <vector>
#include <string>

namespace spades {
	
	class Stopwatch;
	namespace draw {
		class IGLDevice;
		class GLProfiler {
			std::string msg;
			std::string name;
			Stopwatch *watch;
			IGLDevice *device;
		public:
			static void ResetLevel();
			GLProfiler(IGLDevice *, const char *format, ...);
			std::string GetProfileMessage();
			~GLProfiler();
		};
	}
}

