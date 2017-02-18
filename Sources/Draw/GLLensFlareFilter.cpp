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
#include "GLImage.h"
#include "GLLensFlareFilter.h"
#include "GLMapShadowRenderer.h"
#include "GLProfiler.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {
		GLLensFlareFilter::GLLensFlareFilter(GLRenderer *renderer) : renderer(renderer) {
			blurProgram = renderer->RegisterProgram("Shaders/PostFilters/Gauss1D.program");
			scannerProgram = renderer->RegisterProgram("Shaders/LensFlare/Scanner.program");
			drawProgram = renderer->RegisterProgram("Shaders/LensFlare/Draw.program");
			flare1 = (GLImage *)renderer->RegisterImage("Gfx/LensFlare/1.png");
			flare2 = (GLImage *)renderer->RegisterImage("Gfx/LensFlare/2.png");
			flare3 = (GLImage *)renderer->RegisterImage("Gfx/LensFlare/3.png");
			flare4 = (GLImage *)renderer->RegisterImage("Gfx/LensFlare/4.jpg");
			mask1 = (GLImage *)renderer->RegisterImage("Gfx/LensFlare/mask1.png");
			mask2 = (GLImage *)renderer->RegisterImage("Gfx/LensFlare/mask2.png");
			mask3 = (GLImage *)renderer->RegisterImage("Gfx/LensFlare/mask3.png");
			white = (GLImage *)renderer->RegisterImage("Gfx/White.tga");
		}

		GLColorBuffer GLLensFlareFilter::Blur(GLColorBuffer buffer, float spread) {
			// do gaussian blur
			GLProgram *program = blurProgram;
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			int w = buffer.GetWidth();
			int h = buffer.GetHeight();

			static GLProgramAttribute blur_positionAttribute("positionAttribute");
			static GLProgramUniform blur_textureUniform("mainTexture");
			static GLProgramUniform blur_unitShift("unitShift");
			program->Use();
			blur_positionAttribute(program);
			blur_textureUniform(program);
			blur_unitShift(program);
			blur_textureUniform.SetValue(0);

			dev->ActiveTexture(0);
			qr.SetCoordAttributeIndex(blur_positionAttribute());
			dev->Enable(IGLDevice::Blend, false);

			// x-direction
			GLColorBuffer buf2 = renderer->GetFramebufferManager()->CreateBufferHandle(w, h, false);
			dev->BindTexture(IGLDevice::Texture2D, buffer.GetTexture());
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
			blur_unitShift.SetValue(spread / (float)w, 0.f);
			qr.Draw();
			buffer.Release();

			// y-direction
			GLColorBuffer buf3 = renderer->GetFramebufferManager()->CreateBufferHandle(w, h, false);
			dev->BindTexture(IGLDevice::Texture2D, buf2.GetTexture());
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf3.GetFramebuffer());
			blur_unitShift.SetValue(0.f, spread / (float)h);
			qr.Draw();
			buf2.Release();

			return buf3;
		}

		void GLLensFlareFilter::Draw() {
			Draw(MakeVector3(0.f, -1.f, -1.f), true, MakeVector3(1.f, .9f, .8f), true);
		}

		void GLLensFlareFilter::Draw(Vector3 direction, bool renderReflections, Vector3 sunColor,
		                             bool infinityDistance) {
			SPADES_MARK_FUNCTION();

			IGLDevice *dev = renderer->GetGLDevice();

			client::SceneDefinition def = renderer->GetSceneDef();

			// transform sun into NDC
			Vector3 sunWorld = direction;
			Vector3 sunView = {Vector3::Dot(sunWorld, def.viewAxis[0]),
			                   Vector3::Dot(sunWorld, def.viewAxis[1]),
			                   Vector3::Dot(sunWorld, def.viewAxis[2])};
			if (sunView.z <= 0.f) {
				return;
			}

			IGLDevice::UInteger lastFramebuffer = dev->GetInteger(IGLDevice::FramebufferBinding);

			Vector2 fov = {tanf(def.fovX * .5f), tanf(def.fovY * .5f)};
			Vector2 sunScreen;
			sunScreen.x = sunView.x / (sunView.z * fov.x);
			sunScreen.y = sunView.y / (sunView.z * fov.y);

			const float sunRadiusTan = tanf(.53f * .5f * static_cast<float>(M_PI) / 180.f);
			Vector2 sunSize = {sunRadiusTan / fov.x, sunRadiusTan / fov.y};

			GLColorBuffer visiblityBuffer =
			  renderer->GetFramebufferManager()->CreateBufferHandle(64, 64, false);

			GLQuadRenderer qr(dev);

			{
				GLProfiler::Context measure(renderer->GetGLProfiler(), "Occlusion Test");

				GLProgram *scanner = scannerProgram;
				static GLProgramAttribute positionAttribute("positionAttribute");
				static GLProgramUniform scanRange("scanRange");
				static GLProgramUniform drawRange("drawRange");
				static GLProgramUniform radius("radius");
				static GLProgramUniform depthTexture("depthTexture");
				static GLProgramUniform scanZ("scanZ");

				scanner->Use();
				positionAttribute(scanner);
				scanRange(scanner);
				drawRange(scanner);
				radius(scanner);
				depthTexture(scanner);
				scanZ(scanner);

				if (infinityDistance)
					scanZ.SetValue(.9999999f);
				else {
					float far = def.zFar;
					float near = def.zNear;
					float depth = sunView.z;
					scanZ.SetValue(far * (near - depth) / (depth * (near - far)));
				}

				dev->Enable(IGLDevice::Blend, false);

				dev->ActiveTexture(0);
				dev->BindTexture(IGLDevice::Texture2D,
				                 renderer->GetFramebufferManager()->GetDepthTexture());
				depthTexture.SetValue(0);
				dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareMode,
				                  IGLDevice::CompareRefToTexture);
				dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareFunc,
				                  IGLDevice::Less);
				dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                  IGLDevice::Linear);
				dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                  IGLDevice::Linear);

				Vector2 sunTexPos = sunScreen * .5f + .5f;
				Vector2 sunTexSize = sunSize * .5f;
				scanRange.SetValue(sunTexPos.x - sunTexSize.x, sunTexPos.y - sunTexSize.y,
				                   sunTexPos.x + sunTexSize.x, sunTexPos.y + sunTexSize.y);

				drawRange.SetValue(-.5f, -.5f, .5f, .5f);
				radius.SetValue(32.f);

				qr.SetCoordAttributeIndex(positionAttribute());
				dev->BindFramebuffer(IGLDevice::Framebuffer, visiblityBuffer.GetFramebuffer());
				dev->Viewport(0, 0, 64, 64);
				dev->ClearColor(0, 0, 0, 1);
				dev->Clear(IGLDevice::ColorBufferBit);
				qr.Draw();

				// restore depth texture's compare mode

				dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                  IGLDevice::Nearest);
				dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                  IGLDevice::Nearest);
				dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureCompareMode,
				                  IGLDevice::None);
			}

			visiblityBuffer = Blur(visiblityBuffer, 1.f);
			visiblityBuffer = Blur(visiblityBuffer, 2.f);
			visiblityBuffer = Blur(visiblityBuffer, 4.f);

			// lens flare size doesn't follow sun size
			sunSize = MakeVector2(.01f, .01f);
			sunSize.x *= renderer->ScreenHeight() / renderer->ScreenWidth();

			float aroundness = sunScreen.GetPoweredLength() * 0.6f;
			float aroundness2 = std::min(sunScreen.GetPoweredLength() * 3.2f, 1.f);

			dev->BindFramebuffer(IGLDevice::Framebuffer, lastFramebuffer);

			{
				GLProfiler::Context measure(renderer->GetGLProfiler(), "Draw");

				GLProgram *draw = drawProgram;
				static GLProgramAttribute positionAttribute("positionAttribute");
				static GLProgramUniform drawRange("drawRange");
				static GLProgramUniform color("color");
				static GLProgramUniform visibilityTexture("visibilityTexture");
				static GLProgramUniform modulationTexture("modulationTexture");
				static GLProgramUniform flareTexture("flareTexture");

				draw->Use();

				positionAttribute(draw);
				drawRange(draw);
				visibilityTexture(draw);
				modulationTexture(draw);
				flareTexture(draw);
				color(draw);

				dev->Enable(IGLDevice::Blend, true);
				dev->BlendFunc(IGLDevice::One, IGLDevice::One);

				dev->ActiveTexture(2);
				white->Bind(IGLDevice::Texture2D);
				flareTexture.SetValue(2);

				dev->ActiveTexture(1);
				white->Bind(IGLDevice::Texture2D);
				modulationTexture.SetValue(1);

				dev->ActiveTexture(0);
				dev->BindTexture(IGLDevice::Texture2D, visiblityBuffer.GetTexture());
				visibilityTexture.SetValue(0);

				qr.SetCoordAttributeIndex(positionAttribute());
				dev->Viewport(0, 0, dev->ScreenWidth(), dev->ScreenHeight());

				/* render flare */

				dev->ActiveTexture(2);
				flare4->Bind(IGLDevice::Texture2D);

				color.SetValue(sunColor.x * .04f, sunColor.y * .03f, sunColor.z * .04f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 256.f, sunScreen.y - sunSize.y * 256.f,
				                   sunScreen.x + sunSize.x * 256.f,
				                   sunScreen.y + sunSize.y * 256.f);
				qr.Draw();

				dev->ActiveTexture(2);
				white->Bind(IGLDevice::Texture2D);

				color.SetValue(sunColor.x * .3f, sunColor.y * .3f, sunColor.z * .3f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 64.f, sunScreen.y - sunSize.y * 64.f,
				                   sunScreen.x + sunSize.x * 64.f, sunScreen.y + sunSize.y * 64.f);
				qr.Draw();

				color.SetValue(sunColor.x * .5f, sunColor.y * .5f, sunColor.z * .5f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 32.f, sunScreen.y - sunSize.y * 32.f,
				                   sunScreen.x + sunSize.x * 32.f, sunScreen.y + sunSize.y * 32.f);
				qr.Draw();

				color.SetValue(sunColor.x * .8f, sunColor.y * .8f, sunColor.z * .8f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 16.f, sunScreen.y - sunSize.y * 16.f,
				                   sunScreen.x + sunSize.x * 16.f, sunScreen.y + sunSize.y * 16.f);
				qr.Draw();

				color.SetValue(sunColor.x * 1.f, sunColor.y * 1.f, sunColor.z * 1.f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 4.f, sunScreen.y - sunSize.y * 4.f,
				                   sunScreen.x + sunSize.x * 4.f, sunScreen.y + sunSize.y * 4.f);

				qr.Draw();

				color.SetValue(sunColor.x * .1f, sunColor.y * .05f, sunColor.z * .1f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 256.f, sunScreen.y - sunSize.y * 8.f,
				                   sunScreen.x + sunSize.x * 256.f, sunScreen.y + sunSize.y * 8.f);

				qr.Draw();

				/* render dusts */

				dev->ActiveTexture(1);
				mask3->Bind(IGLDevice::Texture2D);

				color.SetValue(sunColor.x * .4f * aroundness, sunColor.y * .4f * aroundness,
				               sunColor.z * .4f * aroundness);
				drawRange.SetValue(sunScreen.x - sunSize.x * 188.f, sunScreen.y - sunSize.y * 188.f,
				                   sunScreen.x + sunSize.x * 188.f,
				                   sunScreen.y + sunSize.y * 188.f);
				qr.Draw();

				if (renderReflections) {

					dev->ActiveTexture(1);
					white->Bind(IGLDevice::Texture2D);
					dev->ActiveTexture(2);
					flare2->Bind(IGLDevice::Texture2D);

					color.SetValue(sunColor.x * 1.f, sunColor.y * 1.f, sunColor.z * 1.f);
					drawRange.SetValue(-(sunScreen.x - sunSize.x * 18.f) * .4f,
					                   -(sunScreen.y - sunSize.y * 18.f) * .4f,
					                   -(sunScreen.x + sunSize.x * 18.f) * .4f,
					                   -(sunScreen.y + sunSize.y * 18.f) * .4f);

					qr.Draw();

					color.SetValue(sunColor.x * .3f, sunColor.y * .3f, sunColor.z * .3f);
					drawRange.SetValue(-(sunScreen.x - sunSize.x * 6.f) * .39f,
					                   -(sunScreen.y - sunSize.y * 6.f) * .39f,
					                   -(sunScreen.x + sunSize.x * 6.f) * .39f,
					                   -(sunScreen.y + sunSize.y * 6.f) * .39f);

					qr.Draw();

					color.SetValue(sunColor.x * 1.f, sunColor.y * 1.f, sunColor.z * 1.f);
					drawRange.SetValue(-(sunScreen.x - sunSize.x * 6.f) * .3f,
					                   -(sunScreen.y - sunSize.y * 6.f) * .3f,
					                   -(sunScreen.x + sunSize.x * 6.f) * .3f,
					                   -(sunScreen.y + sunSize.y * 6.f) * .3f);

					qr.Draw();

					color.SetValue(sunColor.x * .3f, sunColor.y * .3f, sunColor.z * .3f);
					drawRange.SetValue((sunScreen.x - sunSize.x * 12.f) * .6f,
					                   (sunScreen.y - sunSize.y * 12.f) * .6f,
					                   (sunScreen.x + sunSize.x * 12.f) * .6f,
					                   (sunScreen.y + sunSize.y * 12.f) * .6f);

					qr.Draw();

					dev->ActiveTexture(1);
					mask2->Bind(IGLDevice::Texture2D);
					dev->ActiveTexture(2);
					flare1->Bind(IGLDevice::Texture2D);

					color.SetValue(sunColor.x * .5f, sunColor.y * .4f, sunColor.z * .3f);
					drawRange.SetValue((sunScreen.x - sunSize.x * 96.f) * 2.3f,
					                   (sunScreen.y - sunSize.y * 96.f) * 2.3f,
					                   (sunScreen.x + sunSize.x * 96.f) * 2.3f,
					                   (sunScreen.y + sunSize.y * 96.f) * 2.3f);

					qr.Draw();

					color.SetValue(sunColor.x * .3f, sunColor.y * .2f, sunColor.z * .1f);
					drawRange.SetValue((sunScreen.x - sunSize.x * 128.f) * 0.8f,
					                   (sunScreen.y - sunSize.y * 128.f) * 0.8f,
					                   (sunScreen.x + sunSize.x * 128.f) * 0.8f,
					                   (sunScreen.y + sunSize.y * 128.f) * 0.8f);

					qr.Draw();

					dev->ActiveTexture(2);
					flare3->Bind(IGLDevice::Texture2D);

					color.SetValue(sunColor.x * .3f, sunColor.y * .3f, sunColor.z * .3f);
					drawRange.SetValue((sunScreen.x - sunSize.x * 18.f) * 0.5f,
					                   (sunScreen.y - sunSize.y * 18.f) * 0.5f,
					                   (sunScreen.x + sunSize.x * 18.f) * 0.5f,
					                   (sunScreen.y + sunSize.y * 18.f) * 0.5f);

					qr.Draw();

					dev->ActiveTexture(1);
					mask1->Bind(IGLDevice::Texture2D);
					dev->ActiveTexture(2);
					flare3->Bind(IGLDevice::Texture2D);

					color.SetValue(sunColor.x * .8f * aroundness2, sunColor.y * .5f * aroundness2,
					               sunColor.z * .3f * aroundness2);
					float reflSize = 50.f + aroundness2 * 60.f;
					drawRange.SetValue((sunScreen.x - sunSize.x * reflSize) * -2.f,
					                   (sunScreen.y - sunSize.y * reflSize) * -2.f,
					                   (sunScreen.x + sunSize.x * reflSize) * -2.f,
					                   (sunScreen.y + sunSize.y * reflSize) * -2.f);

					qr.Draw();
				}
			}

			dev->ActiveTexture(0);

			// restore blend mode
			dev->BlendFunc(IGLDevice::SrcAlpha, IGLDevice::OneMinusSrcAlpha);
		}
	}
}
