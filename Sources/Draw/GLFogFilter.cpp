//
//  GLFogFilter.cpp
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//


#include "GLFogFilter.h"
#include "IGLDevice.h"
#include "../Core/Math.h"
#include <vector>
#include "GLQuadRenderer.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLRenderer.h"
#include "../Core/Debug.h"
#include "GLMapShadowRenderer.h"

namespace spades {
	namespace draw {
		GLFogFilter::GLFogFilter(GLRenderer *renderer):
		renderer(renderer){
			
		}
		GLColorBuffer GLFogFilter::Filter(GLColorBuffer input) {
			SPADES_MARK_FUNCTION();
			
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			
			GLProgram *lens = renderer->RegisterProgram("Shaders/PostFilters/Fog.program");
			static GLProgramAttribute lensPosition("positionAttribute");
			static GLProgramUniform lensShadowMapTexture("shadowMapTexture");
			static GLProgramUniform lensColorTexture("colorTexture");
			static GLProgramUniform lensDepthTexture("depthTexture");
			static GLProgramUniform lensFov("fov");
			static GLProgramUniform lensViewOrigin("viewOrigin");
			static GLProgramUniform lensViewAxisUp("viewAxisUp");
			static GLProgramUniform lensViewAxisSide("viewAxisSide");
			static GLProgramUniform lensViewAxisFront("viewAxisFront");
			static GLProgramUniform zNearFar("zNearFar");;
			static GLProgramUniform fogColor("fogColor");
			static GLProgramUniform fogDistance("fogDistance");
			
			dev->Enable(IGLDevice::Blend, false);
			
			lensPosition(lens);
			lensShadowMapTexture(lens);
			lensColorTexture(lens);
			lensDepthTexture(lens);
			lensFov(lens);
			lensViewOrigin(lens);
			lensViewAxisUp(lens);
			lensViewAxisSide(lens);
			lensViewAxisFront(lens);
			zNearFar(lens);
			fogColor(lens);
			fogDistance(lens);
			
			lens->Use();
			
			client::SceneDefinition def = renderer->GetSceneDef();
			lensFov.SetValue(tanf(def.fovX * .5f),
							 tanf(def.fovY * .5f));
			lensViewOrigin.SetValue(def.viewOrigin.x,
									def.viewOrigin.y,
									def.viewOrigin.z);
			lensViewAxisUp.SetValue(def.viewAxis[1].x,
									def.viewAxis[1].y,
									def.viewAxis[1].z);
			lensViewAxisSide.SetValue(def.viewAxis[0].x,
									def.viewAxis[0].y,
									  def.viewAxis[0].z);
			lensViewAxisFront.SetValue(def.viewAxis[2].x,
									  def.viewAxis[2].y,
									   def.viewAxis[2].z);
			zNearFar.SetValue(def.zNear, def.zFar);
			
			
			Vector3 fogCol = renderer->GetFogColor();
			fogCol *= fogCol; // linearize
			fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);
			
			fogDistance.SetValue(128.f);
			
			lensColorTexture.SetValue(0);
			lensDepthTexture.SetValue(1);
			lensShadowMapTexture.SetValue(2);
			
			// composite to the final image
			GLColorBuffer output = input.GetManager()->CreateBufferHandle();
			
			
			dev->Enable(IGLDevice::Blend, false);
			qr.SetCoordAttributeIndex(lensPosition());
			dev->ActiveTexture(0);
			dev->BindTexture(IGLDevice::Texture2D, input.GetTexture());
			dev->ActiveTexture(1);
			dev->BindTexture(IGLDevice::Texture2D, input.GetManager()->GetDepthTexture());
			dev->ActiveTexture(2);
			dev->BindTexture(IGLDevice::Texture2D, renderer->GetMapShadowRenderer()->GetTexture());
			dev->BindFramebuffer(IGLDevice::Framebuffer, output.GetFramebuffer());
			dev->Viewport(0, 0, output.GetWidth(), output.GetHeight());
			qr.Draw();
			dev->ActiveTexture(0);
			dev->BindTexture(IGLDevice::Texture2D, 0);
			
			
			return output;
		}
	}
}