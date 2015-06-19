/*
 Copyright (c) 2013 OpenSpades Developers
 
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


#include "GLColorCorrectionFilter.h"
#include "IGLDevice.h"
#include "../Core/Math.h"
#include <vector>
#include "GLQuadRenderer.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLRenderer.h"
#include "../Core/Debug.h"
#include <Core/Settings.h>

SPADES_SETTING(r_bloom, "");

namespace spades {
	namespace draw {
		GLColorCorrectionFilter::GLColorCorrectionFilter(GLRenderer *renderer):
		renderer(renderer){
			lens = renderer->RegisterProgram("Shaders/PostFilters/ColorCorrection.program");
		}
		GLColorBuffer GLColorCorrectionFilter::Filter(GLColorBuffer input, Vector3 tintVal) {
			SPADES_MARK_FUNCTION();
			
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			
			static GLProgramAttribute lensPosition("positionAttribute");
			static GLProgramUniform lensTexture("texture");
			
			static GLProgramUniform saturation("saturation");
			static GLProgramUniform enhancement("enhancement");
			static GLProgramUniform tint("tint");
			
			saturation(lens);
			enhancement(lens);
			tint(lens);
			
			dev->Enable(IGLDevice::Blend, false);
			
			lensPosition(lens);
			lensTexture(lens);
			
			lens->Use();
			
			tint.SetValue(tintVal.x, tintVal.y, tintVal.z);
			
			const client::SceneDefinition& def  = renderer->GetSceneDef();
			
			if(r_bloom) {
				// make image sharper
				saturation.SetValue(.85f * def.saturation);
				enhancement.SetValue(0.7f);
			}else{
				saturation.SetValue(1.f * def.saturation);
				enhancement.SetValue(0.3f);
			}
			
			lensTexture.SetValue(0);
			
			// composite to the final image
			GLColorBuffer output = input.GetManager()->CreateBufferHandle();
			
			
			qr.SetCoordAttributeIndex(lensPosition());
			dev->BindTexture(IGLDevice::Texture2D, input.GetTexture());
			dev->BindFramebuffer(IGLDevice::Framebuffer, output.GetFramebuffer());
			dev->Viewport(0, 0, output.GetWidth(), output.GetHeight());
			qr.Draw();
			dev->BindTexture(IGLDevice::Texture2D, 0);
			
			
			return output;
		}
	}
}


