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

#include <set>

#include <Core/Bitmap.h>
#include <Core/BitmapAtlasGenerator.h>
#include <Core/Debug.h>
#include <Core/Exception.h>
#include "CellToTriangle.h"
#include "GLDynamicLightShader.h"
#include "GLImage.h"
#include "GLOptimizedVoxelModel.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLRenderer.h"
#include "GLShadowMapShader.h"
#include "GLShadowShader.h"
#include "IGLShadowMapRenderer.h"

namespace spades {
	namespace draw {
		void GLOptimizedVoxelModel::PreloadShaders(spades::draw::GLRenderer *renderer) {
			renderer->RegisterProgram("Shaders/OptimizedVoxelModel.program");
			renderer->RegisterProgram("Shaders/OptimizedVoxelModelDynamicLit.program");
			renderer->RegisterProgram("Shaders/OptimizedVoxelModelShadowMap.program");
			renderer->RegisterImage("Gfx/AmbientOcclusion.png");
		}
		GLOptimizedVoxelModel::GLOptimizedVoxelModel(VoxelModel *m, GLRenderer *r) {
			SPADES_MARK_FUNCTION();

			renderer = r;
			device = r->GetGLDevice();

			BuildVertices(m);
			GenerateTexture();

			program = renderer->RegisterProgram("Shaders/OptimizedVoxelModel.program");
			dlightProgram =
			  renderer->RegisterProgram("Shaders/OptimizedVoxelModelDynamicLit.program");
			shadowMapProgram =
			  renderer->RegisterProgram("Shaders/OptimizedVoxelModelShadowMap.program");
			aoImage = (GLImage *)renderer->RegisterImage("Gfx/AmbientOcclusion.png");

			optimizedVoxelModelOutlinesProgram =
			  renderer->RegisterProgram("Shaders/OptimizedVoxelModelOutlines.program");
			optimizedVoxelModelOccludedProgram =
			  renderer->RegisterProgram("Shaders/OptimizedVoxelModelOccluded.program");
			optimizedVoxelModelOcclusionTestProgram =
			  renderer->RegisterProgram("Shaders/OptimizedVoxelModelOcclusionTest.program");

			buffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->BufferData(IGLDevice::ArrayBuffer,
			                   static_cast<IGLDevice::Sizei>(vertices.size() * sizeof(Vertex)),
			                   vertices.data(), IGLDevice::StaticDraw);

			idxBuffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, idxBuffer);
			device->BufferData(IGLDevice::ArrayBuffer,
			                   static_cast<IGLDevice::Sizei>(indices.size() * sizeof(uint32_t)),
			                   indices.data(), IGLDevice::StaticDraw);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);

			origin = m->GetOrigin();
			origin -= .5f; // (0,0,0) is center of voxel (0,0,0)

			Vector3 minPos = {0, 0, 0};
			Vector3 maxPos = {(float)m->GetWidth(), (float)m->GetHeight(), (float)m->GetDepth()};
			minPos += origin;
			maxPos += origin;
			Vector3 maxDiff = {std::max(fabsf(minPos.x), fabsf(maxPos.x)),
			                   std::max(fabsf(minPos.y), fabsf(maxPos.y)),
			                   std::max(fabsf(minPos.z), fabsf(maxPos.z))};
			radius = maxDiff.GetLength();

			boundingBox.min = minPos;
			boundingBox.max = maxPos;

