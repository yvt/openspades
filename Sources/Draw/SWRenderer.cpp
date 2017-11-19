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

#include <algorithm>
#include <array>
#include <cfenv>
#include <cstdlib>

#include "SWFlatMapRenderer.h"
#include "SWImage.h"
#include "SWImageRenderer.h"
#include "SWMapRenderer.h"
#include "SWModel.h"
#include "SWModelRenderer.h"
#include "SWPort.h"
#include "SWRenderer.h"
#include <Client/GameMap.h>
#include <Core/Bitmap.h>
#include <Core/Settings.h>

#include "SWUtils.h"

DEFINE_SPADES_SETTING(r_swStatistics, "0");
DEFINE_SPADES_SETTING(r_swNumThreads, "4");

namespace spades {
	namespace draw {
		SWRenderer::SWRenderer(SWPort *port, SWFeatureLevel level)
		    : featureLevel(level),
		      port(port),
		      map(nullptr),
		      fb(nullptr),
		      inited(false),
		      sceneUsedInThisFrame(false),
		      fogDistance(128.f),
		      fogColor(MakeVector3(0.f, 0.f, 0.f)),
		      drawColorAlphaPremultiplied(MakeVector4(1, 1, 1, 1)),
		      legacyColorPremultiply(false),
		      lastTime(0),
		      duringSceneRendering(false) {

			SPADES_MARK_FUNCTION();

			if (port == nullptr) {
				SPRaise("Port is null.");
			}

			SPLog("---- SWRenderer early initialization started ---");

#ifdef FE_DFL_DISABLE_SSE_DENORMS_ENV
			SPLog("initializing FPU");
			fesetenv(FE_DFL_DISABLE_SSE_DENORMS_ENV);
#endif

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
			depthBuffer.resize(fb->GetWidth() * fb->GetHeight());
			imageRenderer->SetDepthBuffer(depthBuffer.data());

			SPLog("---- SWRenderer early initialization done ---");
		}

		SWRenderer::~SWRenderer() {
			SPADES_MARK_FUNCTION();

			Shutdown();
		}

		void SWRenderer::SetFramebuffer(spades::Bitmap *bmp) {
			if (bmp == nullptr) {
				SPRaise("Framebuffer is null.");
			}
			if (fb) {
				SPAssert(bmp->GetWidth() == fb->GetWidth());
				SPAssert(bmp->GetHeight() == fb->GetHeight());
			}
			fb = bmp;
			imageRenderer->SetFramebuffer(bmp);
			if ((bmp->GetWidth() & 7) || (bmp->GetHeight() & 7)) {
				SPRaise("Framebuffer size is not multiple of 8.");
			}
		}

		void SWRenderer::Init() {
			SPADES_MARK_FUNCTION();

			SPLog("---- SWRenderer late initialization started ---");
			modelManager = std::make_shared<SWModelManager>();

			SPLog("creating model renderer");
			modelRenderer = std::make_shared<SWModelRenderer>(this, featureLevel);
			renderStopwatch.Reset();

			SPLog("---- SWRenderer late initialization done ---");

			inited = true;
		}

		void SWRenderer::Shutdown() {
			SPADES_MARK_FUNCTION();

			SetGameMap(nullptr);

			imageRenderer.reset();
			flatMapRenderer.reset();

			imageManager.reset();
			modelManager.reset();
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
			if (map)
				EnsureInitialized();
			if (map == this->map)
				return;

			flatMapRenderer.reset();
			mapRenderer.reset();

			if (this->map)
				this->map->RemoveListener(this);
			this->map = map;
			if (this->map) {
				this->map->AddListener(this);
				flatMapRenderer = std::make_shared<SWFlatMapRenderer>(this, map);
				mapRenderer = std::make_shared<SWMapRenderer>(this, map, featureLevel);
			}
		}

		void SWRenderer::SetFogColor(spades::Vector3 v) { fogColor = v; }

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
			frustrum[0] = Plane3::PlaneWithPointOnPlane(sceneDef.viewOrigin, sceneDef.viewAxis[2]);
			frustrum[1] = frustrum[0].Flipped();
			frustrum[0].w -= sceneDef.zNear;
			frustrum[1].w += sceneDef.zFar;

