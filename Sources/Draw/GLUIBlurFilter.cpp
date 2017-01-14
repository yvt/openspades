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

#include <vector>

#include "GLUIBlurFilter.h"

#include "GLImage.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"
#include <Core/Debug.h>
#include <Core/Math.h>
#include <Core/Settings.h>

namespace spades {
	namespace draw {
		GLUIBlurFilter::GLUIBlurFilter(GLRenderer *renderer) : renderer(renderer) {
			SPAssert(renderer);

			thru = renderer->RegisterProgram("Shaders/PostFilters/PassThroughConstAlpha.program");
			gauss1d = renderer->RegisterProgram("Shaders/PostFilters/Gauss1D.program");
		}

		GLUIBlurFilter::~GLUIBlurFilter() {}

		void GLUIBlurFilter::UpdateScissorRect(int framebufferWidth, int framebufferHeight) {
			IGLDevice *const dev = renderer->GetGLDevice();
			int const origWidth = dev->ScreenWidth();
			int const origHeight = dev->ScreenHeight();
			struct {
				int x1, y1, x2, y2;
			} const scaledWorkArea = {workArea.x1 * framebufferWidth / origWidth,
			                          workArea.y1 * framebufferHeight / origHeight,
			                          (workArea.x2 * framebufferWidth + origWidth - 1) / origWidth,
			                          (workArea.y2 * framebufferHeight + origHeight - 1) /
			                            origHeight};
			dev->Scissor(scaledWorkArea.x1, scaledWorkArea.y1,
			             scaledWorkArea.x2 - scaledWorkArea.x1,
			             scaledWorkArea.y2 - scaledWorkArea.y1);
		}

		GLColorBuffer GLUIBlurFilter::DownSample(GLColorBuffer tex) {
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

			GLColorBuffer buf2 = renderer->GetFramebufferManager()->CreateBufferHandle(
			  (w + 1) / 2, (h + 1) / 2, false);
			UpdateScissorRect(buf2.GetWidth(), buf2.GetHeight());
			dev->Viewport(0, 0, buf2.GetWidth(), buf2.GetHeight());
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
			qr.Draw();
			return buf2;
		}

		GLColorBuffer GLUIBlurFilter::GaussianBlur(GLColorBuffer tex, bool vertical) {
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

			GLColorBuffer buf2 = renderer->GetFramebufferManager()->CreateBufferHandle(w, h, false);
			UpdateScissorRect(buf2.GetWidth(), buf2.GetHeight());
			dev->Viewport(0, 0, buf2.GetWidth(), buf2.GetHeight());
			dev->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
			qr.Draw();
			return buf2;
		}

		void GLUIBlurFilter::Apply(IGLDevice::UInteger framebuffer, const AABB2 &target) {
			SPADES_MARK_FUNCTION();

			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);

			// Compute the work area
			int margin = 16;
			workArea = {std::max(0, static_cast<int>(target.GetMinX()) - margin),
			            std::max(0, static_cast<int>(target.GetMinY()) - margin),
			            std::min(dev->ScreenWidth(), static_cast<int>(target.GetMaxX()) + margin),
			            std::min(dev->ScreenHeight(), static_cast<int>(target.GetMaxY()) + margin)};

			dev->Enable(IGLDevice::Blend, false);

			// Capture the input image
			auto input = renderer->GetFramebufferManager()->CreateBufferHandle(dev->ScreenWidth(),
			                                                                   dev->ScreenHeight());
			dev->BindFramebuffer(IGLDevice::Framebuffer, framebuffer);
			dev->BindTexture(IGLDevice::Texture2D, input.GetTexture());
			dev->CopyTexSubImage2D(IGLDevice::Texture2D, 0, workArea.x1, workArea.y1, workArea.x1,
			                       workArea.y1, workArea.x2 - workArea.x1,
			                       workArea.y2 - workArea.y1);

			// Downsample and then blur it
			GLColorBuffer downSampled = DownSample(input);
			downSampled = DownSample(downSampled);
			downSampled = GaussianBlur(downSampled, false);
			downSampled = GaussianBlur(downSampled, true);

			// Write back
			static GLProgramAttribute thruPosition("positionAttribute");
			static GLProgramUniform thruColor("colorUniform");
			static GLProgramUniform thruTexture("mainTexture");
			static GLProgramUniform thruTexCoordRange("texCoordRange");

			thruPosition(thru);
			thruColor(thru);
			thruTexture(thru);
			thruTexCoordRange(thru);

			workArea = {std::max(0, static_cast<int>(target.GetMinX())),
			            std::max(0, static_cast<int>(target.GetMinY())),
			            std::min(dev->ScreenWidth(), static_cast<int>(target.GetMaxX())),
			            std::min(dev->ScreenHeight(), static_cast<int>(target.GetMaxY()))};

			dev->BindFramebuffer(IGLDevice::Framebuffer, framebuffer);

			thru->Use();
			thruColor.SetValue(1.f, 1.f, 1.f, 1.f);
			thruTexture.SetValue(0);
			dev->ActiveTexture(0);
			dev->Viewport(0, 0, dev->ScreenWidth(), dev->ScreenHeight());
			dev->BindTexture(IGLDevice::Texture2D, downSampled.GetTexture());
			dev->Scissor(workArea.x1, workArea.y1, workArea.x2 - workArea.x1,
			             workArea.y2 - workArea.y1);
			qr.SetCoordAttributeIndex(thruPosition());
			qr.Draw();
		}
	}
}
