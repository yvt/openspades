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

#include "GLBloomFilter.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"
#include <Core/Debug.h>
#include <Core/Math.h>

namespace spades {
	namespace draw {
		GLBloomFilter::GLBloomFilter(GLRenderer &renderer) : renderer(renderer) {
			thru = renderer.RegisterProgram("Shaders/PostFilters/PassThrough.program");
		}

#define Level BloomLevel

		struct Level {
			int w, h;
			GLColorBuffer buffer;
		};

		GLColorBuffer GLBloomFilter::Filter(GLColorBuffer input) {
			SPADES_MARK_FUNCTION();

			std::vector<Level> levels;

			IGLDevice &dev = renderer.GetGLDevice();
			GLQuadRenderer qr(dev);

			static GLProgramAttribute thruPosition("positionAttribute");
			static GLProgramUniform thruColor("colorUniform");
			static GLProgramUniform thruTexture("mainTexture");
			static GLProgramUniform thruTexCoordRange("texCoordRange");

			thruPosition(thru);
			thruColor(thru);
			thruTexture(thru);
			thruTexCoordRange(thru);

			GLProgram *gammaMix = renderer.RegisterProgram("Shaders/PostFilters/GammaMix.program");
			static GLProgramAttribute gammaMixPosition("positionAttribute");
			static GLProgramUniform gammaMixTexture1("texture1");
			static GLProgramUniform gammaMixTexture2("texture2");
			static GLProgramUniform gammaMixMix1("mix1");
			static GLProgramUniform gammaMixMix2("mix2");

			gammaMixPosition(gammaMix);
			gammaMixTexture1(gammaMix);
			gammaMixTexture2(gammaMix);
			gammaMixMix1(gammaMix);
			gammaMixMix2(gammaMix);

			thru->Use();
			thruColor.SetValue(1.f, 1.f, 1.f, 1.f);
			thruTexture.SetValue(0);
			dev.Enable(IGLDevice::Blend, false);

			// create downsample levels
			for (int i = 0; i < 6; i++) {
				GLColorBuffer prevLevel;
				if (i == 0) {
					prevLevel = input;
				} else {
					prevLevel = levels.back().buffer;
				}

				int prevW = prevLevel.GetWidth();
				int prevH = prevLevel.GetHeight();
				int newW = (prevW + 1) / 2;
				int newH = (prevH + 1) / 2;
				GLColorBuffer newLevel = input.GetManager()->CreateBufferHandle(newW, newH);

				thru->Use();
				qr.SetCoordAttributeIndex(thruPosition());
				dev.BindTexture(IGLDevice::Texture2D, prevLevel.GetTexture());
				dev.BindFramebuffer(IGLDevice::Framebuffer, newLevel.GetFramebuffer());
				dev.Viewport(0, 0, newLevel.GetWidth(), newLevel.GetHeight());
				thruTexCoordRange.SetValue(0.f, 0.f,
				                           (float)newLevel.GetWidth() * 2.f / (float)prevW,
				                           (float)newLevel.GetHeight() * 2.f / (float)prevH);
				qr.Draw();
				dev.BindTexture(IGLDevice::Texture2D, 0);

				Level lv;
				lv.w = newW;
				lv.h = newH;
				lv.buffer = newLevel;
				levels.push_back(lv);
			}

			dev.Enable(IGLDevice::Blend, true);
			dev.BlendFunc(IGLDevice::SrcAlpha, IGLDevice::OneMinusSrcAlpha);

			// composite levels in the opposite direction
			thruTexCoordRange.SetValue(0.f, 0.f, 1.f, 1.f);
			for (int i = (int)levels.size() - 1; i >= 1; i--) {
				int cnt = (int)levels.size() - i;
				float alpha = (float)cnt / (float)(cnt + 1);
				alpha = sqrtf(alpha);
				GLColorBuffer curLevel = levels[i].buffer;
				GLColorBuffer targLevel = levels[i - 1].buffer;

				thru->Use();
				qr.SetCoordAttributeIndex(thruPosition());
				dev.BindTexture(IGLDevice::Texture2D, curLevel.GetTexture());
				dev.BindFramebuffer(IGLDevice::Framebuffer, targLevel.GetFramebuffer());
				dev.Viewport(0, 0, targLevel.GetWidth(), targLevel.GetHeight());
				thruColor.SetValue(1.f, 1.f, 1.f, alpha);
				qr.Draw();
				dev.BindTexture(IGLDevice::Texture2D, 0);
			}

			// composite to the final image
			GLColorBuffer output = input.GetManager()->CreateBufferHandle();
			GLColorBuffer topLevel = levels[0].buffer;

			gammaMix->Use();
			qr.SetCoordAttributeIndex(gammaMixPosition());
			dev.ActiveTexture(0);
			dev.BindTexture(IGLDevice::Texture2D, input.GetTexture());
			dev.ActiveTexture(1);
			dev.BindTexture(IGLDevice::Texture2D, topLevel.GetTexture());
			dev.BindFramebuffer(IGLDevice::Framebuffer, output.GetFramebuffer());
			dev.Viewport(0, 0, output.GetWidth(), output.GetHeight());
			gammaMixTexture1.SetValue(0);
			gammaMixTexture2.SetValue(1);
			gammaMixMix1.SetValue(.8f, .8f, .8f);
			gammaMixMix2.SetValue(.2f, .2f, .2f);
			qr.Draw();
			dev.BindTexture(IGLDevice::Texture2D, 0);

			dev.ActiveTexture(0);

			return output;
		}
	} // namespace draw
} // namespace spades
