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
#include "GLLensDustFilter.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"
#include <Core/Settings.h>

namespace spades {
	namespace draw {
		GLLensDustFilter::GLLensDustFilter(GLRenderer *renderer) : renderer(renderer) {
			thru = renderer->RegisterProgram("Shaders/PostFilters/PassThroughConstAlpha.program");
			gauss1d = renderer->RegisterProgram("Shaders/PostFilters/Gauss1D.program");
			dust = renderer->RegisterProgram("Shaders/PostFilters/LensDust.program");
			dustImg = (GLImage *)renderer->RegisterImage("Textures/LensDustTexture.jpg");

			IGLDevice *dev = renderer->GetGLDevice();
			noiseTex = dev->GenTexture();
			dev->BindTexture(IGLDevice::Texture2D, noiseTex);
			dev->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGBA8, 128, 128, 0, IGLDevice::BGRA,
			                IGLDevice::UnsignedByte, NULL);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                  IGLDevice::Nearest);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                  IGLDevice::Nearest);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS, IGLDevice::Repeat);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT, IGLDevice::Repeat);
		}

		GLLensDustFilter::~GLLensDustFilter() { renderer->GetGLDevice()->DeleteTexture(noiseTex); }

#define Level GLLensDustFilterLevel

		GLColorBuffer GLLensDustFilter::DownSample(GLColorBuffer tex, bool linearize) {
			SPADES_MARK_FUNCTION();
			GLProgram *program = thru;
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			int w = tex.GetWidth();
			int h = tex.GetHeight();

			static GLProgramAttribute blur_positionAttribute("positionAttribute");
			static GLProgramUniform blur_textureUniform("mainTexture");
			static GLProgramUniform blur_colorUniform("colorUniform");
			static GLProgramUniform blur_texCoordRangeUniform("texCoordRange");
			static GLProgramUniform blur_texCoordOffsetUniform("texCoordOffset");
			program->Use();
			blur_positionAttribute(program);

			blur_textureUniform(program);
			blur_textureUniform.SetValue(0);
			dev->ActiveTexture(0);
			dev->BindTexture(IGLDevice::Texture2D, tex.GetTexture());

			blur_texCoordOffsetUniform(program);
			blur_texCoordOffsetUniform.SetValue(1.f / w, 1.f / h, -1.f / w, -1.f / h);

			blur_colorUniform(program);
			blur_colorUniform.SetValue(1.f, 1.f, 1.f, 1.f);

			blur_texCoordRangeUniform(program);
			blur_texCoordRangeUniform.SetValue(0.f, 0.f, (float)((w + 1) & ~1) / w,
			                                   (float)((h + 1) & ~1) / h);

			qr.SetCoordAttributeIndex(blur_positionAttribute());
			if (linearize) {
				dev->Enable(IGLDevice::Blend, true);
				dev->BlendFunc(IGLDevice::SrcColor, IGLDevice::Zero);
			} else {
				dev->Enable(IGLDevice::Blend, false);
			}

			GLColorBuffer buf2 = renderer->GetFramebufferManager()->CreateBufferHandle(
			  (w + 1) / 2, (h + 1) / 2, false);
			dev->Viewport(0, 0, buf2.GetWidth(), buf2.GetHeight());
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
			qr.Draw();
			return buf2;
		}

		GLColorBuffer GLLensDustFilter::GaussianBlur(GLColorBuffer tex, bool vertical) {
			SPADES_MARK_FUNCTION();
			GLProgram *program = gauss1d;
			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);
			int w = tex.GetWidth();
			int h = tex.GetHeight();

			static GLProgramAttribute blur_positionAttribute("positionAttribute");
			static GLProgramUniform blur_textureUniform("mainTexture");
			static GLProgramUniform blur_unitShift("unitShift");
			program->Use();
			blur_positionAttribute(program);

			blur_textureUniform(program);
			blur_textureUniform.SetValue(0);
			dev->ActiveTexture(0);
			dev->BindTexture(IGLDevice::Texture2D, tex.GetTexture());

			blur_unitShift(program);
			blur_unitShift.SetValue(vertical ? 0.f : 1.f / w, vertical ? 1.f / h : 0.f);

			qr.SetCoordAttributeIndex(blur_positionAttribute());
			dev->Enable(IGLDevice::Blend, false);

			GLColorBuffer buf2 = renderer->GetFramebufferManager()->CreateBufferHandle(w, h, false);
			dev->Viewport(0, 0, buf2.GetWidth(), buf2.GetHeight());
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
			qr.Draw();
			return buf2;
		}

		void GLLensDustFilter::UpdateNoise() {
			SPADES_MARK_FUNCTION();

			noise.resize(128 * 128);
			for (size_t i = 0; i < 128 * 128; i++) {
				noise[i] = static_cast<std::uint32_t>(SampleRandom());
			}

			IGLDevice *dev = renderer->GetGLDevice();
			dev->BindTexture(IGLDevice::Texture2D, noiseTex);
			dev->TexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, 128, 128, IGLDevice::BGRA,
			                   IGLDevice::UnsignedByte, noise.data());
		}

		struct Level {
			int w, h;
			GLColorBuffer buffer;
			GLColorBuffer retBuf[4];
		};

		GLColorBuffer GLLensDustFilter::Filter(GLColorBuffer input) {
			SPADES_MARK_FUNCTION();

			UpdateNoise();

			std::vector<Level> levels;

			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);

			GLSettings &settings = renderer->GetSettings();

			static GLProgramAttribute thruPosition("positionAttribute");
			static GLProgramUniform thruColor("colorUniform");
			static GLProgramUniform thruTexture("mainTexture");
			static GLProgramUniform thruTexCoordRange("texCoordRange");

			thruPosition(thru);
			thruColor(thru);
			thruTexture(thru);
			thruTexCoordRange(thru);

			GLColorBuffer downSampled = DownSample(input, settings.r_hdr ? false : true);
			downSampled = GaussianBlur(downSampled, false);
			downSampled = GaussianBlur(downSampled, true);

			thru->Use();
			thruColor.SetValue(1.f, 1.f, 1.f, 1.f);
			thruTexture.SetValue(0);
			dev->Enable(IGLDevice::Blend, false);

			levels.reserve(10);

			// create downsample levels
			for (int i = 0; i < 10; i++) {
				GLColorBuffer prevLevel;
				if (i == 0) {
					prevLevel = downSampled;
				} else {
					prevLevel = levels.back().buffer;
				}

				int prevW = prevLevel.GetWidth();
				int prevH = prevLevel.GetHeight();
				int newW = (prevW + 1) / 2;
				int newH = (prevH + 1) / 2;
				if (newW <= 1 || newH <= 1)
					break;
				GLColorBuffer newLevel = DownSample(prevLevel);
				newLevel = GaussianBlur(newLevel, false);
				newLevel = GaussianBlur(newLevel, true);

				Level lv;
				lv.w = newW;
				lv.h = newH;
				lv.buffer = newLevel;
				levels.push_back(lv);
			}

			dev->Enable(IGLDevice::Blend, true);
			dev->BlendFunc(IGLDevice::SrcAlpha, IGLDevice::OneMinusSrcAlpha);

			// composite levels in the opposite direction
			thru->Use();
			qr.SetCoordAttributeIndex(thruPosition());
			for (int i = (int)levels.size() - 1; i >= 1; i--) {
				int cnt = (int)levels.size() - i;
				float alpha = (float)cnt / (float)(cnt + 1);
				alpha = alpha;

				GLColorBuffer curLevel = levels[i].buffer;

				for (int j = 0; j < 1; j++) {
					if (i < (int)levels.size() - 1) {
						curLevel = levels[i].retBuf[j];
					}
					GLColorBuffer targLevel = levels[i - 1].buffer;
					GLColorBuffer targRet = input.GetManager()->CreateBufferHandle(
					  targLevel.GetWidth(), targLevel.GetHeight(), false);
					levels[i - 1].retBuf[j] = targRet;

					dev->BindFramebuffer(IGLDevice::Framebuffer, targRet.GetFramebuffer());
					dev->Viewport(0, 0, targRet.GetWidth(), targRet.GetHeight());

					dev->BindTexture(IGLDevice::Texture2D, targLevel.GetTexture());
					thruColor.SetValue(1.f, 1.f, 1.f, 1.f);
					thruTexCoordRange.SetValue(0.f, 0.f, 1.f, 1.f);
					dev->Enable(IGLDevice::Blend, false);
					qr.Draw();

					float cx = 0.f, cy = 0.f;

					dev->BindTexture(IGLDevice::Texture2D, curLevel.GetTexture());
					thruColor.SetValue(1.f, 1.f, 1.f, alpha);
					thruTexCoordRange.SetValue(cx, cy, 1.f, 1.f);
					dev->Enable(IGLDevice::Blend, true);
					qr.Draw();

					dev->BindTexture(IGLDevice::Texture2D, 0);
				}
			}

			static GLProgramAttribute dustPosition("positionAttribute");
			static GLProgramUniform dustDustTexture("dustTexture");
			static GLProgramUniform dustBlurTexture1("blurTexture1");
			static GLProgramUniform dustInputTexture("inputTexture");
			static GLProgramUniform dustNoiseTexture("noiseTexture");
			static GLProgramUniform dustNoiseTexCoordFactor("noiseTexCoordFactor");

			dustPosition(dust);
			dustDustTexture(dust);
			dustBlurTexture1(dust);
			dustInputTexture(dust);
			dustNoiseTexture(dust);
			dustNoiseTexCoordFactor(dust);

			dust->Use();

			float facX = renderer->ScreenWidth() / 128.f;
			float facY = renderer->ScreenHeight() / 128.f;
			dustNoiseTexCoordFactor.SetValue(facX, facY, facX / 128.f, facY / 128.f);

			// composite to the final image
			GLColorBuffer output = input.GetManager()->CreateBufferHandle();
			GLColorBuffer topLevel1 = levels[0].retBuf[0];

			qr.SetCoordAttributeIndex(dustPosition());
			dev->ActiveTexture(0);
			dev->BindTexture(IGLDevice::Texture2D, input.GetTexture());
			dev->ActiveTexture(1);
			dev->BindTexture(IGLDevice::Texture2D, topLevel1.GetTexture());
			dev->ActiveTexture(5);
			dustImg->Bind(IGLDevice::Texture2D);
			dev->ActiveTexture(6);
			dev->BindTexture(IGLDevice::Texture2D, noiseTex);
			dev->BindFramebuffer(IGLDevice::Framebuffer, output.GetFramebuffer());
			dev->Viewport(0, 0, output.GetWidth(), output.GetHeight());
			dustBlurTexture1.SetValue(1);
			dustDustTexture.SetValue(5);
			dustNoiseTexture.SetValue(6);
			dustInputTexture.SetValue(0);
			qr.Draw();
			dev->BindTexture(IGLDevice::Texture2D, 0);

			dev->ActiveTexture(0);

			return output;
		}
	}
}
