//
//  GLShadowMapShader.h
//  OpenSpades
//
//  Created by yvt on 7/26/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once


#include "GLProgramUniform.h"
#include <vector>

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLProgramManager;
		class IGLShadowMapRenderer;
		struct GLShadowMapRenderParam;
		class GLShadowMapShader{
			GLProgramUniform projectionViewMatrix;
		public:
			GLShadowMapShader();
			~GLShadowMapShader(){}
			
			static std::vector<GLShader *> RegisterShader(GLProgramManager *);
			
			static IGLShadowMapRenderer *CreateShadowMapRenderer(GLRenderer *);
			
			/** setups shadow map shader.
			 * note that this function sets the current active texture
			 * stage to the returned texture stage.
			 * @param firstTexStage first available texture stage.
			 * @return next available texture stage */
			int operator()(GLRenderer *renderer,
						   GLProgram *, int firstTexStage);
		};
	}
}