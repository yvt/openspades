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

#include "GLSoftSpriteRenderer.h"
#include <Core/Debug.h>
#include "GLFramebufferManager.h"
#include "GLImage.h"
#include "GLProfiler.h"
#include "GLProgram.h"
#include "GLQuadRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"
#include "SWFeatureLevel.h" // for fastRcp

namespace spades {
	namespace draw {

		GLSoftSpriteRenderer::GLSoftSpriteRenderer(GLRenderer *renderer)
		    : renderer(renderer),
		      device(renderer->GetGLDevice()),
		      settings(renderer->GetSettings()),
		      projectionViewMatrix("projectionViewMatrix"),
		      rightVector("rightVector"),
		      upVector("upVector"),
		      frontVector("frontVector"),
		      viewOriginVector("viewOriginVector"),
		      texture("mainTexture"),
		      depthTexture("depthTexture"),
		      viewMatrix("viewMatrix"),
		      fogDistance("fogDistance"),
		      fogColor("fogColor"),
		      zNearFar("zNearFar"),
		      positionAttribute("positionAttribute"),
		      spritePosAttribute("spritePosAttribute"),
		      colorAttribute("colorAttribute") {
			SPADES_MARK_FUNCTION();

			program = renderer->RegisterProgram("Shaders/SoftSprite.program");
		}

		GLSoftSpriteRenderer::~GLSoftSpriteRenderer() { SPADES_MARK_FUNCTION(); }

		void GLSoftSpriteRenderer::Add(spades::draw::GLImage *img, spades::Vector3 center,
		                               float rad, float ang, Vector4 color) {
			SPADES_MARK_FUNCTION_DEBUG();
			const client::SceneDefinition &def = renderer->GetSceneDef();
			Sprite spr;
			spr.image = img;
			spr.center = center;
			spr.radius = rad;
			spr.angle = ang;
			if (settings.r_hdr) {
				// linearize color
				if (color.x > color.w || color.y > color.w || color.z > color.w) {
					// emissive material
					color.x *= color.x;
					color.y *= color.y;
					color.z *= color.z;
				} else {
					// scattering/absorptive material
					float rcp = fastRcp(color.w + .01);
					color.x *= color.x * rcp;
					color.y *= color.y * rcp;
					color.z *= color.z * rcp;
				}
			}
			spr.color = color;
			spr.area = rad * rad * 4.f /
			           std::max(Vector3::Dot(center - def.viewOrigin, def.viewAxis[2]), 0.01f);
			sprites.push_back(spr);
		}

		void GLSoftSpriteRenderer::Clear() {
			SPADES_MARK_FUNCTION();
			sprites.clear();
		}

		float GLSoftSpriteRenderer::LayerForSprite(const Sprite &spr) {
			float v = (spr.area - thresLow) / thresRange;
			if (v < 0.f)
				v = 0.f;
			if (v > 1.f)
				v = 1.f;
			return v;
		}

		void GLSoftSpriteRenderer::Render() {
			SPADES_MARK_FUNCTION();
			lastImage = NULL;
			program->Use();

			device->Enable(IGLDevice::Blend, true);
			device->BlendFunc(IGLDevice::One, IGLDevice::OneMinusSrcAlpha);

			projectionViewMatrix(program);
			rightVector(program);
			frontVector(program);
			viewOriginVector(program);
			upVector(program);
			texture(program);
			depthTexture(program);
			viewMatrix(program);
			fogDistance(program);
			fogColor(program);
			zNearFar(program);

			positionAttribute(program);
			spritePosAttribute(program);
			colorAttribute(program);

			projectionViewMatrix.SetValue(renderer->GetProjectionViewMatrix());
			viewMatrix.SetValue(renderer->GetViewMatrix());

			fogDistance.SetValue(renderer->GetFogDistance());

			Vector3 fogCol = renderer->GetFogColor();
			fogCol *= fogCol; // linearize
			fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);

			const client::SceneDefinition &def = renderer->GetSceneDef();
			rightVector.SetValue(def.viewAxis[0].x, def.viewAxis[0].y, def.viewAxis[0].z);
			upVector.SetValue(def.viewAxis[1].x, def.viewAxis[1].y, def.viewAxis[1].z);
			frontVector.SetValue(def.viewAxis[2].x, def.viewAxis[2].y, def.viewAxis[2].z);

