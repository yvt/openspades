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

#include <atomic>
#include <cstdlib>

#include "GLAmbientShadowRenderer.h"
#include "GLProfiler.h"
#include "GLRenderer.h"
#include <Client/GameMap.h>

#include <Core/ConcurrentDispatch.h>

namespace spades {
	namespace draw {
		class GLAmbientShadowRenderer::UpdateDispatch : public ConcurrentDispatch {
			GLAmbientShadowRenderer &renderer;

		public:
			std::atomic<bool> done{false};
			UpdateDispatch(GLAmbientShadowRenderer &r) : renderer(r) {}
			void Run() override {
				SPADES_MARK_FUNCTION();

				renderer.UpdateDirtyChunks();

				done = true;
			}
		};

		GLAmbientShadowRenderer::GLAmbientShadowRenderer(GLRenderer &r, client::GameMap &m)
		    : renderer(r), device(r.GetGLDevice()), map(m) {
			SPADES_MARK_FUNCTION();

			for (auto &rayDir : rays) {
				Vector3 dir =
				  MakeVector3(SampleRandomFloat(), SampleRandomFloat(), SampleRandomFloat());
				dir = dir.Normalize();
				dir += 0.01f;
				rayDir = dir;
			}

			w = map->Width();
			h = map->Height();
			d = map->Depth();

			chunkW = w / ChunkSize;
			chunkH = h / ChunkSize;
			chunkD = d / ChunkSize;

			chunks = std::vector<Chunk>{static_cast<std::size_t>(chunkW * chunkH * chunkD)};

			for (Chunk &c : chunks) {
				float *data = (float *)c.data;
				std::fill(data, data + ChunkSize * ChunkSize * ChunkSize * 2, 1.f);
			}

			for (int x = 0; x < chunkW; x++) {
				for (int y = 0; y < chunkH; y++) {
					for (int z = 0; z < chunkD; z++) {
						Chunk &c = GetChunk(x, y, z);
						c.cx = x;
						c.cy = y;
						c.cz = z;
					}
				}
			}

			SPLog("Chunk buffer allocated (%d bytes)",
			      (int)sizeof(Chunk) * chunkW * chunkH * chunkD);

			// make texture
			texture = device.GenTexture();
			device.BindTexture(IGLDevice::Texture3D, texture);
			device.TexParamater(IGLDevice::Texture3D, IGLDevice::TextureMagFilter,
			                    IGLDevice::Linear);
			device.TexParamater(IGLDevice::Texture3D, IGLDevice::TextureMinFilter,
			                    IGLDevice::Linear);
			device.TexParamater(IGLDevice::Texture3D, IGLDevice::TextureWrapS, IGLDevice::Repeat);
			device.TexParamater(IGLDevice::Texture3D, IGLDevice::TextureWrapT, IGLDevice::Repeat);
			device.TexParamater(IGLDevice::Texture3D, IGLDevice::TextureWrapR,
			                    IGLDevice::ClampToEdge);
			device.TexImage3D(IGLDevice::Texture3D, 0, IGLDevice::RG, w, h, d + 1, 0, IGLDevice::RG,
			                  IGLDevice::FloatType, NULL);

			SPLog("Chunk texture allocated");

			std::vector<float> v;
			v.resize(w * h * 2);
			std::fill(v.begin(), v.end(), 1.f);
			for (int i = 0; i < d + 1; i++) {
				device.TexSubImage3D(IGLDevice::Texture3D, 0, 0, 0, i, w, h, 1, IGLDevice::RG,
				                     IGLDevice::FloatType, v.data());
			}

			SPLog("Chunk texture initialized");

			dispatch = NULL;
		}

		GLAmbientShadowRenderer::~GLAmbientShadowRenderer() {
			SPADES_MARK_FUNCTION();
			if (dispatch) {
				dispatch->Join();
				delete dispatch;
			}
			device.DeleteTexture(texture);
		}

