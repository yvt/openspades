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


#include "GLDepthOfFieldFilter.h"
#include "IGLDevice.h"
#include "../Core/Math.h"
#include <vector>
#include "GLQuadRenderer.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLRenderer.h"
#include "../Core/Debug.h"
#include "GLProfiler.h"

namespace spades {
	namespace draw {
		GLDepthOfFieldFilter::GLDepthOfFieldFilter(GLRenderer *renderer):
		renderer(renderer){
			gaussProgram = renderer->RegisterProgram("Shaders/PostFilters/Gauss1D.program");
			blurProgram = renderer->RegisterProgram("Shaders/PostFilters/DoFBlur.program");
			cocGen = renderer->RegisterProgram("Shaders/PostFilters/DoFCoCGen.program");
			cocMix = renderer->RegisterProgram("Shaders/PostFilters/DoFCoCMix.program");
			gammaMix = renderer->RegisterProgram("Shaders/PostFilters/GammaMix.program");
		}
		
		GLColorBuffer GLDepthOfFieldFilter::BlurCoC(GLColorBuffer buffer,
											  float spread) {
			SPADES_MARK_FUNCTION();
			// do gaussian blur
			GLProgram *program = gaussProgram;
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
			GLColorBuffer buf2 = renderer->GetFramebufferManager()->CreateBufferHandle(w, h, 1);
			dev->BindTexture(IGLDevice::Texture2D, buffer.GetTexture());
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
			blur_unitShift.SetValue(spread / (float)w, 0.f);
			qr.Draw();
			buffer.Release();
			
			// y-direction
			GLColorBuffer buf3 = renderer->GetFramebufferManager()->CreateBufferHandle(w, h, 1);
			dev->BindTexture(IGLDevice::Texture2D, buf2.GetTexture());
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf3.GetFramebuffer());
			blur_unitShift.SetValue(0.f, spread / (float)h);
			qr.Draw();
			buf2.Release();
			
			return buf3;
		}
		
		GLColorBuffer GLDepthOfFieldFilter::GenerateCoC(float blurDepthRange) {
			SPADES_MARK_FUNCTION();
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			
			int w = dev->ScreenWidth();
			int h = dev->ScreenHeight();
			int w2 = (w + 3) / 4;
			int h2 = (h + 3) / 4;
			
			
			GLColorBuffer coc = renderer->GetFramebufferManager()->CreateBufferHandle(w2, h2, 1);
			
			{
				static GLProgramAttribute positionAttribute("positionAttribute");
				static GLProgramUniform depthTexture("depthTexture");
				static GLProgramUniform zNearFar("zNearFar");
				static GLProgramUniform pixelShift("pixelShift");
				static GLProgramUniform depthScale("depthScale");
				static GLProgramUniform maxVignetteBlur("maxVignetteBlur");
				static GLProgramUniform vignetteScale("vignetteScale");
				
				positionAttribute(cocGen);
				depthTexture(cocGen);
				zNearFar(cocGen);
				pixelShift(cocGen);
				depthScale(cocGen);
				maxVignetteBlur(cocGen);
				vignetteScale(cocGen);
				
				cocGen->Use();
				
				depthTexture.SetValue(0);
				
				const client::SceneDefinition& def = renderer->GetSceneDef();
				zNearFar.SetValue(def.zNear, def.zFar);
				
				pixelShift.SetValue(1.f / (float)w,
									1.f / (float)h);
				
				depthScale.SetValue(1.f / blurDepthRange);
				
				if(h > w){
					vignetteScale.SetValue(2.f * (float)w / (float)h,
										   2.f);
				}else{
					vignetteScale.SetValue(2.f,
										   2.f * (float)h / (float)w);
				}
				maxVignetteBlur.SetValue(sinf(std::max(def.fovX, def.fovY) * .5f) * .5f);
				
				qr.SetCoordAttributeIndex(positionAttribute());
				dev->BindTexture(IGLDevice::Texture2D, renderer->GetFramebufferManager()->GetDepthTexture());
				dev->BindFramebuffer(IGLDevice::Framebuffer, coc.GetFramebuffer());
				dev->Viewport(0, 0, w2, h2);
				qr.Draw();
				dev->BindTexture(IGLDevice::Texture2D, 0);
			}
			// make blurred CoC radius bitmap
			GLColorBuffer cocBlur = BlurCoC(coc, 1.f);
			
			// mix
			
			GLColorBuffer coc2 = renderer->GetFramebufferManager()->CreateBufferHandle(w2, h2, 1);
			
			{
				static GLProgramAttribute positionAttribute("positionAttribute");
				static GLProgramUniform cocTexture("cocTexture");
				static GLProgramUniform cocBlurTexture("cocBlurTexture");
				
				
				positionAttribute(cocMix);
				cocTexture(cocMix);
				cocBlurTexture(cocMix);
				
				cocMix->Use();
				
				cocBlurTexture.SetValue(1);
				dev->ActiveTexture(1);
				dev->BindTexture(IGLDevice::Texture2D, cocBlur.GetTexture());
				
				cocTexture.SetValue(0);
				dev->ActiveTexture(0);
				dev->BindTexture(IGLDevice::Texture2D, coc.GetTexture());
				
				qr.SetCoordAttributeIndex(positionAttribute());
				dev->BindFramebuffer(IGLDevice::Framebuffer, coc2.GetFramebuffer());
				dev->Viewport(0, 0, w2, h2);
				qr.Draw();
				dev->BindTexture(IGLDevice::Texture2D, 0);
			}
			
			return coc2;
		}
		