			// clean up
			numIndices = (unsigned int)indices.size();
			std::vector<Vertex>().swap(vertices);
			std::vector<uint32_t>().swap(indices);
		}
		GLOptimizedVoxelModel::~GLOptimizedVoxelModel() {
			SPADES_MARK_FUNCTION();

			image->Release();
			device->DeleteBuffer(idxBuffer);
			device->DeleteBuffer(buffer);
		}

		void GLOptimizedVoxelModel::GenerateTexture() {
			BitmapAtlasGenerator atlasGen;
			std::map<Bitmap *, int> idx;
			std::vector<IntVector3> poss;
			for (size_t i = 0; i < bmps.size(); i++) {
				idx[bmps[i]] = static_cast<int>(i);
				atlasGen.AddBitmap(bmps[i]);
			}

			BitmapAtlasGenerator::Result result = atlasGen.Pack();
			Handle<Bitmap> bmp(result.bitmap, false);
			SPAssert(result.items.size() == bmps.size());
			for (size_t i = 0; i < bmps.size(); i++) {
				bmps[i]->Release();
			}
			bmps.clear();

			poss.resize(result.items.size());
			for (size_t i = 0; i < result.items.size(); i++) {
				const BitmapAtlasGenerator::Item &item = result.items[i];
				int id = idx[item.bitmap];
				poss[id] = IntVector3::Make(item.x, item.y, 0);
			}

			// move vertices' texture coord
			SPAssert(bmpIndex.size() == vertices.size());
			for (size_t i = 0; i < bmpIndex.size(); i++) {
				int id = (int)bmpIndex[i];
				Vertex &v = vertices[i];
				IntVector3 p = poss[id];
				v.u += p.x;
				v.v += p.y;
			}

			std::vector<uint16_t>().swap(bmpIndex);

			image = static_cast<GLImage *>(renderer->CreateImage(bmp));
		}

		uint8_t GLOptimizedVoxelModel::calcAOID(VoxelModel *m, int x, int y, int z, int ux, int uy,
		                                        int uz, int vx, int vy, int vz) {
			int v = 0;
			if (m->IsSolid(x - ux, y - uy, z - uz))
				v |= 1;
			if (m->IsSolid(x + ux, y + uy, z + uz))
				v |= 1 << 1;
			if (m->IsSolid(x - vx, y - vy, z - vz))
				v |= 1 << 2;
			if (m->IsSolid(x + vx, y + vy, z + vz))
				v |= 1 << 3;
			if (m->IsSolid(x - ux + vx, y - uy + vy, z - uz + vz))
				v |= 1 << 4;
			if (m->IsSolid(x - ux - vx, y - uy - vy, z - uz - vz))
				v |= 1 << 5;
			if (m->IsSolid(x + ux + vx, y + uy + vy, z + uz + vz))
				v |= 1 << 6;
			if (m->IsSolid(x + ux - vx, y + uy - vy, z + uz - vz))
				v |= 1 << 7;
			return (uint8_t)v;
		}

		template <class C> static void FreeClear(C &cntr) {
			for (typename C::iterator it = cntr.begin(); it != cntr.end(); ++it) {
				delete *it;
			}
			cntr.clear();
		}

		class GLOptimizedVoxelModel::SliceGenerator {

		public:
			// 0 - air
			// 1 - unprocessed solid
			// 2 - "inside" the processed area, but not connected
			// 3 - found holes
			uint8_t *slice;
			int usize, vsize;
			int minU, maxU, minV, maxV;

			class Model {
				const uint8_t *slice;
				int usize, vsize;

			public:
				Model(const uint8_t *slice, int usize, int vsize)
				    : slice(slice), usize(usize), vsize(vsize) {}
				inline int GetWidth() { return usize; }
				inline int GetHeight() { return vsize; }
				inline bool operator()(int x, int y) const {
					if (x < 0 || y < 0 || x >= usize || y >= vsize)
						return false;
					return slice[x * vsize + y] != 0;
				}
			};

			std::unique_ptr<c2t::Trianglulator<Model>> triangulator;

			uint8_t &Slice(int u, int v) {
				SPAssert(u >= 0);
				SPAssert(v >= 0);
				SPAssert(u < usize);
				SPAssert(v < vsize);
				return slice[u * vsize + v];
			}
			uint8_t GetSlice(int u, int v) {
				if (u < 0 || v < 0 || u >= usize || v >= vsize)
					return 0;
				return Slice(u, v);
			}

			std::vector<c2t::Point> ProcessArea() {
				if (triangulator == nullptr) {
					triangulator = decltype(triangulator){
					  new c2t::Trianglulator<Model>(Model(slice, usize, vsize))};
				}

				return triangulator->Triangulate();
			}
		};

		static IntVector3 ExactPoint(c2t::Point pt) { return IntVector3::Make(pt.x, pt.y, 0); }

		static int64_t DoubledTriangleArea(IntVector3 v1, IntVector3 v2, IntVector3 v3) {
			int64_t x1 = v1.x, y1 = v1.y;
			int64_t x2 = v2.x, y2 = v2.y;
			int64_t x3 = v3.x, y3 = v3.y;
			return (x1 - x3) * (y2 - y1) - (x1 - x2) * (y3 - y1);
		}

		void GLOptimizedVoxelModel::EmitSlice(uint8_t *slice, int usize, int vsize, int sx, int sy,
		                                      int sz, int ux, int uy, int uz, int vx, int vy,
		                                      int vz, int mx, int my, int mz, bool flip,
		                                      VoxelModel *model) {
			SPADES_MARK_FUNCTION();
			int minU = -1, minV = -1, maxU = -1, maxV = -1;

			for (int u = 0; u < usize; u++) {
				for (int v = 0; v < vsize; v++) {
					if (slice[u * vsize + v]) {
						if (minU == -1 || u < minU)
							minU = u;
						if (maxU == -1 || u > maxU)
							maxU = u;
						if (minV == -1 || v < minV)
							minV = v;
						if (maxV == -1 || v > maxV)
							maxV = v;
					}
				}
			}

			if (minU == -1) {
				// no face
				return;
			}

			int nx, ny, nz;
			nx = uy * vz - uz * vy;
			ny = uz * vx - ux * vz;
			nz = ux * vy - uy * vx;
			if (!flip) {
				nx = -nx;
				ny = -ny;
				nz = -nz;
			}

			SliceGenerator generator;
			generator.slice = slice;
			generator.usize = usize;
			generator.vsize = vsize;
			generator.minU = minU;
			generator.maxU = maxU;
			generator.minV = minV;
			generator.maxV = maxV;

			int tu = minU - 1, tv = minV - 1;
			int bw = (maxU - minU) + 3, bh = (maxV - minV) + 3;
			int bId = static_cast<int>(bmps.size());
			Bitmap *bmp = new Bitmap(bw, bh);
			bmps.push_back(bmp);
			{
				uint32_t *pixels = bmp->GetPixels();
				IntVector3 p1 = {mx, my, mz};

				IntVector3 uu = {ux, uy, uz};
				IntVector3 vv = {vx, vy, vz};
				IntVector3 nn = {nx, ny, nz};

				p1 += uu * tu;
				p1 += vv * tv;

				for (int y = 0; y < bh; y++) {
					IntVector3 p2 = p1;
					for (int x = 0; x < bw; x++) {
						IntVector3 p3 = p2;
						int u = x + tu, v = y + tv;
						if (u < 0 || v < 0 || u >= usize || v >= vsize || !slice[u * vsize + v]) {
							if ((v >= 0 && v < vsize) && u > 0 && slice[(u - 1) * vsize + (v)]) {
								u--;
								p3 -= uu;
							} else if ((v >= 0 && v < vsize) && u < usize - 1 &&
							           slice[(u + 1) * vsize + (v)]) {
								u++;
								p3 += uu;
							} else if ((u >= 0 && u < usize) && v > 0 &&
							           slice[(u)*vsize + (v - 1)]) {
								v--;
								p3 -= vv;
							} else if ((u >= 0 && u < usize) && v < vsize - 1 &&
							           slice[(u)*vsize + (v + 1)]) {
								v++;
								p3 += vv;
							} else if (u > 0 && v > 0 && slice[(u - 1) * vsize + (v - 1)]) {
								u--;
								v--;
								p3 -= uu;
								p3 -= vv;
							} else if (u > 0 && v < vsize - 1 && slice[(u - 1) * vsize + (v + 1)]) {
								u--;
								v++;
								p3 -= uu;
								p3 += vv;
							} else if (u < usize - 1 && v > 0 && slice[(u + 1) * vsize + (v - 1)]) {
								u++;
								v--;
								p3 += uu;
								p3 -= vv;
							} else if (u < usize - 1 && v < vsize - 1 &&
							           slice[(u + 1) * vsize + (v + 1)]) {
								u++;
								v++;
								p3 += uu;
								p3 += vv;
							} else {
								*(pixels++) = 0x00ff00ff;
								p2 += uu;
								continue;
							}
						}

						SPAssert(model->IsSolid(p3.x, p3.y, p3.z));
						uint32_t col = model->GetColor(p3.x, p3.y, p3.z);
						auto material = static_cast<MaterialType>(col >> 24);

						col &= 0xffffff;

						// Add AOID (selector for the pre-integrated ambient occlusion texture).
						// Use special values for certain materials.
						if (material == MaterialType::Emissive) {
							col |= ((uint32_t)255) << 24;
						} else {
							p3 += nn;
							SPAssert(!model->IsSolid(p3.x, p3.y, p3.z));

							uint8_t aoId = calcAOID(model, p3.x, p3.y, p3.z, ux, uy, uz, vx, vy, vz);

							if (aoId % 16 == 15) {
								// These AOIDs are allocated for non-default materials.
								aoId = 15;
							}

							col |= ((uint8_t)aoId) << 24;
						}

						*(pixels++) = col;

						p2 += uu;
					}

					p1 += vv;
				}
			}

			// TODO: optimize scan range
			auto polys = generator.ProcessArea();
			for (std::size_t i = 0; i < polys.size(); i += 3) {
				uint32_t idx = (uint32_t)vertices.size();
				IntVector3 pt1 = ExactPoint(polys[i + 0]);
				IntVector3 pt2 = ExactPoint(polys[i + 1]);
				IntVector3 pt3 = ExactPoint(polys[i + 2]);

				// degenerate triangle
				if (DoubledTriangleArea(pt1, pt2, pt3) == 0)
					continue;

				Vertex vtx;
				vtx.nx = nx;
				vtx.ny = ny;
				vtx.nz = nz;

				vtx.x = sx + (int)pt1.x * ux + (int)pt1.y * vx;
				vtx.y = sy + (int)pt1.x * uy + (int)pt1.y * vy;
				vtx.z = sz + (int)pt1.x * uz + (int)pt1.y * vz;
				vtx.u = (int)pt1.x - tu;
				vtx.v = (int)pt1.y - tv;
				vertices.push_back(vtx);

				vtx.x = sx + (int)pt2.x * ux + (int)pt2.y * vx;
				vtx.y = sy + (int)pt2.x * uy + (int)pt2.y * vy;
				vtx.z = sz + (int)pt2.x * uz + (int)pt2.y * vz;
				vtx.u = (int)pt2.x - tu;
				vtx.v = (int)pt2.y - tv;
				vertices.push_back(vtx);

				vtx.x = sx + (int)pt3.x * ux + (int)pt3.y * vx;
				vtx.y = sy + (int)pt3.x * uy + (int)pt3.y * vy;
				vtx.z = sz + (int)pt3.x * uz + (int)pt3.y * vz;
				vtx.u = (int)pt3.x - tu;
				vtx.v = (int)pt3.y - tv;
				vertices.push_back(vtx);

				if (!flip) {
					indices.push_back(idx + 2);
					indices.push_back(idx + 1);
					indices.push_back(idx);

				} else {
					indices.push_back(idx);
					indices.push_back(idx + 1);
					indices.push_back(idx + 2);
				}
				bmpIndex.push_back(bId);
				bmpIndex.push_back(bId);
				bmpIndex.push_back(bId);
			}
		}

		void GLOptimizedVoxelModel::BuildVertices(spades::VoxelModel *model) {
			SPADES_MARK_FUNCTION();

			SPAssert(vertices.empty());
			SPAssert(indices.empty());

			int w = model->GetWidth();
			int h = model->GetHeight();
			int d = model->GetDepth();

			std::vector<uint8_t> slice;

			// x-slice
			slice.resize(h * d);
			std::fill(slice.begin(), slice.end(), 0);
			for (int x = 0; x < w; x++) {
				for (int y = 0; y < h; y++) {
					for (int z = 0; z < d; z++) {
						uint8_t &s = slice[y * d + z];
						if (x == 0)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) && !model->IsSolid(x - 1, y, z)) ? 1 : 0;
					}
				}
				EmitSlice(slice.data(), h, d, x, 0, 0, 0, 1, 0, 0, 0, 1, x, 0, 0, false, model);

				for (int y = 0; y < h; y++) {
					for (int z = 0; z < d; z++) {
						uint8_t &s = slice[y * d + z];
						if (x == w - 1)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) && !model->IsSolid(x + 1, y, z)) ? 1 : 0;
					}
				}
				EmitSlice(slice.data(), h, d, x + 1, 0, 0, 0, 1, 0, 0, 0, 1, x, 0, 0, true, model);
			}

			// y-slice
			slice.resize(w * d);
			std::fill(slice.begin(), slice.end(), 0);
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x++) {
					for (int z = 0; z < d; z++) {
						uint8_t &s = slice[x * d + z];
						if (y == 0)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) && !model->IsSolid(x, y - 1, z)) ? 1 : 0;
					}
				}
				EmitSlice(slice.data(), w, d, 0, y, 0, 1, 0, 0, 0, 0, 1, 0, y, 0, true, model);

				for (int x = 0; x < w; x++) {
					for (int z = 0; z < d; z++) {
						uint8_t &s = slice[x * d + z];
						if (y == h - 1)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) && !model->IsSolid(x, y + 1, z)) ? 1 : 0;
					}
				}
				EmitSlice(slice.data(), w, d, 0, y + 1, 0, 1, 0, 0, 0, 0, 1, 0, y, 0, false, model);
			}

			// z-slice
			slice.resize(w * h);
			std::fill(slice.begin(), slice.end(), 0);
			for (int z = 0; z < d; z++) {
				for (int x = 0; x < w; x++) {
					for (int y = 0; y < h; y++) {
						uint8_t &s = slice[x * h + y];
						if (z == 0)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) && !model->IsSolid(x, y, z - 1)) ? 1 : 0;
					}
				}
				EmitSlice(slice.data(), w, h, 0, 0, z, 1, 0, 0, 0, 1, 0, 0, 0, z, false, model);

				for (int x = 0; x < w; x++) {
					for (int y = 0; y < h; y++) {
						uint8_t &s = slice[x * h + y];
						if (z == d - 1)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) && !model->IsSolid(x, y, z + 1)) ? 1 : 0;
					}
				}
				EmitSlice(slice.data(), w, h, 0, 0, z + 1, 1, 0, 0, 0, 1, 0, 0, 0, z, true, model);
			}

			printf("%d vertices emit\n", (int)indices.size());
		}

		void GLOptimizedVoxelModel::Prerender(std::vector<client::ModelRenderParam> params,
		                                      bool ghostPass) {
			SPADES_MARK_FUNCTION();

			RenderSunlightPass(params, ghostPass, false);
		}

		void
		GLOptimizedVoxelModel::RenderShadowMapPass(std::vector<client::ModelRenderParam> params) {
			SPADES_MARK_FUNCTION();

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			shadowMapProgram->Use();

			static GLShadowMapShader shadowMapShader;
			shadowMapShader(renderer, shadowMapProgram, 0);

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(shadowMapProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");

			positionAttribute(shadowMapProgram);
			normalAttribute(shadowMapProgram);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)0);
			if (normalAttribute() != -1) {
				device->VertexAttribPointer(normalAttribute(), 3, IGLDevice::Byte, false,
				                            sizeof(Vertex), (void *)8);
			}
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), true);
			if (normalAttribute() != -1)
				device->EnableVertexAttribArray(normalAttribute(), true);

			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (size_t i = 0; i < params.size(); i++) {
				const client::ModelRenderParam &param = params[i];

				if (!param.castShadow || param.ghost) {
					continue;
				}

				// frustrum cull
				float rad = radius;
				rad *= param.matrix.GetAxis(0).GetLength();

				if (param.depthHack)
					continue;

				if (!renderer->GetShadowMapRenderer()->SphereCull(param.matrix.GetOrigin(), rad)) {
					continue;
				}

				Matrix4 modelMatrix = param.matrix;

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(shadowMapProgram);
				modelMatrixU.SetValue(modelMatrix);

				modelMatrix.m[12] = 0.f;
				modelMatrix.m[13] = 0.f;
				modelMatrix.m[14] = 0.f;
				static GLProgramUniform modelNormalMatrix("modelNormalMatrix");
				modelNormalMatrix(shadowMapProgram);
				modelNormalMatrix.SetValue(modelMatrix);

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     (void *)0);
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), false);
			if (normalAttribute() != -1)
				device->EnableVertexAttribArray(normalAttribute(), false);

			device->ActiveTexture(0);
			device->BindTexture(IGLDevice::Texture2D, 0);
		}

		void GLOptimizedVoxelModel::RenderSunlightPass(std::vector<client::ModelRenderParam> params,
		                                               bool ghostPass, bool farRender) {
			SPADES_MARK_FUNCTION();

			bool mirror = renderer->IsRenderingMirror();

			device->ActiveTexture(0);
			aoImage->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);

			device->ActiveTexture(1);
			image->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Nearest);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			program->Use();

			static GLShadowShader shadowShader;
			shadowShader(renderer, program, 2);

			static GLProgramUniform fogDistance("fogDistance");
			fogDistance(program);
			fogDistance.SetValue(renderer->GetFogDistance());

			static GLProgramUniform fogColor("fogColor");
			fogColor(program);
			Vector3 fogCol = renderer->GetFogColorForSolidPass();
			fogCol *= fogCol; // linearize
			fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);

			static GLProgramUniform aoUniform("ambientOcclusionTexture");
			aoUniform(program);
			aoUniform.SetValue(0);

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(program);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			static GLProgramUniform texScale("texScale");
			texScale(program);
			texScale.SetValue(1.f / image->GetWidth(), 1.f / image->GetHeight());

			static GLProgramUniform modelTexture("modelTexture");
			modelTexture(program);
			modelTexture.SetValue(1);

			static GLProgramUniform sunLightDirection("sunLightDirection");
			sunLightDirection(program);
			Vector3 sunPos = MakeVector3(0, -1, -1);
			sunPos = sunPos.Normalize();
			sunLightDirection.SetValue(sunPos.x, sunPos.y, sunPos.z);

			static GLProgramUniform viewOriginVector("viewOriginVector");
			viewOriginVector(program);
			const auto &viewOrigin = renderer->GetSceneDef().viewOrigin;
			viewOriginVector.SetValue(viewOrigin.x, viewOrigin.y, viewOrigin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute textureCoordAttribute("textureCoordAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");

			positionAttribute(program);
			textureCoordAttribute(program);
			normalAttribute(program);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)0);
			device->VertexAttribPointer(textureCoordAttribute(), 2, IGLDevice::UnsignedShort, false,
			                            sizeof(Vertex), (void *)4);
			device->VertexAttribPointer(normalAttribute(), 3, IGLDevice::Byte, false,
			                            sizeof(Vertex), (void *)8);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(textureCoordAttribute(), true);
			device->EnableVertexAttribArray(normalAttribute(), true);

			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (size_t i = 0; i < params.size(); i++) {
				const client::ModelRenderParam &param = params[i];

				if (mirror && param.depthHack)
					continue;

				if (param.ghost != ghostPass) {
					continue;
				}

				// frustrum cull
				float rad = radius;
				if (!farRender) {
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				static GLProgramUniform customColor("customColor");
				customColor(program);
				customColor.SetValue(param.customColor.x, param.customColor.y, param.customColor.z);

				Matrix4 modelMatrix = param.matrix;
				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(program);
				projectionViewModelMatrix.SetValue((farRender
				                                      ? renderer->farProjectionViewMatrix
				                                      : renderer->GetProjectionViewMatrix()) *
				                                   modelMatrix);

				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				viewModelMatrix(program);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * modelMatrix);

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(program);
				modelMatrixU.SetValue(modelMatrix);

				modelMatrix.m[12] = 0.f;
				modelMatrix.m[13] = 0.f;
				modelMatrix.m[14] = 0.f;
				static GLProgramUniform modelNormalMatrix("modelNormalMatrix");
				modelNormalMatrix(program);
				modelNormalMatrix.SetValue(modelMatrix);

				static GLProgramUniform modelOpacity("modelOpacity");
				modelOpacity(program);
				modelOpacity.SetValue(param.opacity);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     (void *)0);
				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(textureCoordAttribute(), false);
			device->EnableVertexAttribArray(normalAttribute(), false);

			device->ActiveTexture(1);
			device->BindTexture(IGLDevice::Texture2D, 0);
			device->ActiveTexture(0);
			device->BindTexture(IGLDevice::Texture2D, 0);
		}

		void
		GLOptimizedVoxelModel::RenderDynamicLightPass(std::vector<client::ModelRenderParam> params,
		                                              std::vector<GLDynamicLight> lights,
		                                              bool farRender) {
			SPADES_MARK_FUNCTION();

			bool mirror = renderer->IsRenderingMirror();

			device->ActiveTexture(0);
			aoImage->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);

			device->ActiveTexture(1);
			image->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Nearest);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			dlightProgram->Use();

			static GLDynamicLightShader dlightShader;

			static GLProgramUniform fogDistance("fogDistance");
			fogDistance(dlightProgram);
			fogDistance.SetValue(renderer->GetFogDistance());

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(dlightProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			static GLProgramUniform texScale("texScale");
			texScale(dlightProgram);
			texScale.SetValue(1.f / image->GetWidth(), 1.f / image->GetHeight());

			static GLProgramUniform modelTexture("modelTexture");
			modelTexture(dlightProgram);
			modelTexture.SetValue(1);

			static GLProgramUniform viewOriginVector("viewOriginVector");
			viewOriginVector(dlightProgram);
			const auto &viewOrigin = renderer->GetSceneDef().viewOrigin;
			viewOriginVector.SetValue(viewOrigin.x, viewOrigin.y, viewOrigin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute textureCoordAttribute("textureCoordAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");

			positionAttribute(dlightProgram);
			textureCoordAttribute(dlightProgram);
			normalAttribute(dlightProgram);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)0);
			device->VertexAttribPointer(textureCoordAttribute(), 2, IGLDevice::UnsignedShort, false,
			                            sizeof(Vertex), (void *)4);
			device->VertexAttribPointer(normalAttribute(), 3, IGLDevice::Byte, false,
			                            sizeof(Vertex), (void *)8);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(textureCoordAttribute(), true);
			device->EnableVertexAttribArray(normalAttribute(), true);

			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (size_t i = 0; i < params.size(); i++) {
				const client::ModelRenderParam &param = params[i];

				if (mirror && param.depthHack)
					continue;

				if (param.ghost)
					continue;

				// frustrum cull
				float rad = radius;
				if (!farRender) {
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				static GLProgramUniform customColor("customColor");
				customColor(dlightProgram);
				customColor.SetValue(param.customColor.x, param.customColor.y, param.customColor.z);

				Matrix4 modelMatrix = param.matrix;
				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(dlightProgram);
				projectionViewModelMatrix.SetValue((farRender
				                                      ? renderer->farProjectionViewMatrix
				                                      : renderer->GetProjectionViewMatrix()) *
				                                   modelMatrix);

				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				viewModelMatrix(dlightProgram);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * modelMatrix);

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(dlightProgram);
				modelMatrixU.SetValue(modelMatrix);

				modelMatrix.m[12] = 0.f;
				modelMatrix.m[13] = 0.f;
				modelMatrix.m[14] = 0.f;
				static GLProgramUniform modelNormalMatrix("modelNormalMatrix");
				modelNormalMatrix(dlightProgram);
				modelNormalMatrix.SetValue(modelMatrix);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}
				for (size_t i = 0; i < lights.size(); i++) {
					if (!lights[i].SphereCull(param.matrix.GetOrigin(), rad))
						continue;

					dlightShader(renderer, dlightProgram, lights[i], 2);

					device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
					                     (void *)0);
				}
				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(textureCoordAttribute(), false);
			device->EnableVertexAttribArray(normalAttribute(), false);

			device->ActiveTexture(0);
		}

		void GLOptimizedVoxelModel::RenderOutlinesPass(std::vector<client::ModelRenderParam> params,
		                                               Vector3 outlineColor, bool fog,
		                                               bool farRender) {
			SPADES_MARK_FUNCTION();

			const auto mirror = renderer->IsRenderingMirror();

			device->ActiveTexture(0);
			aoImage->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);

			device->ActiveTexture(1);
			image->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Nearest);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			optimizedVoxelModelOutlinesProgram->Use();

			static GLProgramUniform fogColor("fogColor");
			fogColor(optimizedVoxelModelOutlinesProgram);
			auto fogCol = renderer->GetFogColorForSolidPass();
			if (!fog) {
				fogCol = outlineColor;
			}
			fogCol *= fogCol; // linearize
			fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);

			static GLProgramUniform outlineColorUniform("outlineColor");
			outlineColorUniform(optimizedVoxelModelOutlinesProgram);
			outlineColor *= outlineColor;
			outlineColorUniform.SetValue(outlineColor.x, outlineColor.y, outlineColor.z);

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(optimizedVoxelModelOutlinesProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			static GLProgramAttribute positionAttribute("positionAttribute");
			positionAttribute(optimizedVoxelModelOutlinesProgram);

			static GLProgramUniform viewOriginVector("viewOriginVector");
			viewOriginVector(optimizedVoxelModelOutlinesProgram);
			const auto &viewOrigin = renderer->GetSceneDef().viewOrigin;
			viewOriginVector.SetValue(viewOrigin.x, viewOrigin.y, viewOrigin.z);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), nullptr);

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (auto &param : params) {
				if (mirror && param.depthHack)
					continue;

				// frustrum cull
				if (!farRender) {
					auto rad = radius;
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				auto modelMatrix = param.matrix;

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(optimizedVoxelModelOutlinesProgram);
				modelMatrixU.SetValue(modelMatrix);

				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(optimizedVoxelModelOutlinesProgram);
				const auto &pvMat = farRender
				                    ? renderer->farProjectionViewMatrix
				                    : renderer->GetProjectionViewMatrix();
				projectionViewModelMatrix.SetValue(pvMat * modelMatrix);

				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				viewModelMatrix(optimizedVoxelModelOutlinesProgram);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * modelMatrix);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     nullptr);

				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), false);
		}

		void GLOptimizedVoxelModel::RenderOccludedPass(std::vector<client::ModelRenderParam> params,
		                                               bool farRender) {
			SPADES_MARK_FUNCTION();

			const auto mirror = renderer->IsRenderingMirror();

			device->ActiveTexture(0);
			aoImage->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);

			device->ActiveTexture(1);
			image->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Nearest);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			optimizedVoxelModelOccludedProgram->Use();

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(optimizedVoxelModelOccludedProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			positionAttribute(optimizedVoxelModelOccludedProgram);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), nullptr);

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (auto &param : params) {
				if (mirror && param.depthHack)
					continue;

				// frustrum cull
				if (!farRender) {
					auto rad = radius;
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				auto modelMatrix = param.matrix;
				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(optimizedVoxelModelOccludedProgram);
				const auto &pvMat = farRender
				                    ? renderer->farProjectionViewMatrix
					                : renderer->GetProjectionViewMatrix();
				projectionViewModelMatrix.SetValue(pvMat * modelMatrix);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     nullptr);
				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), false);
		}

		void
		GLOptimizedVoxelModel::RenderOcclusionTestPass(std::vector<client::ModelRenderParam> params,
		                                               bool farRender) {
			SPADES_MARK_FUNCTION();

			const auto mirror = renderer->IsRenderingMirror();

			device->ActiveTexture(0);
			aoImage->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);

			device->ActiveTexture(1);
			image->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Nearest);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			optimizedVoxelModelOcclusionTestProgram->Use();

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(optimizedVoxelModelOcclusionTestProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			positionAttribute(optimizedVoxelModelOcclusionTestProgram);

			static GLProgramUniform viewOriginVector("viewOriginVector");
			viewOriginVector(optimizedVoxelModelOcclusionTestProgram);
			const auto &viewOrigin = renderer->GetSceneDef().viewOrigin;
			viewOriginVector.SetValue(viewOrigin.x, viewOrigin.y, viewOrigin.z);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), nullptr);

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (auto &param : params) {
				if (mirror && param.depthHack)
					continue;

				// frustrum cull
				if (!farRender) {
					auto rad = radius;
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				auto modelMatrix = param.matrix;

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(optimizedVoxelModelOcclusionTestProgram);
				modelMatrixU.SetValue(modelMatrix);

				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(optimizedVoxelModelOcclusionTestProgram);
				const auto &pvMat = farRender
				                    ? renderer->farProjectionViewMatrix
					                : renderer->GetProjectionViewMatrix();
				projectionViewModelMatrix.SetValue(pvMat * modelMatrix);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     nullptr);
				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), false);
		}
	}
}
