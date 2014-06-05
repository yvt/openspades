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

#include "GLFogShader.h"
#include "GLRenderer.h"
#include "GLProgramManager.h"
#include "GLMapShadowRenderer.h"
#include "../Core/Settings.h"
#include "GLBasicShadowMapRenderer.h"
#include "GLSparseShadowMapRenderer.h"
#include "../Core/Debug.h"

namespace spades {
	namespace draw {
		GLFogShader::GLFogShader():
		fogCoefficient("fogCoefficient"),
		useExponentalFog("useExponentalFog")
		{}
		
		int GLFogShader::operator()(GLRenderer *renderer,
										  spades::draw::GLProgram *program, int texStage) {
			
			
			IGLDevice *dev = program->GetDevice();
			
			fogCoefficient(program);
			useExponentalFog(program);
			
			float dist = renderer->GetFogDistance();
			auto type = renderer->GetFogType();
			fogCoefficient.SetValue(1.f / dist / dist);
			switch (type) {
				case client::FogType::Classical:
					useExponentalFog.SetValue(0);
					break;
				case client::FogType::Exponential:
					useExponentalFog.SetValue(1);
					break;
				default:
					SPAssert(false);
					break;
			}
			
			dev->ActiveTexture(texStage);
			
			return texStage;
		}
	}
}
