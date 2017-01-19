/*
 Copyright (c) 2016 yvt

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

#include <cmath>
#include <vector>

#include "GLProfiler.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "GLSSAOFilter.h"
#include "GLImage.h"
#include "IGLDevice.h"
#include <Core/Debug.h>
#include <Core/Math.h>

namespace spades {
	namespace draw {

		GLSSAOFilter::GLSSAOFilter(GLRenderer *renderer)
		    : renderer(renderer), settings(renderer->GetSettings()) {
			ssaoProgram = renderer->RegisterProgram("Shaders/PostFilters/SSAO.program");
			bilateralProgram =
			  renderer->RegisterProgram("Shaders/PostFilters/BilateralFilter.program");

			ditherPattern = static_cast<GLImage *>(renderer->RegisterImage("Gfx/DitherPattern4x4.png"));
		}

		GLColorBuffer GLSSAOFilter::GenerateRawSSAOImage(int width, int height) {
			SPADES_MARK_FUNCTION();
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);

			GLColorBuffer output =
			  renderer->GetFramebufferManager()->CreateBufferHandle(width, height, 1);

			{
				GLProgram *program = ssaoProgram;
				static GLProgramAttribute positionAttribute("positionAttribute");
				static GLProgramUniform depthTexture("depthTexture");
				static GLProgramUniform ditherTexture("ditherTexture");
				static GLProgramUniform texCoordRange("texCoordRange");
				static GLProgramUniform zNearFar("zNearFar");
				static GLProgramUniform pixelShift("pixelShift");
				static GLProgramUniform fieldOfView("fieldOfView");
				static GLProgramUniform sampleOffsetScale("sampleOffsetScale");

				positionAttribute(program);
				depthTexture(program);
				ditherTexture(program);
				texCoordRange(program);
				zNearFar(program);
				pixelShift(program);
				fieldOfView(program);
				sampleOffsetScale(program);

				program->Use();

				const client::SceneDefinition &def = renderer->GetSceneDef();
				zNearFar.SetValue(def.zNear, def.zFar);

				fieldOfView.SetValue(std::tan(def.fovX * 0.5f), std::tan(def.fovY * 0.5f));

				pixelShift.SetValue(1.f / (float)width, 1.f / (float)height);

				float kernelSize = std::max(1.0f, std::min(width, height) * 0.0018f);
				sampleOffsetScale.SetValue(kernelSize / (float)width, kernelSize / (float)height);

				if (width < dev->ScreenWidth()) {
					// 2x downsampling
					texCoordRange.SetValue(0.25f / width, 0.25f / height, 1.f, 1.f);
				} else {
					texCoordRange.SetValue(0.f, 0.f, 1.f, 1.f);
				}

				dev->ActiveTexture(0);
				depthTexture.SetValue(0);
				dev->BindTexture(IGLDevice::Texture2D,
				                 renderer->GetFramebufferManager()->GetDepthTexture());

				dev->ActiveTexture(1);
				ditherTexture.SetValue(1);
				ditherPattern->Bind(IGLDevice::Texture2D);
				dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter, IGLDevice::Nearest);
				dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter, IGLDevice::Nearest);

				dev->BindFramebuffer(IGLDevice::Framebuffer, output.GetFramebuffer());
				dev->Viewport(0, 0, width, height);
				qr.SetCoordAttributeIndex(positionAttribute());
				qr.Draw();
				dev->ActiveTexture(0);
				dev->BindTexture(IGLDevice::Texture2D, 0);
			}

			return output;
		}

		GLColorBuffer GLSSAOFilter::ApplyBilateralFilter(GLColorBuffer tex, bool direction,
		                                                 int width, int height) {
			SPADES_MARK_FUNCTION();
			// do gaussian blur
			GLProgram *program = bilateralProgram;
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);

			int w = width == -1 ? tex.GetWidth() : width;
			int h = height == -1 ? tex.GetHeight() : height;

			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramUniform inputTexture("inputTexture");
			static GLProgramUniform depthTexture("depthTexture");
			static GLProgramUniform texCoordRange("texCoordRange");
			static GLProgramUniform unitShift("unitShift");
			static GLProgramUniform zNearFar("zNearFar");
			static GLProgramUniform isUpsampling("isUpsampling");
			static GLProgramUniform pixelShift("pixelShift");
			program->Use();

			positionAttribute(program);
			inputTexture(program);
			depthTexture(program);
			texCoordRange(program);
			unitShift(program);
			zNearFar(program);
			isUpsampling(program);
			pixelShift(program);

			inputTexture.SetValue(0);
			dev->ActiveTexture(0);
			dev->BindTexture(IGLDevice::Texture2D, tex.GetTexture());
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter, IGLDevice::Nearest);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter, IGLDevice::Nearest);

			depthTexture.SetValue(1);
			dev->ActiveTexture(1);
			dev->BindTexture(IGLDevice::Texture2D,
							 renderer->GetFramebufferManager()->GetDepthTexture());

			texCoordRange.SetValue(0.f, 0.f, 1.f, 1.f);

			unitShift.SetValue(direction ? 1.f / (float)width : 0.0f,
			                   direction ? 0.0f : 1.f / (float)height);

			pixelShift.SetValue(1.f / (float)width, 1.f / (float)height, (float)width, (float)height);

			isUpsampling.SetValue(width > tex.GetWidth() ? 1 : 0);

			const client::SceneDefinition &def = renderer->GetSceneDef();
			zNearFar.SetValue(def.zNear, def.zFar);

			qr.SetCoordAttributeIndex(positionAttribute());

			GLColorBuffer buf2 = renderer->GetFramebufferManager()->CreateBufferHandle(w, h, 1);
			dev->Viewport(0, 0, w, h);
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
			qr.Draw();

			dev->ActiveTexture(0);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter, IGLDevice::Linear);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter, IGLDevice::Linear);

			return buf2;
		}

		GLColorBuffer GLSSAOFilter::Filter() {
			SPADES_MARK_FUNCTION();

			IGLDevice *dev = renderer->GetGLDevice();

			int width = dev->ScreenWidth();
			int height = dev->ScreenHeight();

			dev->Enable(IGLDevice::Blend, false);

			bool useLowQualitySSAO = renderer->IsRenderingMirror() || renderer->GetSettings().r_ssao >= 2;
			GLColorBuffer ssao = useLowQualitySSAO ?
				GenerateRawSSAOImage((width + 1) / 2, (height + 1) / 2) :
			GenerateRawSSAOImage(width, height);
			ssao = ApplyBilateralFilter(ssao, false, width, height);
			ssao = ApplyBilateralFilter(ssao, true, width, height);
			if (!renderer->IsRenderingMirror()) {
				ssao = ApplyBilateralFilter(ssao, false, width, height);
				ssao = ApplyBilateralFilter(ssao, true, width, height);
			}

			return ssao;
		}
	}
}
