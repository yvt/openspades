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

#include "GLDynamicLightShader.h"
#include "GLImage.h"
#include "GLProgramManager.h"
#include "GLRenderer.h"
#include <Core/Settings.h>

namespace spades {
	namespace draw {
		GLDynamicLightShader::GLDynamicLightShader()
		    : dynamicLightOrigin("dynamicLightOrigin"),
		      dynamicLightColor("dynamicLightColor"),
		      dynamicLightRadius("dynamicLightRadius"),
		      dynamicLightRadiusInversed("dynamicLightRadiusInversed"),
		      dynamicLightSpotMatrix("dynamicLightSpotMatrix"),
		      dynamicLightProjectionTexture("dynamicLightProjectionTexture"),
		      dynamicLightIsLinear("dynamicLightIsLinear"),
		      dynamicLightLinearDirection("dynamicLightLinearDirection"),
		      dynamicLightLinearLength("dynamicLightLinearLength")

		{
			lastRenderer = NULL;
		}

		GLDynamicLightShader::~GLDynamicLightShader() {}

		std::vector<GLShader *>
		GLDynamicLightShader::RegisterShader(spades::draw::GLProgramManager *r) {
			std::vector<GLShader *> shaders;

			shaders.push_back(r->RegisterShader("Shaders/DynamicLight/Common.fs"));
			shaders.push_back(r->RegisterShader("Shaders/DynamicLight/Common.vs"));

			shaders.push_back(r->RegisterShader("Shaders/DynamicLight/MapNull.fs"));
			shaders.push_back(r->RegisterShader("Shaders/DynamicLight/MapNull.vs"));

			return shaders;
		}

		int GLDynamicLightShader::operator()(GLRenderer *renderer, spades::draw::GLProgram *program,
		                                     const GLDynamicLight &light, int texStage) {
			// TODO: Raw pointers are not unique!
			if (lastRenderer != renderer) {
				whiteImage = renderer->RegisterImage("Gfx/White.tga").Cast<GLImage>();
				lastRenderer = renderer;
			}

			const client::DynamicLightParam &param = light.GetParam();

			IGLDevice &device = renderer->GetGLDevice();
			dynamicLightOrigin(program);
			dynamicLightColor(program);
			dynamicLightRadius(program);
			dynamicLightRadiusInversed(program);
			dynamicLightSpotMatrix(program);
			dynamicLightProjectionTexture(program);
			dynamicLightIsLinear(program);
			dynamicLightLinearDirection(program);
			dynamicLightLinearLength(program);

			dynamicLightOrigin.SetValue(param.origin.x, param.origin.y, param.origin.z);
			dynamicLightColor.SetValue(param.color.x, param.color.y, param.color.z);
			dynamicLightRadius.SetValue(param.radius);
			dynamicLightRadiusInversed.SetValue(1.f / param.radius);

			if (param.type == client::DynamicLightTypeSpotlight) {
				device.ActiveTexture(texStage);
				static_cast<GLImage *>(param.image)->Bind(IGLDevice::Texture2D);
				dynamicLightProjectionTexture.SetValue(texStage);
				texStage++;

				dynamicLightSpotMatrix.SetValue(light.GetProjectionMatrix());

				// bad hack to make texture clamped to edge
				device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
				                    IGLDevice::ClampToEdge);
				device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
				                    IGLDevice::ClampToEdge);

				dynamicLightIsLinear.SetValue(0);
			} else if (param.type == client::DynamicLightTypePoint ||
			           param.type == client::DynamicLightTypeLinear) {
				device.ActiveTexture(texStage);
				whiteImage->Bind(IGLDevice::Texture2D);
				dynamicLightProjectionTexture.SetValue(texStage);
				texStage++;

				// The shader samples from a white image. However, we still have to make sure
				// UV is in a valid range so the fragments are not discarded.
				dynamicLightSpotMatrix.SetValue(Matrix4::Translate(0.5, 0.5, 0.0) *
				                                Matrix4::Scale(0.0));

				if (param.type == client::DynamicLightTypeLinear) {
					// Convert two endpoints to one endpoint + direction + length.
					// `Vector3::Normalize` is no-op when the length is zero,
					// therefore the zero-length case is handled.
					Vector3 direction = param.point2 - param.origin;
					float length = direction.GetLength();
					direction = direction.Normalize();

					dynamicLightLinearDirection.SetValue(direction.x, direction.y, direction.z);
					dynamicLightLinearLength.SetValue(length);

					dynamicLightIsLinear.SetValue(1);
				} else {
					dynamicLightIsLinear.SetValue(0);
				}
			} else {
				SPUnreachable();
			}

			device.ActiveTexture(texStage);

			return texStage;
		}
	} // namespace draw
} // namespace spades
