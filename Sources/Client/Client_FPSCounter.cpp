/*
 Copyright (c) 2013 yvt
 based on code of pysnip (c) Mathias Kaerlev 2011-2012.

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

#include "Client.h"

#include <Core/Stopwatch.h>

namespace spades {
	namespace client {

		Client::FPSCounter::FPSCounter() : numFrames(0), lastFps(0.0) { sw.Reset(); }

		void Client::FPSCounter::MarkFrame() {
			numFrames++;
			if (sw.GetTime() > 0.5) {
				auto diff = sw.GetTime();
				lastFps = static_cast<double>(numFrames) / diff;
				numFrames = 0;
				sw.Reset();
			}
		}
	}
}
