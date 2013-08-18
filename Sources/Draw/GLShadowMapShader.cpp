//
//  GLShadowMapShader.cpp
//  OpenSpades
//
//  Created by yvt on 7/26/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLShadowMapShader.h"
#include "GLRenderer.h"
#include "GLProgramManager.h"
#include "GLMapShadowRenderer.h"
#include "../Core/Settings.h"
#include "GLBasicShadowMapRenderer.h"
#include "../Core/Debug.h"

SPADES_SETTING(r_modelShadows, "1");

namespace spades {
	namespace draw {
		GLShadowMapShader::GLShadowMapShader():
		projectionViewMatrix("projectionViewMatrix")
		{}
		
		std::vector<GLShader *> GLShadowMapShader::RegisterShader(spades::draw::GLProgramManager *r) {
			SPADES_MARK_FUNCTION();
			std::vector<GLShader *>  shaders;
			
			// even there is no dynamic shadow,
			// this is still needed to avoid error...
			
			shaders.push_back(r->RegisterShader("Shaders/ShadowMap/Common.fs"));
			shaders.push_back(r->RegisterShader("Shaders/ShadowMap/Common.vs"));
			
			shaders.push_back(r->RegisterShader("Shaders/ShadowMap/Basic.fs"));
			shaders.push_back(r->RegisterShader("Shaders/ShadowMap/Basic.vs"));
			
			return shaders;
		}
		
		IGLShadowMapRenderer * GLShadowMapShader::CreateShadowMapRenderer(spades::draw::GLRenderer *r){
			SPADES_MARK_FUNCTION();
			if(!r_modelShadows)
				return NULL;
			return new GLBasicShadowMapRenderer(r);
		}
		
		int GLShadowMapShader::operator()(GLRenderer *renderer,
									   spades::draw::GLProgram *program, int texStage) {
			
			
			IGLDevice *dev = program->GetDevice();
			
			GLBasicShadowMapRenderer *r = static_cast<GLBasicShadowMapRenderer *>(renderer->GetShadowMapRenderer());
			
			projectionViewMatrix(program);
			projectionViewMatrix.SetValue(r->matrix);
			
			dev->ActiveTexture(texStage);
			
			return texStage;
		}
	}
}