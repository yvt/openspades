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

#include <vector>

#include <Client/IRenderer.h>
#include <Core/Math.h>
#include "GLDynamicLight.h"
#include "GLProgramUniform.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLImage;
		class GLProgramManager;
		class GLDynamicLightShader {
			GLRenderer *lastRenderer;
			GLImage *whiteImage;

			GLProgramUniform dynamicLightOrigin;
			GLProgramUniform dynamicLightColor;
			GLProgramUniform dynamicLightRadius;
			GLProgramUniform dynamicLightRadiusInversed;
			GLProgramUniform dynamicLightSpotMatrix;
			GLProgramUniform dynamicLightProjectionTexture;

		public:
			GLDynamicLightShader();
			~GLDynamicLightShader() {}

			static std::vector<GLShader *> RegisterShader(GLProgramManager *);

			static bool Cull(const GLDynamicLight &light, const AABB3 &);

			static bool SphereCull(const GLDynamicLight &light, const Vector3 &center,
			                       float radius);

			/** setups shadow shader.
			 * note that this function sets the current active texture
			 * stage to the returned texture stage.
			 * @param firstTexStage first available texture stage.
			 * @return next available texture stage */
			int operator()(GLRenderer *renderer, GLProgram *, const GLDynamicLight &light,
			               int firstTexStage);
		};
	}
}
