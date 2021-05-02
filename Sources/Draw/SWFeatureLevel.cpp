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

#include "SWFeatureLevel.h"
#include <Core/CpuID.h>

namespace spades {
	namespace draw {

		// TODO: correct detection

#if ENABLE_SSE2
		SWFeatureLevel DetectFeatureLevel() {
			CpuID cpuid;
			if (cpuid.Supports(CpuFeature::SSE2))
				return SWFeatureLevel::SSE2;

			// at least sse should be supported
			return SWFeatureLevel::SSE;
		}
#elif ENABLE_SSE
		SWFeatureLevel DetectFeatureLevel() { return SWFeatureLevel::SSE; }
#else
		SWFeatureLevel DetectFeatureLevel() { return SWFeatureLevel::None; }
#endif
	} // namespace draw
} // namespace spades
