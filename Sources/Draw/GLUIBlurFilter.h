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

#include <cstdint>
#include <cstdint>
#include <vector>

#include "GLFramebufferManager.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLProgram;
		class GLImage;
		class GLUIBlurFilter {
			GLProgram *thru;
			GLProgram *gauss1d;
			GLRenderer *renderer;

			struct {
				int x1, y1, x2, y2;
			} workArea;

			GLColorBuffer DownSample(GLColorBuffer);
			GLColorBuffer GaussianBlur(GLColorBuffer, bool vertical);
			void UpdateScissorRect(int framebufferWidth, int framebufferHeight);

		public:
			GLUIBlurFilter(GLRenderer *);
			~GLUIBlurFilter();

			/**
			 * Applies the blur filter to the specified rectangular region in a given framebuffer.
			 *
			 *  - Assumes the scissor test is enabled.
			 *  - Disables blending.
			 *  - Clobbers the scissor rectangle.
			 */
			void Apply(IGLDevice::UInteger framebuffer, const AABB2 &target);
		};
	}
}
