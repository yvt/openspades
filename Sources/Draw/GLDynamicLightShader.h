//
//  GLLightRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/25/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//


#pragma once

#include "GLProgramUniform.h"
#include <vector>
#include "../Client/IRenderer.h"
#include "../Core/Math.h"
#include "GLDynamicLight.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLImage;
		class GLProgramManager;
		class GLDynamicLightShader{
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
			~GLDynamicLightShader(){}
			
			static std::vector<GLShader *> RegisterShader(GLProgramManager *);
			
			static bool Cull(const GLDynamicLight& light,
							 const AABB3&);
			
			static bool SphereCull(const GLDynamicLight& light,
							 const Vector3& center,
								   float radius);
			
			/** setups shadow shader.
			 * note that this function sets the current active texture
			 * stage to the returned texture stage.
			 * @param firstTexStage first available texture stage.
			 * @return next available texture stage */
			int operator()(GLRenderer *renderer,
						   GLProgram *,
						   const GLDynamicLight& light,
						   int firstTexStage);
		};
	}
}