		GLColorBuffer GLDepthOfFieldFilter::Blur(GLColorBuffer buffer,
													
												 GLColorBuffer coc,
												 Vector2 offset) {
			SPADES_MARK_FUNCTION();
			// do gaussian blur
			GLProgram *program = blurProgram;
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			int w = buffer.GetWidth();
			int h = buffer.GetHeight();
			
			static GLProgramAttribute blur_positionAttribute("positionAttribute");
			static GLProgramUniform blur_textureUniform("texture");
			static GLProgramUniform blur_cocUniform("cocTexture");
			static GLProgramUniform blur_offset("offset");
			program->Use();
			blur_positionAttribute(program);
			blur_textureUniform(program);
			blur_cocUniform(program);
			blur_offset(program);
			
			blur_cocUniform.SetValue(1);
			dev->ActiveTexture(1);
			dev->BindTexture(IGLDevice::Texture2D, coc.GetTexture());
			
			blur_textureUniform.SetValue(0);
			dev->ActiveTexture(0);
			
			qr.SetCoordAttributeIndex(blur_positionAttribute());
			
			// x-direction
			float len = offset.GetLength();
			float sX = 1.f / (float)w, sY = 1.f / (float)h;
			while(len > .5f){
				GLColorBuffer buf2 = renderer->GetFramebufferManager()->CreateBufferHandle(w, h, false);
				dev->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
				dev->BindTexture(IGLDevice::Texture2D, buffer.GetTexture());
				blur_offset.SetValue(offset.x * sX, offset.y * sY);
				qr.Draw();
				buffer = buf2;
				
				offset *= .25f;
				len *= .25f;
			}
			
			return buffer;
		}

		GLColorBuffer GLDepthOfFieldFilter::AddMix(GLColorBuffer buffer1, GLColorBuffer buffer2) {
			SPADES_MARK_FUNCTION();
			// do gaussian blur
			GLProgram *program = gammaMix;
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			int w = buffer1.GetWidth();
			int h = buffer1.GetHeight();
			
			static GLProgramAttribute blur_positionAttribute("positionAttribute");
			static GLProgramUniform blur_textureUniform1("texture1");
			static GLProgramUniform blur_textureUniform2("texture2");
			static GLProgramUniform blur_unitShift("unitShift");
			static GLProgramUniform blur_mix1("mix1");
			static GLProgramUniform blur_mix2("mix2");
			program->Use();
			blur_positionAttribute(program);
			
			blur_textureUniform1(program);
			blur_textureUniform1.SetValue(1);
			dev->ActiveTexture(1);
			dev->BindTexture(IGLDevice::Texture2D, buffer1.GetTexture());
			
			blur_textureUniform2(program);
			blur_textureUniform2.SetValue(0);
			dev->ActiveTexture(0);
			dev->BindTexture(IGLDevice::Texture2D, buffer2.GetTexture());
			
			blur_mix1(program);
			blur_mix2(program);
			
			blur_mix1.SetValue(.5f, .5f, .5f);
			blur_mix2.SetValue(.5f, .5f, .5f);
			
			qr.SetCoordAttributeIndex(blur_positionAttribute());
			dev->Enable(IGLDevice::Blend, false);
			
			// x-direction
			GLColorBuffer buf2 = renderer->GetFramebufferManager()->CreateBufferHandle(w, h, false);
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
			qr.Draw();
			return buf2;
		}
		
		GLColorBuffer GLDepthOfFieldFilter::Filter(GLColorBuffer input, float blurDepthRange) {
			SPADES_MARK_FUNCTION();
			
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			
			int w = dev->ScreenWidth();
			int h = dev->ScreenHeight();
			
			dev->Enable(IGLDevice::Blend, false);
			
			GLColorBuffer coc;
            
            {
                GLProfiler p(dev, "CoC Computation");
                coc = GenerateCoC(blurDepthRange);
            }
			
			float maxCoc = (float)std::max(w, h) / 100.f;
			float cos60 = cosf(M_PI / 3.f);
			float sin60 = sinf(M_PI / 3.f);
			
			dev->Viewport(0, 0, w, h);
			
            GLColorBuffer buf1, buf2;
            {
                GLProfiler p(dev, "Blur 1");
                buf1 = Blur(input, coc,
                            MakeVector2(0.f, -1.f) * maxCoc);
            }
            {
                GLProfiler p(dev, "Blur 2");
                buf2 = Blur(input, coc,
									  MakeVector2(-sin60, cos60) * maxCoc);
            }
            {
                GLProfiler p(dev, "Mix 1");
                buf2 = AddMix(buf1, buf2);
			}
                //return buf2;
            {
                GLProfiler p(dev, "Blur 3");
                buf1 = Blur(buf1, coc,
						MakeVector2(-sin60, cos60) * maxCoc);
			}
            {
                GLProfiler p(dev, "Blur 4");
                buf2 = Blur(buf2, coc,
						MakeVector2(sin60, cos60) * maxCoc);
            }
			
            {
                GLProfiler p(dev, "Mix 2");
                GLColorBuffer output = AddMix(buf1, buf2);
                return output;
            }
			
		}
	}
}


