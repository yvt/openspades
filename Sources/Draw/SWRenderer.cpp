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

#include "SWRenderer.h"
#include "SWPort.h"
#include "Bitmap.h"
#include "SWImage.h"
#include "SWModel.h"
#include <Client/GameMap.h>
#include <stdlib.h>
#include "SWImageRenderer.h"
#include <array>
#include <algorithm>
#include <Core/Settings.h>


SPADES_SETTING(r_swStatistics, "0");

namespace spades {
	namespace draw {
		SWRenderer::SWRenderer(SWPort *port,
							   SWFeatureLevel level):
		port(port),
		map(nullptr),
		fb(nullptr),
		inited(false),
		sceneUsedInThisFrame(false),
		fogDistance(128.f),
		fogColor(MakeVector3(0.f, 0.f, 0.f)),
		drawColorAlphaPremultiplied(MakeVector4(1,1,1,1)),
		legacyColorPremultiply(false),
		lastTime(0),
		duringSceneRendering(false),
		featureLevel(level){
			
			SPADES_MARK_FUNCTION();
			
			if(port == nullptr) {
				SPRaise("Port is null.");
			}
			
			SPLog("---- SWRenderer early initialization started ---");
			
			SPLog("creating image manager");
			imageManager = std::make_shared<SWImageManager>();
			
			SPLog("creating image renderer");
			imageRenderer = std::make_shared<SWImageRenderer>(featureLevel);
			imageRenderer->ResetPixelStatistics();
			renderStopwatch.Reset();
			
			SPLog("setting framebuffer.");
			SetFramebuffer(port->GetFramebuffer());
			
			// alloc depth buffer
			SPLog("initializing depth buffer.");
			depthBuffer.reserve(fb->GetWidth() * fb->GetHeight());
			imageRenderer->SetDepthBuffer(depthBuffer.data());
			
			SPLog("---- SWRenderer early initialization done ---");
		}
		
		SWRenderer::~SWRenderer() {
			SPADES_MARK_FUNCTION();
			
			Shutdown();
		}
		
		void SWRenderer::SetFramebuffer(spades::Bitmap *bmp) {
			if(bmp == nullptr) {
				SPRaise("Framebuffer is null.");
			}
			if(fb) {
				SPAssert(bmp->GetWidth() == fb->GetWidth());
				SPAssert(bmp->GetHeight() == fb->GetHeight());
			}
			fb = bmp;
			imageRenderer->SetFramebuffer(bmp);
		}
		
		void SWRenderer::Init() {
			SPADES_MARK_FUNCTION();
			
			SPLog("---- SWRenderer late initialization started ---");
			modelManager = std::make_shared<SWModelManager>();
			
			
			SPLog("---- SWRenderer late initialization done ---");
			
			inited = true;
		}
		
		void SWRenderer::Shutdown() {
			SPADES_MARK_FUNCTION();
			
			imageManager.reset();
			modelManager.reset();
			if(this->map)
				this->map->SetListener(this);
			map = nullptr;
			port = nullptr;
			
			inited = false;
		}
		
		client::IImage *SWRenderer::RegisterImage(const char *filename) {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			return imageManager->RegisterImage(filename);
		}
		
		client::IModel *SWRenderer::RegisterModel(const char *filename) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			return modelManager->RegisterModel(filename);
		}
		
		client::IImage *SWRenderer::CreateImage(spades::Bitmap *bmp) {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			return imageManager->CreateImage(bmp);
		}
		
