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
#include <Core/Settings.h>
#include "GLImage.h"
#include "GLProgramManager.h"
#include "GLRenderer.h"

namespace spades {
	namespace draw {
		GLDynamicLightShader::GLDynamicLightShader()
		    : dynamicLightOrigin("dynamicLightOrigin"),
		      dynamicLightColor("dynamicLightColor"),
		      dynamicLightRadius("dynamicLightRadius"),
		      dynamicLightRadiusInversed("dynamicLightRadiusInversed"),
		      dynamicLightSpotMatrix("dynamicLightSpotMatrix"),
		      dynamicLightProjectionTexture("dynamicLightProjectionTexture")

		{
			lastRenderer = NULL;
		}

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
			if (lastRenderer != renderer) {
				whiteImage = static_cast<GLImage *>(renderer->RegisterImage("Gfx/White.tga"));
				lastRenderer = renderer;
			}

			const client::DynamicLightParam &param = light.GetParam();

			IGLDevice *device = renderer->GetGLDevice();
			dynamicLightOrigin(program);
			dynamicLightColor(program);
			dynamicLightRadius(program);
			dynamicLightRadiusInversed(program);
			dynamicLightSpotMatrix(program);
			dynamicLightProjectionTexture(program);

			dynamicLightOrigin.SetValue(param.origin.x, param.origin.y, param.origin.z);
			dynamicLightColor.SetValue(param.color.x, param.color.y, param.color.z);
			dynamicLightRadius.SetValue(param.radius);
			dynamicLightRadiusInversed.SetValue(1.f / param.radius);

			if (param.type == client::DynamicLightTypeSpotlight) {
				device->ActiveTexture(texStage);
				static_cast<GLImage *>(param.image)->Bind(IGLDevice::Texture2D);
				dynamicLightProjectionTexture.SetValue(texStage);
				texStage++;

				dynamicLightSpotMatrix.SetValue(light.GetProjectionMatrix());

				// bad hack to make texture clamped to edge
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
				                     IGLDevice::ClampToEdge);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
				                     IGLDevice::ClampToEdge);

			} else {
				device->ActiveTexture(texStage);
				whiteImage->Bind(IGLDevice::Texture2D);
				dynamicLightProjectionTexture.SetValue(texStage);
				texStage++;

				dynamicLightSpotMatrix.SetValue(Matrix4::Identity());
			}

			device->ActiveTexture(texStage);

			return texStage;
		}

		bool GLDynamicLightShader::Cull(const GLDynamicLight &light, const spades::AABB3 &box) {
			// TOOD: more tighter check?
			// TODO: spotlight check?
			// TODO: move this function to GLDynamicLight?
			const client::DynamicLightParam &param = light.GetParam();
			return box.Inflate(param.radius) && param.origin;
		}

		bool GLDynamicLightShader::SphereCull(const GLDynamicLight &light,
		                                      const spades::Vector3 &center, float radius) {
			const client::DynamicLightParam &param = light.GetParam();
			float maxDistance = radius + param.radius;
			return (center - param.origin).GetPoweredLength() < maxDistance * maxDistance;
		}
	}
}