			float xCos = cosf(sceneDef.fovX * .5f);
			float xSin = sinf(sceneDef.fovX * .5f);
			float yCos = cosf(sceneDef.fovY * .5f);
			float ySin = sinf(sceneDef.fovY * .5f);

			frustrum[2] = Plane3::PlaneWithPointOnPlane(
			  sceneDef.viewOrigin, sceneDef.viewAxis[2] * xSin - sceneDef.viewAxis[0] * xCos);
			frustrum[3] = Plane3::PlaneWithPointOnPlane(
			  sceneDef.viewOrigin, sceneDef.viewAxis[2] * xSin + sceneDef.viewAxis[0] * xCos);
			frustrum[4] = Plane3::PlaneWithPointOnPlane(
			  sceneDef.viewOrigin, sceneDef.viewAxis[2] * ySin - sceneDef.viewAxis[1] * yCos);
			frustrum[5] = Plane3::PlaneWithPointOnPlane(
			  sceneDef.viewOrigin, sceneDef.viewAxis[2] * ySin + sceneDef.viewAxis[1] * yCos);
		}

		template <SWFeatureLevel> void SWRenderer::ApplyDynamicLight(const DynamicLight &light) {
			int fw = this->fb->GetWidth();
			int fh = this->fb->GetHeight();

			float fovX = tanf(sceneDef.fovX * 0.5f);
			float fovY = tanf(sceneDef.fovY * 0.5f);

			float dvx = -fovX * 2.f / static_cast<float>(fw);
			float dvy = -fovY * 2.f / static_cast<float>(fh);

			int minX = light.minX;
			int minY = light.minY;
			int maxX = light.maxX;
			int maxY = light.maxY;
			int lightHeight = maxY - minY;

			SPAssert(minX >= 0);
			SPAssert(minY >= 0);
			SPAssert(maxX <= fw);
			SPAssert(maxY <= fh);

			Vector3 lightCenter;
			Vector3 diff = light.param.origin - sceneDef.viewOrigin;
			lightCenter.x = Vector3::Dot(diff, sceneDef.viewAxis[0]);
			lightCenter.y = Vector3::Dot(diff, sceneDef.viewAxis[1]);
			lightCenter.z = Vector3::Dot(diff, sceneDef.viewAxis[2]);

			int lightR = ToFixedFactor8(light.param.color.x);
			int lightG = ToFixedFactor8(light.param.color.y);
			int lightB = ToFixedFactor8(light.param.color.z);

			float invRadius2 = 1.f / (light.param.radius * light.param.radius);

			InvokeParallel2([=](unsigned int threadId, unsigned int numThreads) {
				int startY = lightHeight * threadId / numThreads;
				int endY = lightHeight * (threadId + 1) / numThreads;
				startY += minY;
				endY += minY;

				auto *fb = this->fb->GetPixels();
				float *db = depthBuffer.data();
				fb += startY * fw + minX;
				db += startY * fw + minX;

				float vy = fovY + dvy * startY;
				float vx = fovX + dvx * minX;

				int lightWidth = maxX - minX;

				for (int y = startY; y < endY; y++) {
					float vx2 = vx;
					auto *fb2 = fb;
					auto *db2 = db;

					for (int x = lightWidth; x > 0; x--) {
						Vector3 pos;

						pos.z = *db2;
						pos.x = vx2 * pos.z;
						pos.y = vy * pos.z;

						pos -= lightCenter;

						float dist = pos.GetPoweredLength();
						dist *= invRadius2;

						if (dist < 1.f) {
							float strength = 1.f - dist;
							strength *= strength;
							strength *= 256.f;

							int factor = static_cast<int>(strength);

							int actualLightR = lightR * factor;
							int actualLightG = lightG * factor;
							int actualLightB = lightB * factor;

							auto srcColor = *fb2;
							auto srcColorR = (srcColor >> 16) & 0xff;
							auto srcColorG = (srcColor >> 8) & 0xff;
							auto srcColorB = srcColor & 0xff;

							actualLightR *= srcColorR;
							actualLightG *= srcColorG;
							actualLightB *= srcColorB;

							auto destColorR = actualLightR >> 16;
							auto destColorG = actualLightG >> 16;
							auto destColorB = actualLightB >> 16;

							destColorR = std::min<uint32_t>(destColorR + srcColorR, 255);
							destColorG = std::min<uint32_t>(destColorG + srcColorG, 255);
							destColorB = std::min<uint32_t>(destColorB + srcColorB, 255);

							uint32_t destColor =
							  destColorB | (destColorG << 8) | (destColorR << 16);

							*fb2 = destColor;
						}

						vx2 += dvx;
						fb2++;
						db2++;
					}

					vy += dvy;
					fb += fw;
					db += fw;
				}
			});
		}

