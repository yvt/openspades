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
#include <vector>

#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include <Core/Math.h>

namespace spades {
	namespace draw {
		class GLRenderer;
		class IGLDevice;
		class GLImage;
		class GLSettings;
		class GLLongSpriteRenderer {
			struct Sprite {
				GLImage *image;
				Vector3 start;
				Vector3 end;
				float radius;
				Vector4 color;
			};

			struct Vertex {
				float x, y, z;
				float pad;
				float u, v;
				// color
				float r, g, b, a;

				void operator=(const Vector3 &vec) {
					x = vec.x;
					y = vec.y;
					z = vec.z;
				}
			};

			GLRenderer &renderer;
			IGLDevice &device;
			GLSettings &settings;
			std::vector<Sprite> sprites;

			GLImage *lastImage;

			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			GLProgram *program;
			GLProgramUniform projectionViewMatrix;
			GLProgramUniform rightVector;
			GLProgramUniform upVector;
			GLProgramUniform texture;
			GLProgramUniform viewMatrix;
			GLProgramUniform fogDistance;
			GLProgramUniform fogColor;
			GLProgramUniform viewOriginVector;

			GLProgramAttribute positionAttribute;
			GLProgramAttribute texCoordAttribute;
			GLProgramAttribute colorAttribute;

			void Flush();

		public:
			GLLongSpriteRenderer(GLRenderer &);
			~GLLongSpriteRenderer();

			void Add(GLImage *img, Vector3 p1, Vector3 p2, float rad, Vector4 color);
			void Clear();
			void Render();
		};
	} // namespace draw
} // namespace spades
