//
//  GLShadowShader.h
//  OpenSpades
//
//  Created by yvt on 7/23/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "GLProgramUniform.h"
#include <vector>

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLProgramManager;
		class GLShadowShader{
			GLProgramUniform eyeOrigin;
			GLProgramUniform eyeFront;
			GLProgramUniform mapShadowTexture;
			GLProgramUniform fogColor;
			GLProgramUniform ambientColor;
			GLProgramUniform shadowMapViewMatrix;
			GLProgramUniform shadowMapTexture1;
			GLProgramUniform shadowMapTexture2;
			GLProgramUniform shadowMapTexture3;
			GLProgramUniform shadowMapMatrix1;
			GLProgramUniform shadowMapMatrix2;
			GLProgramUniform shadowMapMatrix3;
			GLProgramUniform ambientShadowTexture;
			GLProgramUniform radiosityTextureFlat;
			GLProgramUniform radiosityTextureX;
			GLProgramUniform radiosityTextureY;
			GLProgramUniform radiosityTextureZ;
		public:
			GLShadowShader();
			~GLShadowShader(){}
			
			static std::vector<GLShader *> RegisterShader(GLProgramManager *);
			
			/** setups shadow shader.
			 * note that this function sets the current active texture
			 * stage to the returned texture stage.
			 * @param firstTexStage first available texture stage.
			 * @return next available texture stage */
			int operator()(GLRenderer *renderer,
						   GLProgram *, int firstTexStage);
		};
	}
}