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

#pragma once

#include "GLFramebufferManager.h"

namespace spades {
	namespace draw {
		class GLImage;
		class GLRenderer;
		class GLProgram;
		class GLLensFlareFilter {
			GLRenderer &renderer;
			GLProgram *blurProgram, *scannerProgram, *drawProgram;
			Handle<GLImage> flare1, flare2, flare3, flare4, white;
			Handle<GLImage> mask1, mask2, mask3;
			GLColorBuffer Blur(GLColorBuffer, float spread = 1.f);

		public:
			GLLensFlareFilter(GLRenderer &);
			void Draw();
			void Draw(Vector3 direction, bool reflections, Vector3 color, bool infinityDistance);
		};
	} // namespace draw
} // namespace spades
