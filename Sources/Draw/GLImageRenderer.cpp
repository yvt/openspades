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

#include "GLImageRenderer.h"
#include <Core/Debug.h>
#include <Core/Debug.h>
#include <Core/Exception.h>
#include "GLImage.h"
#include "GLRenderer.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {
		GLImageRenderer::GLImageRenderer(GLRenderer *r)
		    : renderer(r),
		      device(r->GetGLDevice()),
		      invScreenWidthFactored(2.f / device->ScreenWidth()),
		      invScreenHeightFactored(-2.f / device->ScreenHeight()) {

			SPADES_MARK_FUNCTION();
			image = NULL;

			program = renderer->RegisterProgram("Shaders/BasicImage.program");

			program->Use();

			positionAttribute = new GLProgramAttribute("positionAttribute");
			colorAttribute = new GLProgramAttribute("colorAttribute");
			textureCoordAttribute = new GLProgramAttribute("textureCoordAttribute");
			screenSize = new GLProgramUniform("invScreenSizeFactored");
			textureSize = new GLProgramUniform("invTextureSize");
			texture = new GLProgramUniform("mainTexture");

			(*positionAttribute)(program);
			(*colorAttribute)(program);
			(*textureCoordAttribute)(program);

			(*screenSize)(program);
			(*textureSize)(program);
			(*texture)(program);
		}

		GLImageRenderer::~GLImageRenderer() {
			if (image != NULL) {
				image->Release();
				image = NULL;
			}
			delete positionAttribute;
			delete colorAttribute;
			delete textureCoordAttribute;
			delete screenSize;
			delete textureSize;
			delete texture;
		}

		void GLImageRenderer::Flush() {
			if (vertices.empty())
				return;

			SPADES_MARK_FUNCTION();
			SPAssert(image);

			program->Use();

			device->ActiveTexture(0);
			image->Bind(IGLDevice::Texture2D);

			device->VertexAttribPointer((*positionAttribute)(), 2, IGLDevice::FloatType, false,
			                            sizeof(ImageVertex), vertices.data());
			device->VertexAttribPointer((*colorAttribute)(), 4, IGLDevice::FloatType, false,
			                            sizeof(ImageVertex),
			                            (const char *)vertices.data() + sizeof(float) * 4);
			device->VertexAttribPointer((*textureCoordAttribute)(), 2, IGLDevice::FloatType, false,
			                            sizeof(ImageVertex),
			                            (const char *)vertices.data() + sizeof(float) * 2);

			device->EnableVertexAttribArray((*positionAttribute)(), true);
			device->EnableVertexAttribArray((*colorAttribute)(), true);
			device->EnableVertexAttribArray((*textureCoordAttribute)(), true);

			screenSize->SetValue(invScreenWidthFactored, invScreenHeightFactored);
			textureSize->SetValue(image->GetInvWidth(), image->GetInvHeight());
			texture->SetValue(0);

			device->DrawElements(IGLDevice::Triangles,
			                     static_cast<IGLDevice::Sizei>(indices.size()),
			                     IGLDevice::UnsignedInt, indices.data());

			device->EnableVertexAttribArray((*positionAttribute)(), false);
			device->EnableVertexAttribArray((*colorAttribute)(), false);
			device->EnableVertexAttribArray((*textureCoordAttribute)(), false);

			vertices.clear();
			indices.clear();
			image->Release();
			image = NULL;
		}

		void GLImageRenderer::SetImage(spades::draw::GLImage *img) {
			if (img == image)
				return;
			Flush();
			image = img;
			image->AddRef();
		}

		void GLImageRenderer::Add(float dx1, float dy1, float dx2, float dy2, float dx3, float dy3,
		                          float dx4, float dy4, float sx1, float sy1, float sx2, float sy2,
		                          float sx3, float sy3, float sx4, float sy4, float r, float g,
		                          float b, float a) {
			ImageVertex v;
			v.r = r;
			v.g = g;
			v.b = b;
			v.a = a;

			uint32_t idx = (uint32_t)vertices.size();

			v.x = dx1;
			v.y = dy1;
			v.u = sx1;
			v.v = sy1;
			vertices.push_back(v);

			v.x = dx2;
			v.y = dy2;
			v.u = sx2;
			v.v = sy2;
			vertices.push_back(v);

			v.x = dx3;
			v.y = dy3;
			v.u = sx3;
			v.v = sy3;
			vertices.push_back(v);

			v.x = dx4;
			v.y = dy4;
			v.u = sx4;
			v.v = sy4;
			vertices.push_back(v);

			indices.push_back(idx);
			indices.push_back(idx + 1);
			indices.push_back(idx + 2);
			indices.push_back(idx);
			indices.push_back(idx + 2);
			indices.push_back(idx + 3);
		}
	}
}
