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

#include "SWModelRenderer.h"
#include "SWModel.h"
#include "SWRenderer.h"
#include "SWUtils.h"
#include <Core/Bitmap.h>
#include <Core/Debug.h>
#include <Core/Math.h>

namespace spades {
	namespace draw {
		SWModelRenderer::SWModelRenderer(SWRenderer *r, SWFeatureLevel level)
		    : r(r), level(level) {}

		SWModelRenderer::~SWModelRenderer() {}

		struct ZVals {
			float zv[512];
			ZVals() {
				for (int i = 0; i < 512; i++)
					zv[i] = static_cast<float>(i);
			}
			inline float operator[](size_t idx) const { return zv[idx]; }
		};
		static ZVals zvals;

		template <SWFeatureLevel lvl>
		void SWModelRenderer::RenderInner(spades::draw::SWModel &model,
		                                  const client::ModelRenderParam &param) {
			auto &mat = param.matrix;
			auto origin = mat.GetOrigin();
			auto axis1 = mat.GetAxis(0);
			auto axis2 = mat.GetAxis(1);
			auto axis3 = mat.GetAxis(2);
			auto &rawModel = model.GetRawModel();
			auto rawModelOrigin = rawModel.GetOrigin();
			rawModelOrigin += 0.25f;
			origin += axis1 * rawModelOrigin.x;
			origin += axis2 * rawModelOrigin.y;
			origin += axis3 * rawModelOrigin.z;

			int w = rawModel.GetWidth();
			int h = rawModel.GetHeight();
			// int d = rawModel.GetDepth();

			// evaluate brightness for each normals
			uint8_t brights[3 * 3 * 3 + 1];
			{
				auto lightVec = MakeVector3(0.f, -0.707f, -0.707f);
				float dot1 = Vector3::Dot(axis1, lightVec) * fastRSqrt(axis1.GetPoweredLength());
				float dot2 = Vector3::Dot(axis2, lightVec) * fastRSqrt(axis2.GetPoweredLength());
				float dot3 = Vector3::Dot(axis3, lightVec) * fastRSqrt(axis3.GetPoweredLength());
				for (int x = 0; x < 3; x++) {
					float d = 0.0;
					int cnt = 0;
					switch (x) {
						case 0:
							d = -dot1;
							cnt = 1;
							break;
						case 1:
							d = 0.f;
							cnt = 0;
							break;
						case 2:
							d = dot1;
							cnt = 1;
							break;
					}
					for (int y = 0; y < 3; y++) {
						auto d2 = d;
						auto cnt2 = cnt;
						switch (y) {
							case 0:
								d2 -= dot2;
								cnt2++;
								break;
							case 1: break;
							case 2:
								d2 += dot2;
								cnt2++;
								break;
						}
						for (int z = 0; z < 3; z++) {
							auto d3 = d;
							auto cnt3 = cnt2;
							switch (y) {
								case 0:
									d3 -= dot3;
									cnt3++;
									break;
								case 1: break;
								case 2:
									d3 += dot3;
									cnt3++;
									break;
							}
							switch (cnt3) {
								case 2: d3 *= 0.707f; break;
								case 3: d3 *= 0.57735f; break;
							}
							d3 = 192.f + d3 * 62.f;
							brights[x + y * 3 + z * 9] = static_cast<uint8_t>(d3);
						}
					}
				}
			}

			// "emissive" material
			brights[27] = 255;

			// compute center coord. for culling
			{
				auto center = origin;
				auto localCenter = model.GetCenter();
				center += axis1 * localCenter.x;
				center += axis2 * localCenter.y;
				center += axis3 * localCenter.z;

				float largestAxis = axis1.GetPoweredLength();
				largestAxis = std::max(largestAxis, axis2.GetPoweredLength());
				largestAxis = std::max(largestAxis, axis3.GetPoweredLength());

				if (!r->SphereFrustrumCull(center, model.GetRadius() * sqrtf(largestAxis)))
					return;
			}

			Bitmap &fbmp = *r->fb;
			auto *fb = fbmp.GetPixels();
			int fw = fbmp.GetWidth();
			int fh = fbmp.GetHeight();
			auto *db = r->depthBuffer.data();

			Matrix4 viewproj = r->GetProjectionViewMatrix();
			Vector4 ndc2scrscale = {fw * 0.5f, -fh * 0.5f, 1.f, 1.f};
			// Vector4 ndc2scroff = {fw * 0.5f, fh * 0.5f, 0.f, 0.f};
			int ndc2scroffX = fw >> 1;
			int ndc2scroffY = fh >> 1;

			// render each points
			auto tOrigin = viewproj * MakeVector4(origin.x, origin.y, origin.z, 1.f);
			auto tAxis1 = viewproj * MakeVector4(axis1.x, axis1.y, axis1.z, 0.f);
			auto tAxis2 = viewproj * MakeVector4(axis2.x, axis2.y, axis2.z, 0.f);
			auto tAxis3 = viewproj * MakeVector4(axis3.x, axis3.y, axis3.z, 0.f);
			tOrigin *= ndc2scrscale;
			tAxis1 *= ndc2scrscale;
			tAxis2 *= ndc2scrscale;
			tAxis3 *= ndc2scrscale;

			float pointDiameter; // = largestAxis * 0.55f * fh * 0.5f;
			{
				float largestAxis = tAxis1.GetPoweredLength();
				largestAxis = std::max(largestAxis, tAxis2.GetPoweredLength());
				largestAxis = std::max(largestAxis, tAxis3.GetPoweredLength());
				pointDiameter = sqrtf(largestAxis);
			}

			uint32_t customColor;
			customColor = ToFixed8(param.customColor.z) | (ToFixed8(param.customColor.y) << 8) |
			              (ToFixed8(param.customColor.x) << 16);

			auto v1 = tOrigin;
			float zNear = r->sceneDef.zNear;
			for (int x = 0; x < w; x++) {
				auto v2 = v1;
				for (int y = 0; y < h; y++) {
					auto *mp = &model.renderData[model.renderDataAddr[x + y * w]];
					while (*mp != -1) {
						uint32_t data = *(mp++);
						uint32_t normal = *(mp++);
						int z = static_cast<int>(data >> 24);
						// SPAssert(z < d);
						SPAssert(z >= 0);

						auto vv = v2 + tAxis3 * zvals[z];
						if (vv.z < zNear)
							continue;

						// save Z value (don't divide this by W!)
						float zval = vv.z;

						// use vv.z for point radius to be divided by W
						vv.z = pointDiameter;

						// perspective division
						float scl = fastRcp(vv.w);
						vv *= scl;

						int ix = static_cast<int>(vv.x) + ndc2scroffX;
						int iy = static_cast<int>(vv.y) + ndc2scroffY;
						int idm = static_cast<int>(vv.z + .99f);
						idm = std::max(1, idm);
						int minX = ix - (idm >> 1);
						int minY = iy - (idm >> 1);
						if (minX >= fw || minY >= fh)
							continue;
						int maxX = ix + idm;
						int maxY = iy + idm;
						if (maxX <= 0 || maxY <= 0)
							continue;

						minX = std::max(minX, 0);
						minY = std::max(minY, 0);
						maxX = std::min(maxX, fw);
						maxY = std::min(maxY, fh);

						auto *fb2 = fb + (minX + minY * fw);
						auto *db2 = db + (minX + minY * fw);
						int w = maxX - minX;

						uint32_t color = data & 0xffffff;
						if (color == 0)
							color = customColor;

						SPAssert(normal < 28);
						int bright = brights[normal];
#if ENABLE_SSE2
						if constexpr (lvl == SWFeatureLevel::SSE2) {
							auto m = _mm_setr_epi32(color, 0, 0, 0);
							auto f = _mm_set1_epi16(bright << 8);

							m = _mm_unpacklo_epi8(m, _mm_setzero_si128());
							m = _mm_mulhi_epu16(m, f);
							m = _mm_packus_epi16(m, m);

							_mm_store_ss(reinterpret_cast<float *>(&color), _mm_castsi128_ps(m));
						} else
#endif
						{
							uint32_t c1 = color & 0xff00;
							uint32_t c2 = color & 0xff00ff;
							c1 *= bright;
							c2 *= bright;
							color = ((c1 & 0xff0000) | (c2 & 0xff00ff00)) >> 8;
						}

						for (int yy = minY; yy < maxY; yy++) {
							auto *fb3 = fb2;
							auto *db3 = db2;

							for (int xx = w; xx > 0; xx--) {
								if (zval < *db3) {
									*db3 = zval;
									*fb3 = color;
								}
								fb3++;
								db3++;
							}

							fb2 += fw;
							db2 += fw;
						}
					}
					v2 += tAxis2;
				}
				v1 += tAxis1;
			}
		}

		void SWModelRenderer::Render(spades::draw::SWModel &model,
		                             const client::ModelRenderParam &param) {
#if ENABLE_SSE2
			if (static_cast<int>(level) >= static_cast<int>(SWFeatureLevel::SSE2)) {
				RenderInner<SWFeatureLevel::SSE2>(model, param);
			} else
#endif
				RenderInner<SWFeatureLevel::None>(model, param);
		}
	} // namespace draw
} // namespace spades
