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
		class GLRenderer;
		class GLProgram;
		class GLSettings;
		class GLDepthOfFieldFilter {
			GLRenderer &renderer;
			GLSettings &settings;
			GLProgram *cocGen;       // program to generate CoC radius
			GLProgram *cocMix;       // program to mix CoC radius
			GLProgram *gaussProgram; // program to blur CoC radius
			GLProgram *gammaMix;
			GLProgram *passthrough;
			GLProgram *blurProgram; // program to mix CoC radius
			GLProgram *finalMix;    // program to mix CoC radius

			GLColorBuffer GenerateCoC(float blurDepthRange, float vignetteBlur, float globalBlur,
			                          float nearBlur, float farBlur);
			GLColorBuffer BlurCoC(GLColorBuffer, float spread);
			GLColorBuffer Blur(GLColorBuffer, GLColorBuffer coc, Vector2 offset, int divide = 1);
			GLColorBuffer AddMix(GLColorBuffer, GLColorBuffer);
			GLColorBuffer FinalMix(GLColorBuffer tex, GLColorBuffer blur1, GLColorBuffer blur2,
			                       GLColorBuffer coc);
			GLColorBuffer UnderSample(GLColorBuffer);
			bool HighQualityDoFEnabled();

		public:
			GLDepthOfFieldFilter(GLRenderer &);
			GLColorBuffer Filter(GLColorBuffer, float blurDepthRange, float vignetteBlur,
			                     float globalBlur, float nearBlur, float farBlur);
		};
	} // namespace draw
} // namespace spades
