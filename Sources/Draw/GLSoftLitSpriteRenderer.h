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

#include <Core/Math.h>
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "IGLSpriteRenderer.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class IGLDevice;
		class GLImage;
		class GLSettings;
		class GLSoftLitSpriteRenderer : public IGLSpriteRenderer {
			struct Sprite {
				GLImage *image;
				Vector3 center;
				float radius;
				float angle;
				Vector4 color;
				Vector3 emission;
				float area;
				Vector4 dlR, dlG, dlB;
			};

			struct Vertex {
				// center position
				float x, y, z;
				float radius;

				// point coord
				float sx, sy;
				float angle;

				// color
				Vector4 color;
				Vector3 emission;

				// dynamic lights
				Vector4 dlR, dlG, dlB;
			};

			GLRenderer *renderer;
			GLSettings &settings;
			IGLDevice *device;
			std::vector<Sprite> sprites;

			GLImage *lastImage;

			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			GLProgram *program;
			GLProgramUniform projectionViewMatrix;
			GLProgramUniform rightVector;
			GLProgramUniform upVector;
			GLProgramUniform frontVector;
			GLProgramUniform viewOriginVector;
			GLProgramUniform texture;
			GLProgramUniform depthTexture;
			GLProgramUniform viewMatrix;
			GLProgramUniform fogDistance;
			GLProgramUniform fogColor;
			GLProgramUniform zNearFar;
			GLProgramUniform cameraPosition;

			GLProgramAttribute positionAttribute;
			GLProgramAttribute spritePosAttribute;
			GLProgramAttribute colorAttribute;
			GLProgramAttribute emissionAttribute;
			GLProgramAttribute dlRAttribute;
			GLProgramAttribute dlGAttribute;
			GLProgramAttribute dlBAttribute;

			float thresLow, thresRange;

			void Flush();
			float LayerForSprite(const Sprite &);

		public:
			GLSoftLitSpriteRenderer(GLRenderer *);
			~GLSoftLitSpriteRenderer();

			void Add(GLImage *img, Vector3 center, float rad, float ang, Vector4 color) override;
			void Clear() override;
			void Render() override;
		};
	}
}
