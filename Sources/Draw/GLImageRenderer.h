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

#include <cstdint>

#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"

namespace spades {
	namespace draw {
		class GLImage;
		class IGLDevice;
		class GLRenderer;
		class GLImageRenderer {
			GLRenderer *renderer;
			IGLDevice *device;
			GLImage *image;

			float invScreenWidthFactored;
			float invScreenHeightFactored;

			GLProgram *program;

			GLProgramAttribute *positionAttribute;
			GLProgramAttribute *colorAttribute;
			GLProgramAttribute *textureCoordAttribute;

			GLProgramUniform *screenSize;
			GLProgramUniform *textureSize;
			GLProgramUniform *texture;

			struct ImageVertex {
				float x, y, u, v;
				float r, g, b, a;
			};

			std::vector<ImageVertex> vertices;
			std::vector<uint32_t> indices;

		public:
			GLImageRenderer(GLRenderer *renderer);
			~GLImageRenderer();

			void Flush();

			void SetImage(GLImage *);

			void Add(float dx1, float dy1, float dx2, float dy2, float dx3, float dy3, float dx4,
			         float dy4, float sx1, float sy1, float sx2, float sy2, float sx3, float sy3,
			         float sx4, float sy4, float r, float g, float b, float a);
		};
	}
}