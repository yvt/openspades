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

#include "IGLDevice.h"
#include "IGLShadowMapRenderer.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLShadowShader;
		class GLBasicShadowMapRenderer : public IGLShadowMapRenderer {
			friend class GLShadowMapShader;
			friend class GLShadowShader;

			enum { NumSlices = 3 };

			IGLDevice &device;

			int textureSize;

			IGLDevice::UInteger framebuffer[NumSlices];
			IGLDevice::UInteger texture[NumSlices];

			// not used, but required
			IGLDevice::UInteger colorTexture;

			Matrix4 matrix;
			Matrix4 matrices[3];
			OBB3 obb;
			float vpWidth, vpHeight; // used for culling

			void BuildMatrix(float near, float far);

		public:
			GLBasicShadowMapRenderer(GLRenderer &);
			~GLBasicShadowMapRenderer();
			void Render() override;

			bool Cull(const AABB3 &) override;
			bool SphereCull(const Vector3 &center, float rad) override;
		};
	} // namespace draw
} // namespace spades