		/**
		 * Evaluate the AO term at the point specified by given world coordinates.
		 */
		float GLAmbientShadowRenderer::Evaluate(IntVector3 ipos) {
			SPADES_MARK_FUNCTION_DEBUG();

			float sum = 0.0f;
			Vector3 pos = MakeVector3((float)ipos.x, (float)ipos.y, (float)ipos.z);
			pos.x += 0.5f;
			pos.y += 0.5f;
			pos.z += 0.5f;

			for (int i = 0; i < NumRays; i++) {
				Vector3 dir = rays[i];

				unsigned int bits = i & 7;
				if (bits & 1)
					dir.x = -dir.x;
				if (bits & 2)
					dir.y = -dir.y;
				if (bits & 4)
					dir.z = -dir.z;

				Vector3 muzzle = pos;
				IntVector3 hitBlock;

				float brightness = 1.f;
				if (map->CastRay(muzzle, dir, (float)RayLength, hitBlock)) {
					Vector3 centerPos =
					  MakeVector3(hitBlock.x + .5f, hitBlock.y + .5f, hitBlock.z + .5f);
					float dist = (centerPos - muzzle).GetPoweredLength();
					brightness = dist * (1.0 / float((RayLength - 1) * (RayLength - 1)));
					if (brightness > 1.f)
						brightness = 1.f;
				}

				sum += brightness;
			}

			sum = std::min(sum * (2.f / (float)NumRays), 1.0f);

			return sum;
		}

		void GLAmbientShadowRenderer::GameMapChanged(int x, int y, int z, client::GameMap *map) {
			SPADES_MARK_FUNCTION_DEBUG();
			if (map != this->map.GetPointerOrNull()) {
				return;
			}

			Invalidate(x - RayLength, y - RayLength, z - RayLength, x + RayLength, y + RayLength,
			           z + RayLength);
		}

		void GLAmbientShadowRenderer::Invalidate(int minX, int minY, int minZ, int maxX, int maxY,
		                                         int maxZ) {
			SPADES_MARK_FUNCTION_DEBUG();
			if (minZ < 0) {
				minZ = 0;
			}
			if (maxZ > d - 1) {
				maxZ = d - 1;
			}
			if (minX > maxX || minY > maxY || minZ > maxZ) {
				return;
			}

			// these should be floor div
			int cx1 = minX >> ChunkSizeBits;
			int cy1 = minY >> ChunkSizeBits;
			int cz1 = minZ >> ChunkSizeBits;
			int cx2 = maxX >> ChunkSizeBits;
			int cy2 = maxY >> ChunkSizeBits;
			int cz2 = maxZ >> ChunkSizeBits;

			for (int cx = cx1; cx <= cx2; cx++) {
				for (int cy = cy1; cy <= cy2; cy++) {
					for (int cz = cz1; cz <= cz2; cz++) {
						Chunk &c = GetChunkWrapped(cx, cy, cz);
						int originX = cx * ChunkSize;
						int originY = cy * ChunkSize;
						int originZ = cz * ChunkSize;

						int inMinX = std::max(minX - originX, 0);
						int inMinY = std::max(minY - originY, 0);
						int inMinZ = std::max(minZ - originZ, 0);
						int inMaxX = std::min(maxX - originX, ChunkSize - 1);
						int inMaxY = std::min(maxY - originY, ChunkSize - 1);
						int inMaxZ = std::min(maxZ - originZ, ChunkSize - 1);

						if (!c.dirty) {
							c.dirtyMinX = inMinX;
							c.dirtyMinY = inMinY;
							c.dirtyMinZ = inMinZ;
							c.dirtyMaxX = inMaxX;
							c.dirtyMaxY = inMaxY;
							c.dirtyMaxZ = inMaxZ;
							c.dirty = true;
						} else {
							c.dirtyMinX = std::min(inMinX, c.dirtyMinX);
							c.dirtyMinY = std::min(inMinY, c.dirtyMinY);
							c.dirtyMinZ = std::min(inMinZ, c.dirtyMinZ);
							c.dirtyMaxX = std::max(inMaxX, c.dirtyMaxX);
							c.dirtyMaxY = std::max(inMaxY, c.dirtyMaxY);
							c.dirtyMaxZ = std::max(inMaxZ, c.dirtyMaxZ);
						}
					}
				}
			}
		}

		int GLAmbientShadowRenderer::GetNumDirtyChunks() {
			return (int)std::count_if(chunks.begin(), chunks.end(),
			                          [](const Chunk &c) { return c.dirty; });
		}

