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

#include "GLShadowMapShader.h"
#include <Core/Debug.h>
#include <Core/Settings.h>
#include "GLBasicShadowMapRenderer.h"
#include "GLMapShadowRenderer.h"
#include "GLProgramManager.h"
#include "GLRenderer.h"
#include "GLSparseShadowMapRenderer.h"

namespace spades {
	namespace draw {
		GLShadowMapShader::GLShadowMapShader() : projectionViewMatrix("projectionViewMatrix") {}

		std::vector<GLShader *>
		GLShadowMapShader::RegisterShader(spades::draw::GLProgramManager *r) {
			SPADES_MARK_FUNCTION();
			std::vector<GLShader *> shaders;

			// even there is no dynamic shadow,
			// this is still needed to avoid error...

			shaders.push_back(r->RegisterShader("Shaders/ShadowMap/Common.fs"));
			shaders.push_back(r->RegisterShader("Shaders/ShadowMap/Common.vs"));

			shaders.push_back(r->RegisterShader("Shaders/ShadowMap/Basic.fs"));
			shaders.push_back(r->RegisterShader("Shaders/ShadowMap/Basic.vs"));

			return shaders;
		}

		IGLShadowMapRenderer *
		GLShadowMapShader::CreateShadowMapRenderer(spades::draw::GLRenderer *r) {
			SPADES_MARK_FUNCTION();
			auto &settings = r->GetSettings();
			if (!settings.r_modelShadows)
				return NULL;
			if (settings.r_sparseShadowMaps)
				return new GLSparseShadowMapRenderer(r);
			return new GLBasicShadowMapRenderer(r);
		}

		int GLShadowMapShader::operator()(GLRenderer *renderer, spades::draw::GLProgram *program,
		                                  int texStage) {

			IGLDevice *dev = program->GetDevice();
			auto &settings = renderer->GetSettings();

			if (settings.r_sparseShadowMaps) {
				GLSparseShadowMapRenderer *r =
				  static_cast<GLSparseShadowMapRenderer *>(renderer->GetShadowMapRenderer());

				projectionViewMatrix(program);
				projectionViewMatrix.SetValue(r->matrix);
			} else {
				GLBasicShadowMapRenderer *r =
				  static_cast<GLBasicShadowMapRenderer *>(renderer->GetShadowMapRenderer());

				projectionViewMatrix(program);
				projectionViewMatrix.SetValue(r->matrix);
			}

			dev->ActiveTexture(texStage);

			return texStage;
		}
	}
}