		template <SWFeatureLevel level> void SWRenderer::ApplyFog() {
			int fw = this->fb->GetWidth();
			int fh = this->fb->GetHeight();

			float fovX = tanf(sceneDef.fovX * 0.5f);
			float fovY = tanf(sceneDef.fovY * 0.5f);

			float dvx = -fovX * 2.f / static_cast<float>(fw / 4);
			float dvy = -fovY * 2.f / static_cast<float>(fh / 4);

			int fogR = ToFixed8(fogColor.x);
			int fogG = ToFixed8(fogColor.y);
			int fogB = ToFixed8(fogColor.z);
			uint32_t fog1 = static_cast<uint32_t>(fogB + fogR * 0x10000);
			uint32_t fog2 = static_cast<uint32_t>(fogG * 0x100);

			float scale = 255.f / fogDistance;

			InvokeParallel2([&](unsigned int threadId, unsigned int numThreads) {
				int startY = fh * threadId / numThreads;
				int endY = fh * (threadId + 1) / numThreads;
				startY &= ~3;
				endY &= ~3;

				float vy = fovY;
				auto *fb = this->fb->GetPixels();
				float *db = depthBuffer.data();

				vy += dvy * (startY >> 2);
				fb += fw * startY;
				db += fw * startY;

				for (int y = startY; y < endY; y += 4) {
					float vx = fovX;

					for (int x = 0; x < fw; x += 4) {
						float depthScale = (1.f + vx * vx + vy * vy);
						depthScale *= fastRSqrt(depthScale) * scale;
						auto *fb2 = fb + x;
						auto *db2 = db + x;
						for (int by = 0; by < 4; by++) {
							auto *fb3 = fb2;
							auto *db3 = db2;

							for (int bx = 0; bx < 4; bx++) {

								float dist = *db3 * depthScale;
								int factor = std::min(static_cast<int>(dist), 256);
								factor = std::max(0, factor);
								int factor2 = 256 - factor;

								uint32_t color = *fb3;
								uint32_t v1 = (color & 0xff00ff) * factor2;
								uint32_t v2 = (color & 0x00ff00) * factor2;
								v1 += fog1 * factor;
								v2 += fog2 * factor;
								v1 &= 0xff00ff00;
								v2 &= 0xff0000;
								*fb3 = (v1 | v2) >> 8;

								fb3++;
								db3++;
							}

							fb2 += fw;
							db2 += fw;
						}

						vx += dvx;
					}

					vy += dvy;
					fb += fw * 4;
					db += fw * 4;
				}
			});

		} // ApplyFog()

#if ENABLE_SSE2