			viewOriginVector.SetValue(def.viewOrigin.x, def.viewOrigin.y, def.viewOrigin.z);
			texture.SetValue(0);
			depthTexture.SetValue(1);
			zNearFar.SetValue(def.zNear, def.zFar);

			device->ActiveTexture(1);
			device->BindTexture(IGLDevice::Texture2D,
			                    renderer->GetFramebufferManager()->GetDepthTexture());
			device->ActiveTexture(0);

			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(spritePosAttribute(), true);
			device->EnableVertexAttribArray(colorAttribute(), true);

			thresLow = tanf(def.fovX * .5f) * tanf(def.fovY * .5f) * 1.8f;
			thresRange = thresLow * .5f;

			// full-resolution sprites
			{
				GLProfiler::Context measure(renderer->GetGLProfiler(), "Full Resolution");
				for (size_t i = 0; i < sprites.size(); i++) {
					Sprite &spr = sprites[i];
					float layer = LayerForSprite(spr);
					if (layer == 1.f)
						continue;
					if (spr.image != lastImage) {
						Flush();
						lastImage = spr.image;
						SPAssert(vertices.empty());
					}

					Vertex v;
					v.x = spr.center.x;
					v.y = spr.center.y;
					v.z = spr.center.z;
					v.radius = spr.radius;
					v.angle = spr.angle;
					v.r = spr.color.x;
					v.g = spr.color.y;
					v.b = spr.color.z;
					v.a = spr.color.w;

					float fade = 1.f - layer;
					v.r *= fade;
					v.g *= fade;
					v.b *= fade;
					v.a *= fade;

					uint32_t idx = (uint32_t)vertices.size();
					v.sx = -1;
					v.sy = -1;
					vertices.push_back(v);
					v.sx = 1;
					v.sy = -1;
					vertices.push_back(v);
					v.sx = -1;
					v.sy = 1;
					vertices.push_back(v);
					v.sx = 1;
					v.sy = 1;
					vertices.push_back(v);

					indices.push_back(idx);
					indices.push_back(idx + 1);
					indices.push_back(idx + 2);
					indices.push_back(idx + 1);
					indices.push_back(idx + 3);
					indices.push_back(idx + 2);
				}

				Flush();
			}

			// low-res sprites
			IGLDevice::UInteger lastFb = device->GetInteger(IGLDevice::FramebufferBinding);
			int sW = device->ScreenWidth(), sH = device->ScreenHeight();
			int lW = (sW + 3) / 4, lH = (sH + 3) / 4;
			int numLowResSprites = 0;
			GLColorBuffer buf = renderer->GetFramebufferManager()->CreateBufferHandle(lW, lH, true);
			device->BindFramebuffer(IGLDevice::Framebuffer, buf.GetFramebuffer());
			device->ClearColor(0.f, 0.f, 0.f, 0.f);
			device->Clear(IGLDevice::ColorBufferBit);
			device->BlendFunc(IGLDevice::One, IGLDevice::OneMinusSrcAlpha);
			device->Viewport(0, 0, lW, lH);
			{
				GLProfiler::Context measure(renderer->GetGLProfiler(), "Low Resolution");
				for (size_t i = 0; i < sprites.size(); i++) {
					Sprite &spr = sprites[i];
					float layer = LayerForSprite(spr);
					if (layer == 0.f)
						continue;
					if (spr.image != lastImage) {
						Flush();
						lastImage = spr.image;
						SPAssert(vertices.empty());
					}

					numLowResSprites++;

					Vertex v;
					v.x = spr.center.x;
					v.y = spr.center.y;
					v.z = spr.center.z;
					v.radius = spr.radius;
					v.angle = spr.angle;
					v.r = spr.color.x;
					v.g = spr.color.y;
					v.b = spr.color.z;
					v.a = spr.color.w;

					float fade = layer;
					v.r *= fade;
					v.g *= fade;
					v.b *= fade;
					v.a *= fade;

					uint32_t idx = (uint32_t)vertices.size();
					v.sx = -1;
					v.sy = -1;
					vertices.push_back(v);
					v.sx = 1;
					v.sy = -1;
					vertices.push_back(v);
					v.sx = -1;
					v.sy = 1;
					vertices.push_back(v);
					v.sx = 1;
					v.sy = 1;
					vertices.push_back(v);

					indices.push_back(idx);
					indices.push_back(idx + 1);
					indices.push_back(idx + 2);
					indices.push_back(idx + 1);
					indices.push_back(idx + 3);
					indices.push_back(idx + 2);
				}
				Flush();
			}

