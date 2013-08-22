//
//  GLFXAAFilter.cpp
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//


#include "GLFXAAFilter.h"
#include "IGLDevice.h"
#include "../Core/Math.h"
#include <vector>
#include "GLQuadRenderer.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLRenderer.h"
#include "../Core/Debug.h"

namespace spades {
	namespace draw {
		GLFXAAFilter::GLFXAAFilter(GLRenderer *renderer):
		renderer(renderer){
			
		}
		GLColorBuffer GLFXAAFilter::Filter(GLColorBuffer input) {
			SPADES_MARK_FUNCTION();
			
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			
			GLProgram *lens = renderer->RegisterProgram("Shaders/PostFilters/FXAA.program");
			static GLProgramAttribute lensPosition("positionAttribute");
			static GLProgramUniform lensTexture("texture");
			static GLProgramUniform inverseVP("inverseVP");
			
			dev->Enable(IGLDevice::Blend, false);
			
			lensPosition(lens);
			lensTexture(lens);
			inverseVP(lens);
			
			lens->Use();
			
			inverseVP.SetValue(1.f / dev->ScreenWidth(),
							 1.f / dev->ScreenHeight());
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