		template <> void SWRenderer::ApplyFog<SWFeatureLevel::SSE2>() {
			int fw = this->fb->GetWidth();
			int fh = this->fb->GetHeight();

			float fovX = tanf(sceneDef.fovX * 0.5f);
			float fovY = tanf(sceneDef.fovY * 0.5f);

			float dvx = -fovX * 2.f / static_cast<float>(fw / 4);
			float dvy = -fovY * 2.f / static_cast<float>(fh / 4);

			int fogR = ToFixed8(fogColor.x);
			int fogG = ToFixed8(fogColor.y);
			int fogB = ToFixed8(fogColor.z);
			__m128i fog = _mm_setr_epi16(fogB, fogG, fogR, 0, fogB, fogG, fogR, 0);

			float scale = 255.f / fogDistance;

			InvokeParallel2([&](unsigned int threadId, unsigned int numThreads) {
				int startY = fh * threadId / numThreads;
				int endY = fh * (threadId + 1) / numThreads;
				startY &= ~3;
				endY &= ~3;

				float vy = fovY;
				auto *fb = this->fb->GetPixels();
				float *db = depthBuffer.data();

				vy += dvy * (startY >> 2);
				fb += fw * startY;
				db += fw * startY;

				for (int y = startY; y < endY; y += 4) {
					float vx = fovX;

					for (int x = 0; x < fw; x += 4) {
						float depthScale = (1.f + vx * vx + vy * vy);
						depthScale *= fastRSqrt(depthScale) * scale;
						auto depthScale4 = _mm_set1_ps(depthScale);

						auto *fb2 = fb + x;
						auto *db2 = db + x;
						for (int by = 0; by < 4; by++) {
							auto *fb3 = fb2;
							auto *db3 = db2;

							auto dist = _mm_load_ps(db3);
							auto color = _mm_load_si128(reinterpret_cast<__m128i *>(fb3));

							dist = _mm_mul_ps(dist, depthScale4);
							dist = _mm_max_ps(dist, _mm_set1_ps(0.f));
							dist = _mm_min_ps(dist, _mm_set1_ps(256.f));
							auto factorX = _mm_cvtps_epi32(dist);

							auto factorY = _mm_sub_epi32(_mm_set1_epi32(0x100), factorX);

							factorX = _mm_shufflelo_epi16(factorX, 0xa0);
							factorX = _mm_shufflehi_epi16(factorX, 0xa0);
							factorY = _mm_shufflelo_epi16(factorY, 0xa0);
							factorY = _mm_shufflehi_epi16(factorY, 0xa0);

							// first 2px
							auto color1 = _mm_unpacklo_epi8(color, _mm_setzero_si128());
							auto factor1X = _mm_shuffle_epi32(factorY, 0x50);
							auto factor1Y = _mm_shuffle_epi32(factorX, 0x50);
							color1 = _mm_mullo_epi16(color1, factor1X);
							auto fog1 = _mm_mullo_epi16(fog, factor1Y);
							fog1 = _mm_adds_epu16(fog1, color1);
							fog1 = _mm_srli_epi16(fog1, 8);

							// next 2px
							auto color2 = _mm_unpackhi_epi8(color, _mm_setzero_si128());
							auto factor2X = _mm_shuffle_epi32(factorY, 0xfa);
							auto factor2Y = _mm_shuffle_epi32(factorX, 0xfa);
							color2 = _mm_mullo_epi16(color2, factor2X);
							auto fog2 = _mm_mullo_epi16(fog, factor2Y);
							fog2 = _mm_adds_epu16(fog2, color2);
							fog2 = _mm_srli_epi16(fog2, 8);

							auto pack = _mm_packus_epi16(fog1, fog2);
							_mm_store_si128(reinterpret_cast<__m128i *>(fb3), pack);

							fb2 += fw;
							db2 += fw;
						}

						vx += dvx;
					}

					vy += dvy;
					fb += fw * 4;
					db += fw * 4;
				}
			});

		} // ApplyFog()

#endif

		void SWRenderer::EnsureSceneStarted() {
			SPADES_MARK_FUNCTION_DEBUG();
			if (!duringSceneRendering) {
				SPRaise("Illegal call outside of StartScene ... EndScene");
			}
		}

		void SWRenderer::EnsureSceneNotStarted() {
			SPADES_MARK_FUNCTION_DEBUG();
			if (duringSceneRendering) {
				SPRaise("Illegal call between StartScene ... EndScene");
			}
		}

		void SWRenderer::EnsureInitialized() {
			SPADES_MARK_FUNCTION_DEBUG();
			if (!inited) {
				SPRaise("Renderer is not initialized");
			}
			EnsureValid();
		}

		void SWRenderer::EnsureValid() {
			SPADES_MARK_FUNCTION_DEBUG();
			if (!port) {
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

			projectionViewMatrix = projectionMatrix * viewMatrix;
		}

		void SWRenderer::RenderModel(client::IModel *model, const client::ModelRenderParam &param) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			EnsureSceneStarted();

			auto *mdl = dynamic_cast<SWModel *>(model);
			if (mdl == nullptr)
				SPInvalidArgument("model");

			Model m;
			m.model = mdl;
			m.param = param;

			models.push_back(m);
		}

		void SWRenderer::AddLight(const client::DynamicLightParam &param) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			EnsureSceneStarted();

			if (param.type != client::DynamicLightTypePoint) {
				// TODO: support non-point lights
				return;
			}

			auto diff = param.origin - sceneDef.viewOrigin;
			float rad2 = param.radius * param.radius;
			float poweredLength = diff.GetPoweredLength();

