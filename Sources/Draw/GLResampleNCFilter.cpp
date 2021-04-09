/*
 Copyright (c) 2021 yvt

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

#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "GLResampleBicubicFilter.h"
#include "GLResampleNCFilter.h"
#include "GLFXAAFilter.h"
#include "IGLDevice.h"
#include <Core/Debug.h>
#include <Core/Math.h>

namespace spades {
	namespace draw {
		GLResampleNCFilter::GLResampleNCFilter(GLRenderer &renderer) : renderer(renderer) {
			lens = renderer.RegisterProgram("Shaders/PostFilters/ResampleNC.program");
			gaussProgram = renderer.RegisterProgram("Shaders/PostFilters/Gauss1D.program");

			// Preload
			GLResampleBicubicFilter{renderer};
			GLFXAAFilter{&renderer};
		}
		GLColorBuffer GLResampleNCFilter::Filter(GLColorBuffer input, int outputWidth,
		                                         int outputHeight) {
			SPADES_MARK_FUNCTION();

			IGLDevice *dev = renderer.GetGLDevice();
			GLQuadRenderer qr(dev);

			// ---------------------------------------------------------------

			GLColorBuffer resampledBuffer =
			  GLResampleBicubicFilter{renderer}.Filter(input, outputWidth, outputHeight);

			// ---------------------------------------------------------------

			static GLProgramAttribute blur_positionAttribute("positionAttribute");
			static GLProgramUniform blur_textureUniform("mainTexture");
			static GLProgramUniform blur_unitShift("unitShift");
			gaussProgram->Use();
			blur_positionAttribute(gaussProgram);
			blur_textureUniform(gaussProgram);
			blur_unitShift(gaussProgram);
			blur_textureUniform.SetValue(0);

			dev->ActiveTexture(0);
			qr.SetCoordAttributeIndex(blur_positionAttribute());
			dev->Enable(IGLDevice::Blend, false);

			GLColorBuffer intermediateBuffer = renderer.GetFramebufferManager()->CreateBufferHandle(
			  input.GetWidth(), resampledBuffer.GetHeight());
			dev->BindTexture(IGLDevice::Texture2D, resampledBuffer.GetTexture());
			dev->BindFramebuffer(IGLDevice::Framebuffer, intermediateBuffer.GetFramebuffer());
			dev->Viewport(0, 0, intermediateBuffer.GetWidth(), intermediateBuffer.GetHeight());
			blur_unitShift.SetValue(1.0f / (float)input.GetWidth(), 0.f);
			qr.Draw();

			// ---------------------------------------------------------------

			GLColorBuffer outputBuffer = input.GetManager()->CreateBufferHandle(
			  resampledBuffer.GetWidth(), resampledBuffer.GetHeight());

			static GLProgramAttribute lensPosition("positionAttribute");
			static GLProgramUniform lensIntermediateTexture("intermediateTexture");
			static GLProgramUniform lensResampledTexture("resampledTexture");
			static GLProgramUniform lensOriginalTexture("originalTexture");
			static GLProgramUniform inverseOriginalSize("inverseOriginalSize");

			dev->Enable(IGLDevice::Blend, false);

			lensPosition(lens);
			lensIntermediateTexture(lens);
			lensResampledTexture(lens);
			lensOriginalTexture(lens);
			inverseOriginalSize(lens);

			lens->Use();

			inverseOriginalSize.SetValue(1.f / input.GetWidth(), 1.f / input.GetHeight());

			dev->ActiveTexture(2);
			lensIntermediateTexture.SetValue(2);
			dev->BindTexture(IGLDevice::Texture2D, intermediateBuffer.GetTexture());

			dev->ActiveTexture(1);
			lensResampledTexture.SetValue(1);
			dev->BindTexture(IGLDevice::Texture2D, resampledBuffer.GetTexture());

			dev->ActiveTexture(0);
			lensOriginalTexture.SetValue(0);
			dev->BindTexture(IGLDevice::Texture2D, input.GetTexture());

			qr.SetCoordAttributeIndex(lensPosition());
			dev->BindFramebuffer(IGLDevice::Framebuffer, outputBuffer.GetFramebuffer());
			dev->Viewport(0, 0, outputBuffer.GetWidth(), outputBuffer.GetHeight());
			qr.Draw();
			dev->BindTexture(IGLDevice::Texture2D, 0);

			return GLFXAAFilter{&renderer}.Filter(outputBuffer);
		}
	} // namespace draw
} // namespace spades
