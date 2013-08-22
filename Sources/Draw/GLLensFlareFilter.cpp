//
//  GLLensFlareFilter.cpp
//  OpenSpades
//
//  Created by Tomoaki Kawada on 8/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLLensFlareFilter.h"
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
		GLLensFlareFilter::GLLensFlareFilter(GLRenderer *renderer):
		renderer(renderer){
			
		}
		
		GLColorBuffer GLLensFlareFilter::Blur(GLColorBuffer buffer,
											  float spread) {
			// do gaussian blur
			GLProgram *program = renderer->RegisterProgram("Shaders/PostFilters/Gauss1D.program");
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			int w = buffer.GetWidth();
			int h = buffer.GetHeight();
			
			static GLProgramAttribute blur_positionAttribute("positionAttribute");
			static GLProgramUniform blur_textureUniform("texture");
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
			SPADES_MARK_FUNCTION();
			
			IGLDevice *dev = renderer->GetGLDevice();
			
			
			client::SceneDefinition def = renderer->GetSceneDef();
			
			// transform sun into NDC
			Vector3 sunWorld = MakeVector3(0, -1, -1);
			Vector3 sunView = {
				Vector3::Dot(sunWorld, def.viewAxis[0]),
				Vector3::Dot(sunWorld, def.viewAxis[1]),
				Vector3::Dot(sunWorld, def.viewAxis[2])
			};
			if(sunView.z <= 0.f) {
				return;
			}
			
			IGLDevice::UInteger lastFramebuffer = dev->GetInteger(IGLDevice::FramebufferBinding);
			
			Vector2 fov = {
				tanf(def.fovX * .5f),
				tanf(def.fovY * .5f)
			};
			Vector2 sunScreen;
			sunScreen.x = sunView.x / (sunView.z * fov.x);
			sunScreen.y = sunView.y / (sunView.z * fov.y);
			
			const float sunRadiusTan = tanf(.53f * .5f * M_PI / 180.f);
			Vector2 sunSize = {
				sunRadiusTan / fov.x,
				sunRadiusTan / fov.y
			};
			
			GLColorBuffer visiblityBuffer = renderer->GetFramebufferManager()->CreateBufferHandle(64, 64, false);
			
			GLQuadRenderer qr(dev);
			
			{
				GLProgram *scanner = renderer->RegisterProgram("Shaders/LensFlare/Scanner.program");
				static GLProgramAttribute positionAttribute("positionAttribute");
				static GLProgramUniform scanRange("scanRange");
				static GLProgramUniform drawRange("drawRange");
				static GLProgramUniform radius("radius");
				static GLProgramUniform depthTexture("depthTexture");
				
				scanner->Use();
				positionAttribute(scanner);
				scanRange(scanner);
				drawRange(scanner);
				radius(scanner);
				depthTexture(scanner);
				
				dev->Enable(IGLDevice::Blend, false);
				
				dev->ActiveTexture(0);
				dev->BindTexture(IGLDevice::Texture2D, renderer->GetFramebufferManager()->GetDepthTexture());
				depthTexture.SetValue(0);
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureCompareMode,
								  IGLDevice::CompareRefToTexture);
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureCompareFunc,
								  IGLDevice::Less);
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureMagFilter,
								  IGLDevice::Linear);
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureMinFilter,
								  IGLDevice::Linear);
				
				Vector2 sunTexPos = sunScreen * .5f + .5f;
				Vector2 sunTexSize = sunSize * .5f;
				scanRange.SetValue(sunTexPos.x - sunTexSize.x,
								   sunTexPos.y - sunTexSize.y,
								   sunTexPos.x + sunTexSize.x,
								   sunTexPos.y + sunTexSize.y);
				
				drawRange.SetValue(-.5f, -.5f,
								   .5f, .5f);
				radius.SetValue(32.f);
				
				qr.SetCoordAttributeIndex(positionAttribute());
				dev->BindFramebuffer(IGLDevice::Framebuffer,
									 visiblityBuffer.GetFramebuffer());
				dev->Viewport(0, 0, 64, 64);
				dev->ClearColor(0, 0, 0, 1);
				dev->Clear(IGLDevice::ColorBufferBit);
				qr.Draw();
				
				// restore depth texture's compare mode
				
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureMagFilter,
								  IGLDevice::Nearest);
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureMinFilter,
								  IGLDevice::Nearest);
				dev->TexParamater(IGLDevice::Texture2D,
								  IGLDevice::TextureCompareMode,
								  IGLDevice::None);
			}
			
			visiblityBuffer = Blur(visiblityBuffer, 1.f);
			visiblityBuffer = Blur(visiblityBuffer, 2.f);
			visiblityBuffer = Blur(visiblityBuffer, 4.f);
			
			dev->BindFramebuffer(IGLDevice::Framebuffer,
								 lastFramebuffer);
			
			{
				GLProgram *draw = renderer->RegisterProgram("Shaders/LensFlare/Draw.program");
				static GLProgramAttribute positionAttribute("positionAttribute");
				static GLProgramUniform drawRange("drawRange");
				static GLProgramUniform color("color");
				static GLProgramUniform visibilityTexture("visibilityTexture");
				
				draw->Use();
				
				positionAttribute(draw);
				drawRange(draw);
				visibilityTexture(draw);
				color(draw);
				
				dev->Enable(IGLDevice::Blend, true);
				dev->BlendFunc(IGLDevice::One,
							   IGLDevice::One);
				
				dev->ActiveTexture(0);
				dev->BindTexture(IGLDevice::Texture2D, visiblityBuffer.GetTexture());
				visibilityTexture.SetValue(0);
				
				qr.SetCoordAttributeIndex(positionAttribute());
				dev->Viewport(0, 0, dev->ScreenWidth(), dev->ScreenHeight());
				
				color.SetValue(.2f, .2f, .2f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 128.f,
								   sunScreen.y - sunSize.y * 128.f,
								   sunScreen.x + sunSize.x * 128.f,
								   sunScreen.y + sunSize.y * 128.f);
				
				qr.Draw();
				
				color.SetValue(.4f, .4f, .4f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 64.f,
								   sunScreen.y - sunSize.y * 64.f,
								   sunScreen.x + sunSize.x * 64.f,
								   sunScreen.y + sunSize.y * 64.f);
				qr.Draw();
				
				
				color.SetValue(.5f, .5f, .5f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 32.f,
								   sunScreen.y - sunSize.y * 32.f,
								   sunScreen.x + sunSize.x * 32.f,
								   sunScreen.y + sunSize.y * 32.f);
				qr.Draw();
				
				color.SetValue(.7f, .7f, .7f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 16.f,
								   sunScreen.y - sunSize.y * 16.f,
								   sunScreen.x + sunSize.x * 16.f,
								   sunScreen.y + sunSize.y * 16.f);
				qr.Draw();
				
				
				color.SetValue(1.f, 1.f, 1.f);
				drawRange.SetValue((sunScreen.x - sunSize.x * 4.f) * .4f,
								   (sunScreen.y - sunSize.y * 4.f) * .4f,
								   (sunScreen.x + sunSize.x * 4.f) * .4f,
								   (sunScreen.y + sunSize.y * 4.f) * .4f);
				
				qr.Draw();
				
				color.SetValue(.07f, .15f, .1f);
				drawRange.SetValue(-(sunScreen.x - sunSize.x * 38.f) * 1.6f,
								   -(sunScreen.y - sunSize.y * 38.f) * 1.6f,
								   -(sunScreen.x + sunSize.x * 38.f) * 1.6f,
								   -(sunScreen.y + sunSize.y * 38.f) * 1.6f);
				
				qr.Draw();
				
				color.SetValue(.03f, .08f, .2f);
				drawRange.SetValue(-(sunScreen.x - sunSize.x * 18.f),
								   -(sunScreen.y - sunSize.y * 18.f),
								   -(sunScreen.x + sunSize.x * 18.f),
								   -(sunScreen.y + sunSize.y * 18.f));
				
				qr.Draw();
				
				color.SetValue(1.f, 1.f, 1.f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 4.f,
								   sunScreen.y - sunSize.y * 4.f,
								   sunScreen.x + sunSize.x * 4.f,
								   sunScreen.y + sunSize.y * 4.f);
				
				qr.Draw();
				
				color.SetValue(.02f, .07f, .4f);
				drawRange.SetValue(sunScreen.x - sunSize.x * 256.f,
								   sunScreen.y - sunSize.y * 4.f,
								   sunScreen.x + sunSize.x * 256.f,
								   sunScreen.y + sunSize.y * 4.f);
				
				qr.Draw();
			}
			
			// restore blend mode
			dev->BlendFunc(IGLDevice::SrcAlpha,
						   IGLDevice::OneMinusSrcAlpha);
			
		}
	}
}