		client::IModel *SWRenderer::CreateModel(spades::VoxelModel *model) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			return modelManager->CreateModel(model);
		}
		
		void SWRenderer::SetGameMap(client::GameMap *map) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			if(this->map)
				this->map->SetListener(nullptr);
			this->map = map;
			if(this->map)
				this->map->SetListener(this);
		}
		
		void SWRenderer::SetFogColor(spades::Vector3 v) {
			fogColor = v;
		}
		
		
		void SWRenderer::BuildProjectionMatrix() {
			SPADES_MARK_FUNCTION();
			
			float near = sceneDef.zNear;
			float far = sceneDef.zFar;
			float t = near * tanf(sceneDef.fovY * .5f);
			float r = near * tanf(sceneDef.fovX * .5f);
			float a = r * 2.f, b = t * 2.f, c = far - near;
			Matrix4 mat;
			mat.m[0] = near * 2.f / a;
			mat.m[1] = 0.f;
			mat.m[2] = 0.f;
			mat.m[3] = 0.f;
			mat.m[4] = 0.f;
			mat.m[5] = near * 2.f / b;
			mat.m[6] = 0.f;
			mat.m[7] = 0.f;
			mat.m[8] = 0.f;
			mat.m[9] = 0.f;
			mat.m[10] = -(far + near) / c;
			mat.m[11] = -1.f;
			mat.m[12] = 0.f;
			mat.m[13] = 0.f;
			mat.m[14] = -(far * near * 2.f) / c;
			mat.m[15] = 0.f;
			projectionMatrix = mat;
		}
		
		void SWRenderer::BuildView() {
			SPADES_MARK_FUNCTION();
			
			Matrix4 mat = Matrix4::Identity();
			mat.m[0] = sceneDef.viewAxis[0].x;
			mat.m[4] = sceneDef.viewAxis[0].y;
			mat.m[8] = sceneDef.viewAxis[0].z;
			mat.m[1] = sceneDef.viewAxis[1].x;
			mat.m[5] = sceneDef.viewAxis[1].y;
			mat.m[9] = sceneDef.viewAxis[1].z;
			mat.m[2] = -sceneDef.viewAxis[2].x;
			mat.m[6] = -sceneDef.viewAxis[2].y;
			mat.m[10] = -sceneDef.viewAxis[2].z;
			
			Vector4 v = mat * sceneDef.viewOrigin;
			mat.m[12] = -v.x;
			mat.m[13] = -v.y;
			mat.m[14] = -v.z;
			
			viewMatrix = mat;
		}
		
		void SWRenderer::BuildFrustrum() {
			// far/near
			frustrum[0] = Plane3::PlaneWithPointOnPlane(sceneDef.viewOrigin,
														sceneDef.viewAxis[2]);
			frustrum[1] = frustrum[0].Flipped();
			frustrum[0].w -= sceneDef.zNear;
			frustrum[1].w += sceneDef.zFar;
			
			float xCos = cosf(sceneDef.fovX * .5f);
			float xSin = sinf(sceneDef.fovX * .5f);
			float yCos = cosf(sceneDef.fovY * .5f);
			float ySin = sinf(sceneDef.fovY * .5f);
			
			frustrum[2] = Plane3::PlaneWithPointOnPlane
			(sceneDef.viewOrigin,
			 sceneDef.viewAxis[2] * xSin - sceneDef.viewAxis[0] * xCos);
			frustrum[3] = Plane3::PlaneWithPointOnPlane
			(sceneDef.viewOrigin,
			 sceneDef.viewAxis[2] * xSin + sceneDef.viewAxis[0] * xCos);
			frustrum[4] = Plane3::PlaneWithPointOnPlane
			(sceneDef.viewOrigin,
			 sceneDef.viewAxis[2] * ySin - sceneDef.viewAxis[1] * yCos);
			frustrum[5] = Plane3::PlaneWithPointOnPlane
			(sceneDef.viewOrigin,
			 sceneDef.viewAxis[2] * ySin + sceneDef.viewAxis[1] * yCos);
			
		}
		
		void SWRenderer::EnsureSceneStarted() {
			SPADES_MARK_FUNCTION_DEBUG();
			if(!duringSceneRendering) {
				SPRaise("Illegal call outside of StartScene ... EndScene");
			}
		}
		
		void SWRenderer::EnsureSceneNotStarted() {
			SPADES_MARK_FUNCTION_DEBUG();
			if(duringSceneRendering) {
				SPRaise("Illegal call between StartScene ... EndScene");
			}
		}
		
		void SWRenderer::EnsureInitialized() {
			SPADES_MARK_FUNCTION_DEBUG();
			if(!inited){
				SPRaise("Renderer is not initialized");
			}
			EnsureValid();
		}
		
		void SWRenderer::EnsureValid() {
			SPADES_MARK_FUNCTION_DEBUG();
			if(!port){
				SPRaise("Renderer is not valid");
			}
		}
		
		void SWRenderer::StartScene(const client::SceneDefinition &def) {
			SPADES_MARK_FUNCTION();
			
			EnsureInitialized();
			EnsureSceneNotStarted();
			
			sceneDef = def;
			duringSceneRendering = true;
			
			BuildProjectionMatrix();
			BuildView();
			BuildFrustrum();
			
			
		}
		
		void SWRenderer::RenderModel(client::IModel *model, const client::ModelRenderParam &) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			EnsureSceneStarted();
			
		}
		
		void SWRenderer::AddLight(const client::DynamicLightParam &light) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			EnsureSceneStarted();
			
		}
		
		void SWRenderer::AddDebugLine(spades::Vector3 a, spades::Vector3 b, spades::Vector4 color) {
			EnsureInitialized();
			EnsureSceneStarted();
			
		}
		
		void SWRenderer::AddSprite(client::IImage *, spades::Vector3 center, float radius, float rotation) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			EnsureSceneStarted();
			
		}
		
		void SWRenderer::AddLongSprite(client::IImage *, spades::Vector3 p1, spades::Vector3 p2, float radius) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			EnsureSceneStarted();
			
		}
		
		static uint32_t ConvertColor32(Vector4 col) {
			auto convertColor = [](float f) {
				int i = static_cast<int>(f * 255.f + .5f);
				return static_cast<uint32_t>(std::max(std::min(i, 255), 0));
			};
			uint32_t c;
			c = convertColor(col.x);
			c |= convertColor(col.y) << 8;
			c |= convertColor(col.z) << 16;
			c |= convertColor(col.w) << 24;
			return c;
		}
		
		void SWRenderer::EndScene() {
			EnsureInitialized();
			EnsureSceneStarted();
			
			// clear scene
			std::fill(fb->GetPixels(), fb->GetPixels() +
					  fb->GetWidth() * fb->GetHeight(),
					  ConvertColor32(MakeVector4(fogColor.x, fogColor.y, fogColor.z, 1.f)));
			std::fill(fb->GetPixels(), fb->GetPixels() +
					  fb->GetWidth() * fb->GetHeight(),
					  0x7f7f7f);
			
			
			duringSceneRendering = false;
		}
		
		void SWRenderer::MultiplyScreenColor(spades::Vector3 v) {
			EnsureSceneNotStarted();
			
		}
		
		void SWRenderer::SetColor(spades::Vector4 col) {
			EnsureValid();
			drawColorAlphaPremultiplied = col;
			legacyColorPremultiply = true;
		}
		
		void SWRenderer::SetColorAlphaPremultiplied(spades::Vector4 col) {
			EnsureValid();
			legacyColorPremultiply = false;
			drawColorAlphaPremultiplied = col;
		}
		
		
		void SWRenderer::DrawImage(client::IImage *image,
								   const spades::Vector2 &outTopLeft) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  AABB2(outTopLeft.x, outTopLeft.y,
							image->GetWidth(),
							image->GetHeight()),
					  AABB2(0, 0,
							image->GetWidth(),
							image->GetHeight()));
		}
		
		void SWRenderer::DrawImage(client::IImage *image, const spades::AABB2 &outRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  outRect,
					  AABB2(0, 0,
							image->GetWidth(),
							image->GetHeight()));
		}
		
		void SWRenderer::DrawImage(client::IImage *image,
								   const spades::Vector2 &outTopLeft,
								   const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  AABB2(outTopLeft.x, outTopLeft.y,
							inRect.GetWidth(),
							inRect.GetHeight()),
					  inRect);
		}
		
		void SWRenderer::DrawImage(client::IImage *image,
								   const spades::AABB2 &outRect,
								   const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			
			DrawImage(image,
					  Vector2::Make(outRect.GetMinX(), outRect.GetMinY()),
					  Vector2::Make(outRect.GetMaxX(), outRect.GetMinY()),
					  Vector2::Make(outRect.GetMinX(), outRect.GetMaxY()),
					  inRect);
		}
		
		void SWRenderer::DrawImage(client::IImage *image,
								   const spades::Vector2 &outTopLeft,
								   const spades::Vector2 &outTopRight,
								   const spades::Vector2 &outBottomLeft,
								   const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			
			EnsureValid();
			EnsureSceneNotStarted();
			
			// d = a + (b - a) + (c - a)
			//   = b + c - a
			Vector2 outBottomRight = outTopRight + outBottomLeft - outTopLeft;
			
			SWImage *img = dynamic_cast<SWImage *>(image);
			if(!img){
				SPInvalidArgument("image");
			}
			
			imageRenderer->SetShaderType(SWImageRenderer::ShaderType::Image);
			
			Vector4 col = drawColorAlphaPremultiplied;
			if(legacyColorPremultiply) {
				// in legacy mode, image color is
				// non alpha-premultiplied.
				col.x *= col.w;
				col.y *= col.w;
				col.z *= col.w;
			}
			
			std::array<SWImageRenderer::Vertex, 4> vtx;
			vtx[0].color = col;
			vtx[1].color = col;
			vtx[2].color = col;
			vtx[3].color = col;
			
			vtx[0].position = MakeVector4(outTopLeft.x, outTopLeft.y,
										  1.f, 1.f);
			vtx[1].position = MakeVector4(outTopRight.x, outTopRight.y,
										  1.f, 1.f);
			vtx[2].position = MakeVector4(outBottomLeft.x, outBottomLeft.y,
										  1.f, 1.f);
			vtx[3].position = MakeVector4(outBottomRight.x, outBottomRight.y,
										  1.f, 1.f);
			Vector2 scl = {img->GetInvWidth(), img->GetInvHeight()};
			vtx[0].uv = MakeVector2(inRect.min.x, inRect.min.y) * scl;
			vtx[1].uv = MakeVector2(inRect.max.x, inRect.min.y) * scl;
			vtx[2].uv = MakeVector2(inRect.min.x, inRect.max.y) * scl;
			vtx[3].uv = MakeVector2(inRect.max.x, inRect.max.y) * scl;
			
			imageRenderer->DrawPolygon(img, vtx[0], vtx[1], vtx[2]);
			imageRenderer->DrawPolygon(img, vtx[1], vtx[3], vtx[2]);
			
		}

		void SWRenderer::DrawFlatGameMap(const spades::AABB2 &outRect, const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			EnsureSceneNotStarted();
			
		}
		
		void SWRenderer::FrameDone() {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			EnsureSceneNotStarted();
			
		}
		
		void SWRenderer::Flip() {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			EnsureSceneNotStarted();
			
			if(r_swStatistics) {
				double dur = renderStopwatch.GetTime();
				SPLog("==== SWRenderer Statistics ====");
				SPLog("Elapsed Time: %.3fus", dur * 1000000.0);
				SPLog("Polygon pixels drawn: %llu", imageRenderer->GetPixelsDrawn());
			}
			
			imageRenderer->ResetPixelStatistics();
			renderStopwatch.Reset();
			
			{
				uint32_t rdb = rand();
				uint32_t *ptr = fb->GetPixels();
				for(int pixels = fb->GetWidth() * fb->GetHeight() / 10;
					pixels > 0; pixels--) {
					*ptr = ((rdb >> 16) & 0xff) * 0x10101;
					rdb = (rdb * 34563511) ^ 1245525;
					rdb += (pixels >> 10) + 1;
					ptr++;
				}
			}
			
			port->Swap();
			
			// next frame's framebuffer
			SetFramebuffer(port->GetFramebuffer());
		}
		
		Bitmap *SWRenderer::ReadBitmap() {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			EnsureSceneNotStarted();
			return fb->Clone();
		}
		
		float SWRenderer::ScreenWidth() {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			return static_cast<float>(fb->GetWidth());
		}
		
		float SWRenderer::ScreenHeight() {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			return static_cast<float>(fb->GetHeight());
		}
		
		
		bool SWRenderer::BoxFrustrumCull(const AABB3& box) {
			/*if(IsRenderingMirror()) {
				// reflect
				AABB3 bx = box;
				std::swap(bx.min.z, bx.max.z);
				bx.min.z = 63.f * 2.f - bx.min.z;
				bx.max.z = 63.f * 2.f - bx.max.z;
				return PlaneCullTest(frustrum[0], bx) &&
				PlaneCullTest(frustrum[1], bx) &&
				PlaneCullTest(frustrum[2], bx) &&
				PlaneCullTest(frustrum[3], bx) &&
				PlaneCullTest(frustrum[4], bx) &&
				PlaneCullTest(frustrum[5], bx);
			}*/
			return PlaneCullTest(frustrum[0], box) &&
			PlaneCullTest(frustrum[1], box) &&
			PlaneCullTest(frustrum[2], box) &&
			PlaneCullTest(frustrum[3], box) &&
			PlaneCullTest(frustrum[4], box) &&
			PlaneCullTest(frustrum[5], box);
		}
		bool SWRenderer::SphereFrustrumCull(const Vector3& center,
											float radius) {
			/*if(IsRenderingMirror()) {
				// reflect
				Vector3 vx = center;
				vx.z = 63.f * 2.f - vx.z;
				for(int i = 0; i < 6; i++){
					if(frustrum[i].GetDistanceTo(vx) < -radius)
						return false;
				}
				return true;
			}*/
			for(int i = 0; i < 6; i++){
				if(frustrum[i].GetDistanceTo(center) < -radius)
					return false;
			}
			return true;
		}
		
		void SWRenderer::GameMapChanged(int x, int y, int z, client::GameMap *map) {
			if(map != this->map) {
				return;
			}
			
		}
	}
}