		void GLAmbientShadowRenderer::Update() {
			if (GetNumDirtyChunks() > 0 && (dispatch == NULL || dispatch->done)) {
				if (dispatch) {
					dispatch->Join();
					delete dispatch;
				}
				dispatch = new UpdateDispatch(*this);
				dispatch->Start();
			}

			// Count the number of chunks that need to be uploaded to GPU.
			// This value is approximate but it should be okay for profiling use
			std::size_t numChunksToLoad = std::count_if(
			  chunks.begin(), chunks.end(), [](const Chunk &c) { return !c.transferDone.load(); });
			GLProfiler::Context profiler{renderer.GetGLProfiler(),
			                             "Large Ambient Occlusion [>= %d chunk(s)]",
			                             numChunksToLoad};

			device.BindTexture(IGLDevice::Texture3D, texture);
			for (Chunk &c : chunks) {
				if (!c.transferDone.exchange(true)) {
					device.TexSubImage3D(IGLDevice::Texture3D, 0, c.cx * ChunkSize,
					                     c.cy * ChunkSize, c.cz * ChunkSize + 1, ChunkSize,
					                     ChunkSize, ChunkSize, IGLDevice::RG, IGLDevice::FloatType,
					                     c.data);
				}
			}
		}

		void GLAmbientShadowRenderer::UpdateDirtyChunks() {
			std::array<std::size_t, 256> dirtyChunkIds;
			std::size_t numDirtyChunks = 0;
			int nearDirtyChunks = 0;

			// first, check only chunks in near range
			Vector3 eyePos = renderer.GetSceneDef().viewOrigin;
			int eyeX = (int)(eyePos.x) >> ChunkSizeBits;
			int eyeY = (int)(eyePos.y) >> ChunkSizeBits;
			int eyeZ = (int)(eyePos.z) >> ChunkSizeBits;

			for (size_t i = 0; i < chunks.size(); i++) {
				Chunk &c = chunks[i];
				int dx = (c.cx - eyeX) & (chunkW - 1);
				int dy = (c.cy - eyeY) & (chunkH - 1);
				int dz = (c.cz - eyeZ);
				if (dx >= 6 && dx <= chunkW - 6)
					continue;
				if (dy >= 6 && dy <= chunkW - 6)
					continue;
				if (dz >= 6 || dz <= -6)
					continue;
				if (c.dirty) {
					dirtyChunkIds[numDirtyChunks++] = static_cast<int>(i);
					nearDirtyChunks++;
					if (numDirtyChunks >= dirtyChunkIds.size())
						break;
				}
			}

			// far chunks
			if (numDirtyChunks == 0) {
				for (size_t i = 0; i < chunks.size(); i++) {
					Chunk &c = chunks[i];
					if (c.dirty) {
						dirtyChunkIds[numDirtyChunks++] = static_cast<int>(i);
						if (numDirtyChunks >= dirtyChunkIds.size())
							break;
					}
				}
			}

			// limit update count per frame
			for (int i = 0; i < 8; i++) {
				if (numDirtyChunks <= 0)
					break;
				std::size_t idx = SampleRandomInt(std::size_t{0}, numDirtyChunks - 1);
				Chunk &c = chunks[dirtyChunkIds[idx]];

				// remove from list (fast)
				if (idx < numDirtyChunks - 1) {
					std::swap(dirtyChunkIds[idx], dirtyChunkIds[numDirtyChunks - 1]);
				}
				numDirtyChunks--;

				UpdateChunk(c.cx, c.cy, c.cz);
			}
			/*
			printf("%d (%d near) chunk update left\n",
			       GetNumDirtyChunks(), nearDirtyChunks);*/
		}

