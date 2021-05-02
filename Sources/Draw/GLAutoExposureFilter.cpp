/*
 Copyright (c) 2015 yvt

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

#include "GLAutoExposureFilter.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "GLSettings.h"
#include "IGLDevice.h"
#include <Core/Debug.h>
#include <Core/Math.h>

namespace spades {
	namespace draw {
		GLAutoExposureFilter::GLAutoExposureFilter(GLRenderer &renderer) : renderer{renderer} {
			thru = renderer.RegisterProgram("Shaders/PostFilters/PassThrough.program");
			preprocess =
			  renderer.RegisterProgram("Shaders/PostFilters/AutoExposurePreprocess.program");
			computeGain = renderer.RegisterProgram("Shaders/PostFilters/AutoExposure.program");

			IGLDevice &dev = renderer.GetGLDevice();

			exposureTexture = dev.GenTexture();

			dev.BindTexture(IGLDevice::Texture2D, exposureTexture);

			dev.TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGBA16F, 1, 1, 0, IGLDevice::RGBA,
			               IGLDevice::UnsignedByte, NULL);
			SPLog("Brightness Texture Allocated");
			dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter, IGLDevice::Nearest);
			dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter, IGLDevice::Nearest);
			dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS, IGLDevice::ClampToEdge);
			dev.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT, IGLDevice::ClampToEdge);

			exposureFramebuffer = dev.GenFramebuffer();
			dev.BindFramebuffer(IGLDevice::Framebuffer, exposureFramebuffer);

			dev.FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
			                         IGLDevice::Texture2D, exposureTexture, 0);
			dev.Viewport(0, 0, 1, 1);
			dev.ClearColor(1.f, 1.f, 1.f, 1.f);
			dev.Clear(IGLDevice::ColorBufferBit);

			dev.BindFramebuffer(IGLDevice::Framebuffer, 0);
			dev.BindTexture(IGLDevice::Texture2D, 0);

			SPLog("Brightness Framebuffer Allocated");
		}

		GLAutoExposureFilter::~GLAutoExposureFilter() {
			IGLDevice &dev = renderer.GetGLDevice();
			dev.DeleteTexture(exposureTexture);
			dev.DeleteFramebuffer(exposureFramebuffer);
		}

		namespace {
			struct Level {
				int w, h;
				GLColorBuffer buffer;
			};
		} // namespace

		GLColorBuffer GLAutoExposureFilter::Filter(GLColorBuffer input, float dt) {
			SPADES_MARK_FUNCTION();

			std::vector<Level> levels;

			IGLDevice &dev = renderer.GetGLDevice();
			GLQuadRenderer qr(dev);

			GLSettings &settings = renderer.GetSettings();

			static GLProgramAttribute thruPosition("positionAttribute");
			static GLProgramUniform thruColor("colorUniform");
			static GLProgramUniform thruTexture("mainTexture");
			static GLProgramUniform thruTexCoordRange("texCoordRange");

			thruPosition(thru);
			thruColor(thru);
			thruTexture(thru);
			thruTexCoordRange(thru);

			static GLProgramAttribute preprocessPosition("positionAttribute");
			static GLProgramUniform preprocessColor("colorUniform");
			static GLProgramUniform preprocessTexture("mainTexture");
			static GLProgramUniform preprocessTexCoordRange("texCoordRange");

			preprocessPosition(preprocess);
			preprocessColor(preprocess);
			preprocessTexture(preprocess);
			preprocessTexCoordRange(preprocess);

			static GLProgramAttribute computeGainPosition("positionAttribute");
			static GLProgramUniform computeGainColor("colorUniform");
			static GLProgramUniform computeGainTexture("mainTexture");
			static GLProgramUniform computeGainTexCoordRange("texCoordRange");
			static GLProgramUniform computeGainMinGain("minGain");
			static GLProgramUniform computeGainMaxGain("maxGain");

			computeGainPosition(computeGain);
			computeGainColor(computeGain);
			computeGainTexture(computeGain);
			computeGainTexCoordRange(computeGain);
			computeGainMinGain(computeGain);
			computeGainMaxGain(computeGain);

			preprocess->Use();
			preprocessColor.SetValue(1.f, 1.f, 1.f, 1.f);
			preprocessTexture.SetValue(0);
			preprocessTexCoordRange.SetValue(0.f, 0.f, 1.f, 1.f);

			thru->Use();
			thruColor.SetValue(1.f, 1.f, 1.f, 1.f);
			thruTexture.SetValue(0);
			thruTexCoordRange.SetValue(0.f, 0.f, 1.f, 1.f);

			dev.Enable(IGLDevice::Blend, false);
			dev.ActiveTexture(0);

			// downsample until it becomes 1x1x
			GLColorBuffer buffer = input;
			bool firstLevel = true;

			while (buffer.GetWidth() > 1 || buffer.GetHeight() > 1) {
				int prevW = buffer.GetWidth();
				int prevH = buffer.GetHeight();
				int newW = (prevW + 1) / 2;
				int newH = (prevH + 1) / 2;
				GLColorBuffer newLevel = input.GetManager()->CreateBufferHandle(newW, newH);

				dev.BindTexture(IGLDevice::Texture2D, buffer.GetTexture());
				dev.BindFramebuffer(IGLDevice::Framebuffer, newLevel.GetFramebuffer());
				if (firstLevel) {
					preprocess->Use();
					qr.SetCoordAttributeIndex(preprocessPosition());
					firstLevel = false;
				} else {
					thru->Use();
					qr.SetCoordAttributeIndex(thruPosition());
				}
				dev.Viewport(0, 0, newLevel.GetWidth(), newLevel.GetHeight());
				dev.ClearColor(1.f, 1.f, 1.f, 1.f);
				dev.Clear(IGLDevice::ColorBufferBit);
				qr.Draw();
				dev.BindTexture(IGLDevice::Texture2D, 0);

				buffer = newLevel;
			}

			// compute estimated brightness on GPU
			dev.Enable(IGLDevice::Blend, true);
			dev.BlendFunc(IGLDevice::SrcAlpha, IGLDevice::OneMinusSrcAlpha);

			float minExposure = settings.r_hdrAutoExposureMin;
			float maxExposure = settings.r_hdrAutoExposureMax;

			// safety
			minExposure = std::min(std::max(minExposure, -10.0f), 10.0f);
			maxExposure = std::min(std::max(maxExposure, minExposure), 10.0f);

			// adaption speed control
			if ((float)settings.r_hdrAutoExposureSpeed < 0.0f) {
				settings.r_hdrAutoExposureSpeed = 0.0f;
			}
			float rate = 1.0f - std::pow(0.01f, dt * settings.r_hdrAutoExposureSpeed);

			computeGain->Use();
			computeGainTexCoordRange.SetValue(0.f, 0.f, 1.f, 1.f);
			computeGainTexture.SetValue(0);
			computeGainColor.SetValue(1.f, 1.f, 1.f, rate);
			computeGainMinGain.SetValue(std::pow(2.0f, minExposure));
			computeGainMaxGain.SetValue(std::pow(2.0f, maxExposure));
			qr.SetCoordAttributeIndex(computeGainPosition());
			dev.BindFramebuffer(IGLDevice::Framebuffer, exposureFramebuffer);
			dev.BindTexture(IGLDevice::Texture2D, buffer.GetTexture());
			dev.Viewport(0, 0, 1, 1);
			qr.Draw();
			dev.BindTexture(IGLDevice::Texture2D, 0);

			// apply exposure adjustment
			thru->Use();
			thruColor.SetValue(1.f, 1.f, 1.f, 1.f);
			thruTexCoordRange.SetValue(0.f, 0.f, 1.f, 1.f);
			thruTexture.SetValue(0);
			dev.Enable(IGLDevice::Blend, true);
			dev.BlendFunc(IGLDevice::DestColor, IGLDevice::Zero); // multiply
			qr.SetCoordAttributeIndex(thruPosition());
			dev.BindTexture(IGLDevice::Texture2D, exposureTexture);
			dev.BindFramebuffer(IGLDevice::Framebuffer, input.GetFramebuffer());
			dev.Viewport(0, 0, input.GetWidth(), input.GetHeight());

			qr.Draw();
			dev.BindTexture(IGLDevice::Texture2D, 0);

			dev.BlendFunc(IGLDevice::SrcAlpha, IGLDevice::OneMinusSrcAlpha);

			return input;
		}
	} // namespace draw
} // namespace spades
