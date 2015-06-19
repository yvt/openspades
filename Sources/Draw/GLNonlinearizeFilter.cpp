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


#include "GLNonlinearizeFilter.h"
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

namespace spades {
	namespace draw {
		GLNonlinearlizeFilter::GLNonlinearlizeFilter(GLRenderer *renderer):
		renderer(renderer){
			lens = renderer->RegisterProgram("Shaders/PostFilters/Nonlinearize.program");
		}
		GLColorBuffer GLNonlinearlizeFilter::Filter(GLColorBuffer input) {
			SPADES_MARK_FUNCTION();
			
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			
			static GLProgramAttribute lensPosition("positionAttribute");
			static GLProgramUniform lensTexture("texture");
			
			dev->Enable(IGLDevice::Blend, false);
			
			lensPosition(lens);
			lensTexture(lens);
			
			lens->Use();
			
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


