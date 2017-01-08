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
#include "GLCameraBlurFilter.h"
#include "GLProfiler.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {
		GLCameraBlurFilter::GLCameraBlurFilter(GLRenderer *renderer) : renderer(renderer) {
			prevMatrix = Matrix4::Identity();
			program = renderer->RegisterProgram("Shaders/PostFilters/CameraBlur.program");
		}

#define M(r, c) (d.m[(r) + (c)*4])

		static Matrix4 ReverseMatrix(Matrix4 d) {
			return Matrix4(
			  M(1, 2) * M(2, 1) - M(1, 1) * M(2, 2), M(1, 0) * M(2, 2) - M(1, 2) * M(2, 0),
			  M(1, 1) * M(2, 0) - M(1, 0) * M(2, 1), 0, M(0, 1) * M(2, 2) - M(0, 2) * M(2, 1),
			  M(0, 2) * M(2, 0) - M(0, 0) * M(2, 2), M(0, 0) * M(2, 1) - M(0, 1) * M(2, 0), 0, 0, 0,
			  0, 0, M(0, 2) * M(1, 1) - M(0, 1) * M(1, 2), M(0, 0) * M(1, 2) - M(0, 2) * M(1, 0),
			  M(0, 1) * M(1, 0) - M(0, 0) * M(1, 1), 1);
		}

		static float MyACos(float v) {
			if (v >= 1.f)
				return 0.f;
			else
				return acosf(v);
		}

		GLColorBuffer GLCameraBlurFilter::Filter(GLColorBuffer input, float radialBlur) {
			SPADES_MARK_FUNCTION();

			if (radialBlur > 0.f)
				radialBlur = 1.f - radialBlur;
			else
				radialBlur = 1.f;

			bool hasRadialBlur = radialBlur < .9999f;

			IGLDevice *dev = renderer->GetGLDevice();
			GLQuadRenderer qr(dev);

			dev->Enable(IGLDevice::Blend, false);

			static GLProgramAttribute programPosition("positionAttribute");
			static GLProgramUniform programTexture("mainTexture");
			static GLProgramUniform programDepthTexture("depthTexture");
			static GLProgramUniform programReverseMatrix("reverseMatrix");
			static GLProgramUniform programShutterTimeScale("shutterTimeScale");

			programPosition(program);
			programTexture(program);
			programDepthTexture(program);
			programReverseMatrix(program);
			programShutterTimeScale(program);

			const client::SceneDefinition &def = renderer->GetSceneDef();
			Matrix4 newMatrix = Matrix4::Identity();
			Vector3 axes[] = {def.viewAxis[0], def.viewAxis[1], def.viewAxis[2]};
			axes[0] /= std::tan(def.fovX * 0.5f);
			axes[1] /= std::tan(def.fovY * 0.5f);
			newMatrix.m[0] = axes[0].x;
			newMatrix.m[1] = axes[1].x;
			newMatrix.m[2] = axes[2].x;
			newMatrix.m[4] = axes[0].y;
			newMatrix.m[5] = axes[1].y;
			newMatrix.m[6] = axes[2].y;
			newMatrix.m[8] = axes[0].z;
			newMatrix.m[9] = axes[1].z;
			newMatrix.m[10] = axes[2].z;

			Matrix4 inverseNewMatrix = newMatrix.Inversed();
			Matrix4 diffMatrix = prevMatrix * inverseNewMatrix;
			prevMatrix = newMatrix;
			Matrix4 reverseMatrix = ReverseMatrix(diffMatrix);

			if (diffMatrix.m[0] < .3f || diffMatrix.m[5] < .3f || diffMatrix.m[10] < .3f) {
				// too much change
				if (hasRadialBlur) {
					diffMatrix = Matrix4::Identity();
				} else {
					// skip blur
					return input;
				}
			}

			float movePixels = MyACos(diffMatrix.m[0]);
			float shutterTimeScale = .3f;
			movePixels = std::max(movePixels, MyACos(diffMatrix.m[5]));
			movePixels = std::max(movePixels, MyACos(diffMatrix.m[10]));
			movePixels = tanf(movePixels) / tanf(def.fovX * .5f);
			movePixels *= (float)dev->ScreenWidth() * .5f;
			movePixels *= shutterTimeScale;

			movePixels = std::max(movePixels, (1.f - radialBlur) * dev->ScreenWidth() * 0.5f);

			if (movePixels < 1.f) {
				// too less change, skip camera blur
				return input;
			}

			int levels = (int)ceilf(logf(movePixels) / logf(5.f));
			if (levels <= 0)
				levels = 1;

			if (hasRadialBlur)
				radialBlur *= radialBlur;
			reverseMatrix = Matrix4::Scale(radialBlur, radialBlur, 1.f) * reverseMatrix;

			program->Use();

			programTexture.SetValue(0);
			programDepthTexture.SetValue(1);
			programReverseMatrix.SetValue(reverseMatrix);

			// composite to the final image
			GLColorBuffer buf = input;

			qr.SetCoordAttributeIndex(programPosition());
			dev->ActiveTexture(1);
			dev->BindTexture(IGLDevice::Texture2D,
			                 renderer->GetFramebufferManager()->GetDepthTexture());
			dev->ActiveTexture(0);

			for (int i = 0; i < levels; i++) {
				GLColorBuffer output = input.GetManager()->CreateBufferHandle();
				programShutterTimeScale.SetValue(shutterTimeScale);
				dev->BindTexture(IGLDevice::Texture2D, buf.GetTexture());
				dev->BindFramebuffer(IGLDevice::Framebuffer, output.GetFramebuffer());
				dev->Viewport(0, 0, output.GetWidth(), output.GetHeight());
				qr.Draw();
				dev->BindTexture(IGLDevice::Texture2D, 0);
				shutterTimeScale /= 5.f;
				buf = output;
			}

			dev->ActiveTexture(1);
			dev->BindTexture(IGLDevice::Texture2D, 0);
			dev->ActiveTexture(0);

			return buf;
		}
	}
}