			float fogCullRange = param.radius + fogDistance;
			if (poweredLength > fogCullRange * fogCullRange) {
				// fog cull
				return;
			}

			DynamicLight light;
			if (poweredLength < rad2) {
				light.minX = 0;
				light.minY = 0;
				light.maxX = fb->GetWidth();
				light.maxY = fb->GetHeight();
			} else if (Vector3::Dot(diff, sceneDef.viewAxis[2]) < 0.f) {
				// view plane cull
				return;
			} else {
				auto viewRange = [](float cx, float cy, float fov,
				                    float screenSize) -> std::array<int, 2> {
					auto trans = [screenSize, fov](float v) {
						v = (v / fov) * 0.5f + 0.5f;
						v = std::max(v, 0.f);
						v = std::min(v, 1.f);
						v *= screenSize;
						return static_cast<int>(v);
					};
					auto dist = cx * cx + cy * cy - 1.f;
					if (dist <= 0.f) {
						return std::array<int, 2>{{0, static_cast<int>(screenSize)}};
					}

					auto denom = cx * cx - 1.f;
					if (fabsf(denom) < 1.e-10f) {
						denom = 1.e-8f;
					}
					denom = 1.f / denom;

					dist = sqrtf(dist);

					if (cx <= 1.f) {
						if (cy > 0.f) {
							return std::array<int, 2>{
							  {trans(cx * cy - dist), static_cast<int>(screenSize)}};
						} else {
							return std::array<int, 2>{{0, trans(cx * cy + dist)}};
						}
					} else {
						return std::array<int, 2>{{trans(cx * cy - dist), trans(cx * cy + dist)}};
					}
				};
				auto invRad = 1.f / param.radius;
				auto rangeX = viewRange(Vector3::Dot(diff, sceneDef.viewAxis[2]) * invRad,
				                        Vector3::Dot(diff, sceneDef.viewAxis[0]) * invRad,
				                        tanf(sceneDef.fovX * 0.5f), ScreenWidth());
				auto rangeY = viewRange(Vector3::Dot(diff, sceneDef.viewAxis[1]) * invRad,
				                        Vector3::Dot(diff, sceneDef.viewAxis[0]) * invRad,
				                        tanf(sceneDef.fovY * 0.5f), ScreenHeight());
				light.minX = rangeX[0];
				light.maxX = rangeX[1];
				light.minY = rangeY[0];
				light.maxY = rangeY[1];
			}