		void GLAmbientShadowRenderer::UpdateChunk(int cx, int cy, int cz) {
			Chunk &c = GetChunk(cx, cy, cz);
			if (!c.dirty)
				return;

			int originX = cx * ChunkSize;
			int originY = cy * ChunkSize;
			int originZ = cz * ChunkSize;

			// Compute the slightly larger volume for blurring
			constexpr int padding = 2;
			float wData[ChunkSize + padding * 2][ChunkSize + padding * 2][ChunkSize + padding * 2]
			           [2];
			std::uint8_t wFlags[ChunkSize + padding * 2][ChunkSize + padding * 2]
			                   [ChunkSize + padding * 2];
			int wOriginX = originX - padding;
			int wOriginY = originY - padding;
			int wOriginZ = originZ - padding;
			int wDirtyMinX = c.dirtyMinX;
			int wDirtyMinY = c.dirtyMinY;
			int wDirtyMinZ = c.dirtyMinZ;
			int wDirtyMaxX = c.dirtyMaxX + padding * 2;
			int wDirtyMaxY = c.dirtyMaxY + padding * 2;
			int wDirtyMaxZ = c.dirtyMaxZ + padding * 2;

			auto b = [](int i) -> std::uint8_t { return (std::uint8_t)1 << i; };
			auto to_b = [](bool b, int i) -> std::uint8_t { return (std::uint8_t)b << i; };

			for (int z = wDirtyMinZ; z <= wDirtyMaxZ; z++)
				for (int y = wDirtyMinY; y <= wDirtyMaxY; y++)
					for (int x = wDirtyMinX; x <= wDirtyMaxX; x++) {
						IntVector3 pos{
						  x + wOriginX,
						  y + wOriginY,
						  z + wOriginZ,
						};

						if (map->IsSolidWrapped(pos.x, pos.y, pos.z)) {
							wData[z][y][x][0] = 0.0;
							wData[z][y][x][1] = 0.0;
						} else {
							wData[z][y][x][0] = Evaluate(pos);
							wData[z][y][x][1] = 1.0;
						}
						// bit 0: solids
						// bit 1: contact (by-surface voxel)
						wFlags[z][y][x] =
						  to_b(map->IsSolidWrapped(pos.x, pos.y, pos.z), 0) |
						  to_b(map->IsSolidWrapped(pos.x - 1, pos.y - 1, pos.z - 1) |
						         map->IsSolidWrapped(pos.x - 1, pos.y - 1, pos.z) |
						         map->IsSolidWrapped(pos.x - 1, pos.y - 1, pos.z + 1) |
						         map->IsSolidWrapped(pos.x - 1, pos.y, pos.z - 1) |
						         map->IsSolidWrapped(pos.x - 1, pos.y, pos.z) |
						         map->IsSolidWrapped(pos.x - 1, pos.y, pos.z + 1) |
						         map->IsSolidWrapped(pos.x - 1, pos.y + 1, pos.z - 1) |
						         map->IsSolidWrapped(pos.x - 1, pos.y + 1, pos.z) |
						         map->IsSolidWrapped(pos.x - 1, pos.y + 1, pos.z + 1) |
						         map->IsSolidWrapped(pos.x - 1, pos.y - 1, pos.z - 1) |
						         map->IsSolidWrapped(pos.x, pos.y - 1, pos.z) |
						         map->IsSolidWrapped(pos.x, pos.y - 1, pos.z + 1) |
						         map->IsSolidWrapped(pos.x, pos.y, pos.z - 1) |
						         map->IsSolidWrapped(pos.x, pos.y, pos.z + 1) |
						         map->IsSolidWrapped(pos.x, pos.y + 1, pos.z - 1) |
						         map->IsSolidWrapped(pos.x, pos.y + 1, pos.z) |
						         map->IsSolidWrapped(pos.x, pos.y + 1, pos.z + 1) |
						         map->IsSolidWrapped(pos.x + 1, pos.y - 1, pos.z - 1) |
						         map->IsSolidWrapped(pos.x + 1, pos.y - 1, pos.z) |
						         map->IsSolidWrapped(pos.x + 1, pos.y - 1, pos.z + 1) |
						         map->IsSolidWrapped(pos.x + 1, pos.y, pos.z - 1) |
						         map->IsSolidWrapped(pos.x + 1, pos.y, pos.z) |
						         map->IsSolidWrapped(pos.x + 1, pos.y, pos.z + 1) |
						         map->IsSolidWrapped(pos.x + 1, pos.y + 1, pos.z - 1) |
						         map->IsSolidWrapped(pos.x + 1, pos.y + 1, pos.z) |
						         map->IsSolidWrapped(pos.x + 1, pos.y + 1, pos.z + 1),
						       1);
					}

			// The AO terms are sampled 0.5 blocks away from the terrain surface,
			// which leads to under-shadowing. Compensate for this effect.
			for (int z = wDirtyMinZ; z <= wDirtyMaxZ; z++)
				for (int y = wDirtyMinY; y <= wDirtyMaxY; y++)
					for (int x = wDirtyMinX; x <= wDirtyMaxX; x++) {
						float &d = wData[z][y][x][0];
						d *= d * d + 1.0f - d;
					}

			// Blur the result to remove noise
			//
			//	  |     this        |     neighbor    |
			//	  | solid | contact | solid | contact | blur
			//	  |   0        0    |   0        x    |   1
			//	  |   0        1    |   0        0    |   0  (prevent under-shadowing)
			//	  |   0        1    |   0        1    |   1
			//	  |   0        x    |   1        x    |   0  (solid voxel's value is zero)
			//	  |   1        x    |   0        x    |   0  (solid voxel's value must remain zero)
			//	  |   1        x    |   1        x    |   x
			//
			//
			//	             this voxel
			//
			//	                    solid
			//	                  /-------\  				.
			//	          +---+---+---+---+
			//	          | 1 | 0 | 0 | 0 |
			//	          +---+---+---+---+\				.
			//	          | 1 | 1 | 0 | 0 | |
			//	         /+---+---+---+---+ | contact  neighbor
			//	        | | 0 | 0 |   |   | |
			//	  solid | +---+---+---+---+/
			//	        | | 0 | 0 |   |   |
			//	         \+---+---+---+---+
			//	              \-------/
			//	               contact
			//
			static const float divider[] = {1.0f, 1.0f / 2.0f, 1.0f / 3.0f};
			auto mask = [](bool b, float x) { return b ? x : 0.0f; };
			auto shouldBlur = [=](std::uint8_t thisFlags, std::uint8_t neighborFlags) {
				return ((neighborFlags & b(0)) | ((~thisFlags | neighborFlags) & b(1))) == 0b10;
			};
			for (int blurPass = 0; blurPass < 2; ++blurPass) {
				for (int z = wDirtyMinZ; z <= wDirtyMaxZ; z++)
					for (int y = wDirtyMinY; y <= wDirtyMaxY; y++)
						for (int x = wDirtyMinX + 1; x < wDirtyMaxX; x++) {
							if (wFlags[z][y][x] & b(0)) {
								continue;
							}
							// Do not blur between by-surface voxels and
							// in-the-air voxels
							bool m1 = shouldBlur(wFlags[z][y][x], wFlags[z][y][x - 1]);
							bool m2 = shouldBlur(wFlags[z][y][x], wFlags[z][y][x + 1]);
							wData[z][y][x][0] =
							  (wData[z][y][x][0] + mask(m1, wData[z][y][x - 1][0]) +
							   mask(m2, wData[z][y][x + 1][0])) *
							  divider[(int)m1 + (int)m2];
						}
				for (int z = wDirtyMinZ; z <= wDirtyMaxZ; z++)
					for (int y = wDirtyMinY + 1; y < wDirtyMaxY; y++)
						for (int x = wDirtyMinX; x <= wDirtyMaxX; x++) {
							if (wFlags[z][y][x] & b(0)) {
								continue;
							}
							bool m1 = shouldBlur(wFlags[z][y][x], wFlags[z][y - 1][x]);
							bool m2 = shouldBlur(wFlags[z][y][x], wFlags[z][y + 1][x]);
							wData[z][y][x][0] =
							  (wData[z][y][x][0] + mask(m1, wData[z][y - 1][x][0]) +
							   mask(m2, wData[z][y + 1][x][0])) *
							  divider[(int)m1 + (int)m2];
						}
				for (int z = wDirtyMinZ + 1; z < wDirtyMaxZ; z++)
					for (int y = wDirtyMinY; y <= wDirtyMaxY; y++)
						for (int x = wDirtyMinX; x <= wDirtyMaxX; x++) {
							if (wFlags[z][y][x] & b(0)) {
								continue;
							}
							bool m1 = shouldBlur(wFlags[z][y][x], wFlags[z - 1][y][x]);
							bool m2 = shouldBlur(wFlags[z][y][x], wFlags[z + 1][y][x]);
							wData[z][y][x][0] =
							  (wData[z][y][x][0] + mask(m1, wData[z - 1][y][x][0]) +
							   mask(m2, wData[z + 1][y][x][0])) *
							  divider[(int)m1 + (int)m2];
						}
			}

			// Copy the result to `c.data`
			for (int z = c.dirtyMinZ; z <= c.dirtyMaxZ; z++)
				for (int y = c.dirtyMinY; y <= c.dirtyMaxY; y++)
					for (int x = c.dirtyMinX; x <= c.dirtyMaxX; x++) {
						c.data[z][y][x][0] = wData[z + padding][y + padding][x + padding][0];
						c.data[z][y][x][1] = wData[z + padding][y + padding][x + padding][1];
					}

			c.dirty = false;
			c.transferDone = false;
		}
	} // namespace draw
} // namespace spades
