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

#include "GLBasicShadowMapRenderer.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/Settings.h>
#include "GLProfiler.h"
#include "GLRenderer.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {

		GLBasicShadowMapRenderer::GLBasicShadowMapRenderer(GLRenderer *r)
		    : IGLShadowMapRenderer(r) {
			SPADES_MARK_FUNCTION();

			device = r->GetGLDevice();

			textureSize = r->GetSettings().r_shadowMapSize;
			if ((int)textureSize > 4096) {
				SPLog("r_shadowMapSize is too large; changed to 4096");
				r->GetSettings().r_shadowMapSize = textureSize = 4096;
			}

			colorTexture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D, colorTexture);
			device->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGB, textureSize, textureSize, 0,
			                   IGLDevice::RGB, IGLDevice::UnsignedByte, NULL);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
			                     IGLDevice::ClampToEdge);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
			                     IGLDevice::ClampToEdge);

			for (int i = 0; i < NumSlices; i++) {
				texture[i] = device->GenTexture();
				device->BindTexture(IGLDevice::Texture2D, texture[i]);
				device->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::DepthComponent24,
				                   textureSize, textureSize, 0, IGLDevice::DepthComponent,
				                   IGLDevice::UnsignedInt, NULL);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
				                     IGLDevice::Linear);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
				                     IGLDevice::Linear);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
				                     IGLDevice::ClampToEdge);
				device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
				                     IGLDevice::ClampToEdge);

				framebuffer[i] = device->GenFramebuffer();
				device->BindFramebuffer(IGLDevice::Framebuffer, framebuffer[i]);
				device->FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
				                             IGLDevice::Texture2D, colorTexture, 0);
				device->FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::DepthAttachment,
				                             IGLDevice::Texture2D, texture[i], 0);

				device->BindFramebuffer(IGLDevice::Framebuffer, 0);
			}
		}

		GLBasicShadowMapRenderer::~GLBasicShadowMapRenderer() {
			SPADES_MARK_FUNCTION();

			for (int i = 0; i < NumSlices; i++) {
				device->DeleteTexture(texture[i]);
				device->DeleteFramebuffer(framebuffer[i]);
			}
			device->DeleteTexture(colorTexture);
		}