			light.param = param;
			lights.push_back(light);
		}

		void SWRenderer::AddDebugLine(spades::Vector3 a, spades::Vector3 b, spades::Vector4 color) {
			EnsureInitialized();
			EnsureSceneStarted();

			DebugLine l = {a, b, color};
			debugLines.push_back(l);
		}

		void SWRenderer::AddSprite(client::IImage *image, spades::Vector3 center, float radius,
		                           float rotation) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			EnsureSceneStarted();

			if (!SphereFrustrumCull(center, radius * 1.5f))
				return;

			SWImage *img = dynamic_cast<SWImage *>(image);
			if (!img) {
				SPInvalidArgument("image");
			}

			sprites.push_back(Sprite());
			auto &spr = sprites.back();

			spr.img = img;
			spr.center = center;
			spr.radius = radius;
			spr.rotation = rotation;
			spr.color = drawColorAlphaPremultiplied;
		}

		void SWRenderer::AddLongSprite(client::IImage *, spades::Vector3 p1, spades::Vector3 p2,
		                               float radius) {
			SPADES_MARK_FUNCTION();
			EnsureInitialized();
			EnsureSceneStarted();
			// TODO: long sprite
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
			std::fill(fb->GetPixels(), fb->GetPixels() + fb->GetWidth() * fb->GetHeight(),
			          ConvertColor32(MakeVector4(fogColor.x, fogColor.y, fogColor.z, 1.f)));
			std::fill(fb->GetPixels(), fb->GetPixels() + fb->GetWidth() * fb->GetHeight(),
			          0x7f7f7f);

			// draw map
			if (mapRenderer) {
				// flat map renderer sends 'Update RLE' to map renderer.
				// rendering map before this leads to the corrupted renderer image.
				flatMapRenderer->Update();
				mapRenderer->Render(sceneDef, fb, depthBuffer.data());
			}

			// draw models
			for (auto &m : models) {
				modelRenderer->Render(m.model, m.param);
			}
			models.clear();

			// deferred lighting
			for (const auto &light : lights) {
				ApplyDynamicLight<SWFeatureLevel::None>(light);
			}
			lights.clear();

#if ENABLE_SSE2
			if (static_cast<int>(featureLevel) >= static_cast<int>(SWFeatureLevel::SSE2))
				ApplyFog<SWFeatureLevel::SSE2>();
			else
#endif
				ApplyFog<SWFeatureLevel::None>();

			// render sprites
			{
				imageRenderer->SetShaderType(SWImageRenderer::ShaderType::Sprite);
				imageRenderer->SetMatrix(projectionViewMatrix);
				imageRenderer->SetZRange(sceneDef.zNear, sceneDef.zFar);

				auto right = sceneDef.viewAxis[0];
				auto up = sceneDef.viewAxis[1];
				for (std::size_t i = 0; i < sprites.size(); i++) {
					auto &spr = sprites[i];
					float s = sinf(spr.rotation) * spr.radius;
					float c = cosf(spr.rotation) * spr.radius;
					auto trans = [s, c, &spr, right, up](float x, float y) {
						auto v = spr.center;
						v += right * (c * x - s * y);
						v += up * (s * x + c * y);
						return MakeVector4(v.x, v.y, v.z, 1.f);
					};
					auto x1 = trans(-1.f, -1.f);
					auto x2 = trans(1.f, -1.f);
					auto x3 = trans(-1.f, 1.f);
					auto x4 = trans(1.f, 1.f);
					SWImageRenderer::Vertex v1, v2, v3;
					v1.color = v2.color = v3.color = spr.color;
					v1.uv = MakeVector2(0.f, 0.f);
					v1.position = x1;
					v2.uv = MakeVector2(1.f, 0.f);
					v2.position = x2;
					v3.uv = MakeVector2(0.f, 1.f);
					v3.position = x3;
					imageRenderer->DrawPolygon(spr.img, v1, v2, v3);
					v1.uv = MakeVector2(1.f, 0.f);
					v1.position = x2;
					v2.uv = MakeVector2(1.f, 1.f);
					v2.position = x4;
					v3.uv = MakeVector2(0.f, 1.f);
					v3.position = x3;
					imageRenderer->DrawPolygon(spr.img, v1, v2, v3);
				}
				sprites.clear();
			}

			// render debug lines
			{
				float cw = fb->GetWidth() * 0.5f;
				float ch = fb->GetHeight() * 0.5f;
				for (size_t i = 0; i < debugLines.size(); i++) {
					auto &l = debugLines[i];
					auto v1 = projectionViewMatrix * l.v1;
					auto v2 = projectionViewMatrix * l.v2;
					if (v1.z < 0.001f || v2.z < 0.001f)
						continue;

					// SWRenderer's depth value is based on view Z coord
					float d1 = Vector3::Dot(l.v1 - sceneDef.viewOrigin, sceneDef.viewAxis[2]);
					float d2 = Vector3::Dot(l.v2 - sceneDef.viewOrigin, sceneDef.viewAxis[2]);
					d1 = fastRcp(d1);
					d2 = fastRcp(d2);

					v1 *= fastRcp(v1.w);
					v2 *= fastRcp(v2.w);

					int x1 = static_cast<int>(v1.x * cw + cw);
					int y1 = static_cast<int>(ch - v1.y * ch);
					int x2 = static_cast<int>(v2.x * cw + cw);
					int y2 = static_cast<int>(ch - v2.y * ch);

					auto *fb = this->fb->GetPixels();
					auto *db = this->depthBuffer.data();
					int fw = this->fb->GetWidth();
					int fh = this->fb->GetHeight();

					uint32_t col;
					col = ToFixed8(l.color.z) | (ToFixed8(l.color.y) << 8) |
					      (ToFixed8(l.color.x) << 16);

					if (x1 == x2 && y1 == y2) {
						if (x1 >= 0 && y1 >= 0 && x1 < fw && y1 < fh) {
							d1 = fastRcp(d1);
							if (d1 < db[x1 + y1 * fw]) {
								fb[x1 + y1 * fw] = col;
							}
						}
						continue;
					}

					if (abs(x2 - x1) > abs(y2 - y1)) {
						if (x1 >= x2) {
							std::swap(x1, x2);
							std::swap(y1, y2);
							std::swap(d1, d2);
						}
						int sgn = (y2 > y1) ? 1 : -1;
						int cy = y1;
						int dy = abs(y2 - y1);
						int fract = 0;
						int divisor = x2 - x1;
						int minX = std::max(x1, 0);
						int maxX = std::min(x2, fw - 1);
						float depth = d1, ddepth = (d2 - d1) * fastRcp(x2 - x1 + 1);
						if (x1 < 0) {
							long long v = dy;
							v *= -x1;
							cy += sgn * (v / divisor);
							fract = v % divisor;
							depth -= x1 * ddepth;
						}
						fb += minX;
						db += minX;
						for (int x = minX; x <= maxX; x++) {
							if (cy >= 0 && cy < fh) {
								float d = fastRcp(depth);
								if (d < db[fw * cy]) {
									fb[fw * cy] = col;
								}
							}
							fract += dy;
							if (fract >= divisor) {
								cy += sgn;
								fract -= divisor;
							}
							depth += ddepth;
							fb++;
							db++;
						}
					} else {
						if (y1 >= y2) {
							std::swap(x1, x2);
							std::swap(y1, y2);
							std::swap(d1, d2);
						}
						int sgn = (x2 > x1) ? 1 : -1;
						int cx = x1;
						int dx = abs(x2 - x1);
						int fract = 0;
						int divisor = y2 - y1;
						int minY = std::max(y1, 0);
						int maxY = std::min(y2, fh - 1);
						float depth = d1, ddepth = (d2 - d1) * fastRcp(y2 - y1 + 1);
						if (y1 < 0) {
							long long v = dx;
							v *= -y1;
							cx += sgn * (v / divisor);
							fract = v % divisor;
							depth -= x1 * ddepth;
						}
						fb += minY * fw;
						db += minY * fw;
						for (int y = minY; y <= maxY; y++) {
							if (cx >= 0 && cx < fw) {
								float d = fastRcp(depth);
								if (d < db[cx]) {
									fb[cx] = col;
								}
							}
							fract += dx;
							if (fract >= divisor) {
								cx += sgn;
								fract -= divisor;
							}
							depth += ddepth;
							fb += fw;
							db += fw;
						}
					}
				}
				debugLines.clear();
			}

			// all objects were rendered

			duringSceneRendering = false;
		}

		void SWRenderer::MultiplyScreenColor(spades::Vector3 v) { EnsureSceneNotStarted(); }

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

		void SWRenderer::DrawImage(client::IImage *image, const spades::Vector2 &outTopLeft) {
			SPADES_MARK_FUNCTION();

			if (image == nullptr) {
				SPRaise("Size must be specified when null image is provided");
			}

			DrawImage(image,
			          AABB2(outTopLeft.x, outTopLeft.y, image->GetWidth(), image->GetHeight()),
			          AABB2(0, 0, image->GetWidth(), image->GetHeight()));
		}

		void SWRenderer::DrawImage(client::IImage *image, const spades::AABB2 &outRect) {
			SPADES_MARK_FUNCTION();

			DrawImage(image, outRect,
			          AABB2(0, 0, image ? image->GetWidth() : 0, image ? image->GetHeight() : 0));
		}

		void SWRenderer::DrawImage(client::IImage *image, const spades::Vector2 &outTopLeft,
		                           const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();

			DrawImage(image,
			          AABB2(outTopLeft.x, outTopLeft.y, inRect.GetWidth(), inRect.GetHeight()),
			          inRect);
		}

		void SWRenderer::DrawImage(client::IImage *image, const spades::AABB2 &outRect,
		                           const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();

			DrawImage(image, Vector2::Make(outRect.GetMinX(), outRect.GetMinY()),
			          Vector2::Make(outRect.GetMaxX(), outRect.GetMinY()),
			          Vector2::Make(outRect.GetMinX(), outRect.GetMaxY()), inRect);
		}

		void SWRenderer::DrawImage(client::IImage *image, const spades::Vector2 &outTopLeft,
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
			if (img == nullptr && image != nullptr) {
				// not SWImage
				SPInvalidArgument("image");
			}

			imageRenderer->SetShaderType(SWImageRenderer::ShaderType::Image);

			Vector4 col = drawColorAlphaPremultiplied;
			if (legacyColorPremultiply) {
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

			vtx[0].position = MakeVector4(outTopLeft.x, outTopLeft.y, 1.f, 1.f);
			vtx[1].position = MakeVector4(outTopRight.x, outTopRight.y, 1.f, 1.f);
			vtx[2].position = MakeVector4(outBottomLeft.x, outBottomLeft.y, 1.f, 1.f);
			vtx[3].position = MakeVector4(outBottomRight.x, outBottomRight.y, 1.f, 1.f);
			if (img) {
				Vector2 scl = {img->GetInvWidth(), img->GetInvHeight()};
				vtx[0].uv = MakeVector2(inRect.min.x, inRect.min.y) * scl;
				vtx[1].uv = MakeVector2(inRect.max.x, inRect.min.y) * scl;
				vtx[2].uv = MakeVector2(inRect.min.x, inRect.max.y) * scl;
				vtx[3].uv = MakeVector2(inRect.max.x, inRect.max.y) * scl;
			}

			imageRenderer->DrawPolygon(img, vtx[0], vtx[1], vtx[2]);
			imageRenderer->DrawPolygon(img, vtx[1], vtx[3], vtx[2]);
		}

		void SWRenderer::DrawFlatGameMap(const spades::AABB2 &outRect,
		                                 const spades::AABB2 &inRect) {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			EnsureSceneNotStarted();

			if (!flatMapRenderer) {
				SPRaise("DrawFlatGameMap was called without an active map.");
			}

			DrawImage(flatMapRenderer->GetImage(), outRect, inRect);
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

			if (r_swStatistics) {
				double dur = renderStopwatch.GetTime();
				SPLog("==== SWRenderer Statistics ====");
				SPLog("Elapsed Time: %.3fus", dur * 1000000.0);
				SPLog("Polygon pixels drawn: %llu", imageRenderer->GetPixelsDrawn());
			}

			imageRenderer->ResetPixelStatistics();
			renderStopwatch.Reset();
			/*
			{
			    uint32_t rdb = mt_engine();
			    uint32_t *ptr = fb->GetPixels();
			    for(int pixels = fb->GetWidth() * fb->GetHeight() / 10;
			        pixels > 0; pixels--) {
			        *ptr = ((rdb >> 16) & 0xff) * 0x10101;
			        rdb = (rdb * 34563511) ^ 1245525;
			        rdb += (pixels >> 10) + 1;
			        ptr++;
			    }
			}
			*/
			port->Swap();

			// next frame's framebuffer
			SetFramebuffer(port->GetFramebuffer());
		}

		Bitmap *SWRenderer::ReadBitmap() {
			SPADES_MARK_FUNCTION();
			EnsureValid();
			EnsureSceneNotStarted();

			int w = fb->GetWidth();
			int h = fb->GetHeight();
			uint32_t *inPix = fb->GetPixels();
			Bitmap *bm = new Bitmap(w, h);
			uint32_t *outPix = bm->GetPixels();
			for (int y = 0; y < h; y++) {
				uint32_t *src = inPix + y * w;
				uint32_t *dest = outPix + (h - 1 - y) * w;
				for (int x = w; x != 0; x--) {
					auto c = *(src++);
					c = 0xff000000 | (c & 0xff00) | ((c & 0xff) << 16) | ((c & 0xff0000) >> 16);
					*(dest++) = c;
				}
			}
			return bm;
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

		bool SWRenderer::BoxFrustrumCull(const AABB3 &box) {
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
			return PlaneCullTest(frustrum[0], box) && PlaneCullTest(frustrum[1], box) &&
			       PlaneCullTest(frustrum[2], box) && PlaneCullTest(frustrum[3], box) &&
			       PlaneCullTest(frustrum[4], box) && PlaneCullTest(frustrum[5], box);
		}
		bool SWRenderer::SphereFrustrumCull(const Vector3 &center, float radius) {
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
			for (int i = 0; i < 6; i++) {
				if (frustrum[i].GetDistanceTo(center) < -radius)
					return false;
			}
			return true;
		}

		void SWRenderer::GameMapChanged(int x, int y, int z, client::GameMap *map) {
			if (map != this->map) {
				return;
			}

			flatMapRenderer->SetNeedsUpdate(x, y);
		}
	}
}
