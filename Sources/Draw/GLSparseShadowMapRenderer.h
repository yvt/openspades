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

#include <cstdint>

#include "IGLDevice.h"
#include "IGLShadowMapRenderer.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLShadowShader;
		class GLSettings;
		class GLSparseShadowMapRenderer : public IGLShadowMapRenderer {
			friend class GLShadowMapShader;
			friend class GLShadowShader;

			struct ModelRenderer;
			struct Internal;

			enum { Tiles = 64 };

			IGLDevice &device;
			GLSettings &settings;

			int textureSize;
			int minLod;
			int maxLod;

			uint32_t pagetable[Tiles][Tiles];

			IGLDevice::UInteger framebuffer;
			IGLDevice::UInteger texture;
			IGLDevice::UInteger pagetableTexture;

			// not used, but required
			IGLDevice::UInteger colorTexture;

			Matrix4 matrix;
			OBB3 obb;
			float vpWidth, vpHeight; // used for culling

			void BuildMatrix(float near, float far);

		protected:
			void RenderShadowMapPass() override;

		public:
			GLSparseShadowMapRenderer(GLRenderer &);
			~GLSparseShadowMapRenderer();
			void Render() override;

			bool Cull(const AABB3 &) override;
			bool SphereCull(const Vector3 &center, float rad) override;
		};
	} // namespace draw
} // namespace spades