			// finalize

			device->ActiveTexture(1);
			device->BindTexture(IGLDevice::Texture2D, 0);
			device->ActiveTexture(0);
			device->BindTexture(IGLDevice::Texture2D, 0);
			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(spritePosAttribute(), false);
			device->EnableVertexAttribArray(colorAttribute(), false);

			// composite downsampled sprite
			device->BlendFunc(IGLDevice::One, IGLDevice::OneMinusSrcAlpha);
			if (numLowResSprites > 0) {
				GLProfiler::Context measure(renderer->GetGLProfiler(), "Finalize");
				GLQuadRenderer qr(device);

				// do gaussian blur
				GLProgram *program =
				  renderer->RegisterProgram("Shaders/PostFilters/Gauss1D.program");
				static GLProgramAttribute blur_positionAttribute("positionAttribute");
				static GLProgramUniform blur_textureUniform("mainTexture");
				static GLProgramUniform blur_unitShift("unitShift");
				program->Use();
				blur_positionAttribute(program);
				blur_textureUniform(program);
				blur_unitShift(program);
				blur_textureUniform.SetValue(0);
				device->ActiveTexture(0);
				qr.SetCoordAttributeIndex(blur_positionAttribute());
				device->Enable(IGLDevice::Blend, false);

				// x-direction
				GLColorBuffer buf2 =
				  renderer->GetFramebufferManager()->CreateBufferHandle(lW, lH, true);
				device->BindTexture(IGLDevice::Texture2D, buf.GetTexture());
				device->BindFramebuffer(IGLDevice::Framebuffer, buf2.GetFramebuffer());
				blur_unitShift.SetValue(1.f / lW, 0.f);
				qr.Draw();
				buf.Release();

				// x-direction
				GLColorBuffer buf3 =
				  renderer->GetFramebufferManager()->CreateBufferHandle(lW, lH, true);
				device->BindTexture(IGLDevice::Texture2D, buf2.GetTexture());
				device->BindFramebuffer(IGLDevice::Framebuffer, buf3.GetFramebuffer());
				blur_unitShift.SetValue(0.f, 1.f / lH);
				qr.Draw();
				buf2.Release();

				buf = buf3;

				device->Enable(IGLDevice::Blend, true);

				// composite
				program = renderer->RegisterProgram("Shaders/PostFilters/PassThrough.program");
				static GLProgramAttribute positionAttribute("positionAttribute");
				static GLProgramUniform colorUniform("colorUniform");
				static GLProgramUniform textureUniform("mainTexture");
				static GLProgramUniform texCoordRange("texCoordRange");

				positionAttribute(program);
				textureUniform(program);
				texCoordRange(program);
				colorUniform(program);

				program->Use();

				textureUniform.SetValue(0);
				texCoordRange.SetValue(0.f, 0.f, 1.f, 1.f);
				colorUniform.SetValue(1.f, 1.f, 1.f, 1.f);

				qr.SetCoordAttributeIndex(positionAttribute());
				device->BindFramebuffer(IGLDevice::Framebuffer, lastFb);
				device->BindTexture(IGLDevice::Texture2D, buf.GetTexture());
				device->Viewport(0, 0, sW, sH);
				qr.Draw();
				device->BindTexture(IGLDevice::Texture2D, 0);

			} else {
				device->Viewport(0, 0, sW, sH);

				device->BindFramebuffer(IGLDevice::Framebuffer, lastFb);
			}

			buf.Release();
		}

		void GLSoftSpriteRenderer::Flush() {
			SPADES_MARK_FUNCTION_DEBUG();

			if (vertices.empty())
				return;

			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::FloatType, false,
			                            sizeof(Vertex), &(vertices[0].x));
			device->VertexAttribPointer(spritePosAttribute(), 4, IGLDevice::FloatType, false,
			                            sizeof(Vertex), &(vertices[0].sx));
			device->VertexAttribPointer(colorAttribute(), 4, IGLDevice::FloatType, false,
			                            sizeof(Vertex), &(vertices[0].r));

			SPAssert(lastImage);
			lastImage->Bind(IGLDevice::Texture2D);

			device->DrawElements(IGLDevice::Triangles,
			                     static_cast<IGLDevice::Sizei>(indices.size()),
			                     IGLDevice::UnsignedInt, indices.data());

			vertices.clear();
			indices.clear();
		}
	}
}