#define Segment GLBSMRSegment

		struct Segment {
			float low, high;
			bool empty;
			Segment() : empty(true) {}
			Segment(float l, float h) : low(std::min(l, h)), high(std::max(l, h)), empty(false) {}
			void operator+=(const Segment &seg) {
				if (seg.empty)
					return;
				if (empty) {
					low = seg.low;
					high = seg.high;
					empty = false;
				} else {
					low = std::min(low, seg.low);
					high = std::max(high, seg.high);
				}
			}
			void operator+=(float v) {
				if (empty) {
					low = high = v;
				} else {
					if (v < low)
						low = v;
					if (v > high)
						high = v;
				}
			}
		};

		static Vector3 FrustrumCoord(const client::SceneDefinition &def, float x, float y,
		                             float z) {
			x *= z;
			y *= z;
			return def.viewOrigin + def.viewAxis[0] * x + def.viewAxis[1] * y + def.viewAxis[2] * z;
		}

		static Segment ZRange(Vector3 base, Vector3 dir, Plane3 plane1, Plane3 plane2) {
			return Segment(plane1.GetDistanceTo(base) / Vector3::Dot(dir, plane1.n),
			               plane2.GetDistanceTo(base) / Vector3::Dot(dir, plane2.n));
		}

		void GLBasicShadowMapRenderer::BuildMatrix(float near, float far) {
			// TODO: variable light direction?
			Vector3 lightDir = MakeVector3(0, -1, -1).Normalize();
			// set better up dir?
			Vector3 up = MakeVector3(0, 0, 1);
			Vector3 side = Vector3::Cross(up, lightDir).Normalize();
			up = Vector3::Cross(lightDir, side).Normalize();

			// build frustrum
			client::SceneDefinition def = GetRenderer()->GetSceneDef();
			Vector3 frustrum[8];
			float tanX = tanf(def.fovX * .5f);
			float tanY = tanf(def.fovY * .5f);

			frustrum[0] = FrustrumCoord(def, tanX, tanY, near);
			frustrum[1] = FrustrumCoord(def, tanX, -tanY, near);
			frustrum[2] = FrustrumCoord(def, -tanX, tanY, near);
			frustrum[3] = FrustrumCoord(def, -tanX, -tanY, near);
			frustrum[4] = FrustrumCoord(def, tanX, tanY, far);
			frustrum[5] = FrustrumCoord(def, tanX, -tanY, far);
			frustrum[6] = FrustrumCoord(def, -tanX, tanY, far);
			frustrum[7] = FrustrumCoord(def, -tanX, -tanY, far);

			// compute frustrum's x,y boundary
			float minX, maxX, minY, maxY;
			minX = maxX = Vector3::Dot(frustrum[0], side);
			minY = maxY = Vector3::Dot(frustrum[0], up);
			for (int i = 1; i < 8; i++) {
				float x = Vector3::Dot(frustrum[i], side);
				float y = Vector3::Dot(frustrum[i], up);
				if (x < minX)
					minX = x;
				if (x > maxX)
					maxX = x;
				if (y < minY)
					minY = y;
				if (y > maxY)
					maxY = y;
			}

			// compute frustrum's z boundary
			Segment seg;
			Plane3 plane1(0, 0, 1, -4.f);
			Plane3 plane2(0, 0, 1, 64.f);
			seg += ZRange(side * minX + up * minY, lightDir, plane1, plane2);
			seg += ZRange(side * minX + up * maxY, lightDir, plane1, plane2);
			seg += ZRange(side * maxX + up * minY, lightDir, plane1, plane2);
			seg += ZRange(side * maxX + up * maxY, lightDir, plane1, plane2);

			for (int i = 1; i < 8; i++) {
				seg += Vector3::Dot(frustrum[i], lightDir);
			}

			// build frustrum obb
			Vector3 origin = side * minX + up * minY + lightDir * seg.low;
			Vector3 axis1 = side * (maxX - minX);
			Vector3 axis2 = up * (maxY - minY);
			Vector3 axis3 = lightDir * (seg.high - seg.low);

			obb = OBB3(Matrix4::FromAxis(axis1, axis2, axis3, origin));
			vpWidth = 2.f / axis1.GetLength();
			vpHeight = 2.f / axis2.GetLength();

			// convert to projectionview matrix
			matrix = obb.m.InversedFast();

			matrix = Matrix4::Scale(2.f) * matrix;
			matrix = Matrix4::Translate(-1, -1, -1) * matrix;

			// scale a little big for padding
			matrix = Matrix4::Scale(.98f) * matrix;
			//
			matrix = Matrix4::Scale(1, 1, -1) * matrix;

// make sure frustrums in range
#ifndef NDEBUG
			for (int i = 0; i < 8; i++) {
				Vector4 v = matrix * frustrum[i];
				SPAssert(v.x >= -1.f);
				SPAssert(v.y >= -1.f);
				// SPAssert(v.z >= -1.f);
				SPAssert(v.x < 1.f);
				SPAssert(v.y < 1.f);
				// SPAssert(v.z < 1.f);
			}
#endif
		}

		void GLBasicShadowMapRenderer::Render() {
			SPADES_MARK_FUNCTION();

			IGLDevice::Integer lastFb = device->GetInteger(IGLDevice::FramebufferBinding);

			// client::SceneDefinition def = GetRenderer()->GetSceneDef();

			float nearDist = 0.f;

			for (int i = 0; i < NumSlices; i++) {

				GLProfiler::Context profiler(GetRenderer()->GetGLProfiler(), "Slice %d / %d", i + 1, (int)NumSlices);

				float farDist = 0.0;
				// TODO: variable far distance according to the scene definition
				//       (note that this needs uniform shader variable)
				switch (i) {
					case 0: farDist = 12.f; break;
					case 1: farDist = 40.f; break;
					case 2: farDist = 150.f; break;
				}

				BuildMatrix(nearDist, farDist);
				matrices[i] = matrix;
				/*
				printf("m[%d]=\n[%f,%f,%f,%f]\n[%f,%f,%f,%f]\n[%f,%f,%f,%f]\n[%f,%f,%f,%f]\n",
				       i, matrix.m[0], matrix.m[4], matrix.m[8], matrix.m[12],
				       matrix.m[1], matrix.m[5], matrix.m[9], matrix.m[13],
				       matrix.m[2], matrix.m[6], matrix.m[10], matrix.m[14],
				       matrix.m[3], matrix.m[7], matrix.m[11], matrix.m[15]);*/
				/*
				matrix = Matrix4::Identity();
				matrix = Matrix4::Scale(1.f / 16.f);
				matrix = matrix * Matrix4::Rotate(MakeVector3(1, 0, 0), M_PI / 4.f);
				matrix = matrix * Matrix4::Translate(-def.viewOrigin);
				matrix = Matrix4::Scale(1,1,16.f / 70.f) * matrix;*/

				device->BindFramebuffer(IGLDevice::Framebuffer, framebuffer[i]);
				device->Viewport(0, 0, textureSize, textureSize);
				device->ClearDepth(1.f);
				device->Clear(IGLDevice::DepthBufferBit);

				RenderShadowMapPass();

				nearDist = farDist;
			}

			device->BindFramebuffer(IGLDevice::Framebuffer, lastFb);

			device->Viewport(0, 0, device->ScreenWidth(), device->ScreenHeight());
		}

		bool GLBasicShadowMapRenderer::Cull(const spades::AABB3 &) {

			// TODO: who uses this?
			SPNotImplemented();
			return true;
		}

		bool GLBasicShadowMapRenderer::SphereCull(const spades::Vector3 &center, float rad) {
			Vector4 vw = matrix * center;
			float xx = fabsf(vw.x);
			float yy = fabsf(vw.y);
			float rx = rad * vpWidth;
			float ry = rad * vpHeight;

			return xx < (1.f + rx) && yy < (1.f + ry);

			// return true;
			// return obb.GetDistanceTo(center) < rad;
		}
	}
}
