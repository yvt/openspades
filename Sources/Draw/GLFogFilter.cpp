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

#include <vector>

#include <Core/Debug.h>
#include <Core/Math.h>
#include "GLFogFilter.h"
#include "GLMapShadowRenderer.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {
		GLFogFilter::GLFogFilter(GLRenderer *renderer) : renderer(renderer) {
			lens = renderer->RegisterProgram("Shaders/PostFilters/Fog.program");
		}
		GLColorBuffer GLFogFilter::Filter(GLColorBuffer input) {
			SPADES_MARK_FUNCTION();

			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);

			static GLProgramAttribute lensPosition("positionAttribute");
			static GLProgramUniform lensShadowMapTexture("shadowMapTexture");
			static GLProgramUniform lensCoarseShadowMapTexture("coarseShadowMapTexture");
			static GLProgramUniform lensColorTexture("colorTexture");
			static GLProgramUniform lensDepthTexture("depthTexture");
			static GLProgramUniform lensFov("fov");
			static GLProgramUniform lensViewOrigin("viewOrigin");
			static GLProgramUniform lensViewAxisUp("viewAxisUp");
			static GLProgramUniform lensViewAxisSide("viewAxisSide");
			static GLProgramUniform lensViewAxisFront("viewAxisFront");
			static GLProgramUniform zNearFar("zNearFar");
			;
			static GLProgramUniform fogColor("fogColor");
			static GLProgramUniform fogDistance("fogDistance");

			dev->Enable(IGLDevice::Blend, false);

			lensPosition(lens);
			lensShadowMapTexture(lens);
			lensCoarseShadowMapTexture(lens);
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
			lensFov.SetValue(tanf(def.fovX * .5f), tanf(def.fovY * .5f));
			if (renderer->IsRenderingMirror()) {
				def.viewOrigin.z = 63.f * 2.f - def.viewOrigin.z;
				def.viewAxis[0].z = -def.viewAxis[0].z;
				def.viewAxis[1].z = -def.viewAxis[1].z;
				def.viewAxis[2].z = -def.viewAxis[2].z;
			}
			lensViewOrigin.SetValue(def.viewOrigin.x, def.viewOrigin.y, def.viewOrigin.z);
			lensViewAxisUp.SetValue(def.viewAxis[1].x, def.viewAxis[1].y, def.viewAxis[1].z);
			lensViewAxisSide.SetValue(def.viewAxis[0].x, def.viewAxis[0].y, def.viewAxis[0].z);
			lensViewAxisFront.SetValue(def.viewAxis[2].x, def.viewAxis[2].y, def.viewAxis[2].z);
			zNearFar.SetValue(def.zNear, def.zFar);

			Vector3 fogCol = renderer->GetFogColor();
			fogCol *= fogCol; // linearize
			fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);

			fogDistance.SetValue(128.f);

			lensColorTexture.SetValue(0);
			lensDepthTexture.SetValue(1);
			lensShadowMapTexture.SetValue(2);
			lensCoarseShadowMapTexture.SetValue(3);

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
			dev->ActiveTexture(3);
			dev->BindTexture(IGLDevice::Texture2D,
			                 renderer->GetMapShadowRenderer()->GetCoarseTexture());
			dev->BindFramebuffer(IGLDevice::Framebuffer, output.GetFramebuffer());
			dev->Viewport(0, 0, output.GetWidth(), output.GetHeight());
			qr.Draw();
			dev->ActiveTexture(0);
			dev->BindTexture(IGLDevice::Texture2D, 0);

			return output;
		}
	}
}