//
//  GLLensFilter.cpp
//  OpenSpades
//
//  Created by yvt on 7/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//


#include "GLLensFilter.h"
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
		GLLensFilter::GLLensFilter(GLRenderer *renderer):
		renderer(renderer){
			
		}
		GLColorBuffer GLLensFilter::Filter(GLColorBuffer input) {
			SPADES_MARK_FUNCTION();
			
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			
			GLProgram *lens = renderer->RegisterProgram("Shaders/PostFilters/Lens.program");
			static GLProgramAttribute lensPosition("positionAttribute");
			static GLProgramUniform lensTexture("texture");
			static GLProgramUniform lensFov("fov");
			
			dev->Enable(IGLDevice::Blend, false);
			
			lensPosition(lens);
			lensTexture(lens);
			lensFov(lens);
			
			lens->Use();
			
			client::SceneDefinition def = renderer->GetSceneDef();
			lensFov.SetValue(tanf(def.fovX * .5f),
							 tanf(def.fovY * .5f));
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


