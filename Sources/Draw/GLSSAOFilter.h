/*
 Copyright (c) 2016 yvt

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

#pragma once

#include "GLFramebufferManager.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLProgram;
		class GLImage;
		class GLSettings;
		class GLSSAOFilter {
			GLRenderer &renderer;
			GLSettings &settings;
			GLProgram *ssaoProgram;
			GLProgram *bilateralProgram;

			Handle<GLImage> ditherPattern;

			GLColorBuffer GenerateRawSSAOImage(int width, int height);
			GLColorBuffer ApplyBilateralFilter(GLColorBuffer, bool direction, int width = -1,
			                                   int height = -1);

		public:
			GLSSAOFilter(GLRenderer &);
			GLColorBuffer Filter();
		};
	} // namespace draw
} // namespace spades
