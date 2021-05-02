/*
 Copyright (c) 2021 yvt

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

#include "GLFramebufferManager.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLProgram;
		class GLImage;
		class GLFogFilter2 {
			GLRenderer &renderer;
			GLProgram *lens;
			Handle<GLImage> ditherPattern;
			IGLDevice::UInteger noiseTex;
			/**
			 * The recorded value of `GLRenderer::GetFrameNumber()` of when `noiseTex` was updated
			 * last time.
			 */
			std::uint32_t lastNoiseTexFrameNumber = 0xffffffff;

		public:
			GLFogFilter2(GLRenderer &);
			~GLFogFilter2();
			GLColorBuffer Filter(GLColorBuffer);
		};
	} // namespace draw
} // namespace spades
