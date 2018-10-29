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

#include <Client/GameMap.h>
#include "GLAmbientShadowRenderer.h"
#include "GLProfiler.h"
#include "GLRenderer.h"

#include <Core/ConcurrentDispatch.h>

namespace spades {
	namespace draw {
		class GLAmbientShadowRenderer::UpdateDispatch : public ConcurrentDispatch {
			GLAmbientShadowRenderer *renderer;

		public:
			std::atomic<bool> done {false};
			UpdateDispatch(GLAmbientShadowRenderer *r) : renderer(r) { }
			void Run() override {
				SPADES_MARK_FUNCTION();

				renderer->UpdateDirtyChunks();

				done = true;
			}
		};

		GLAmbientShadowRenderer::GLAmbientShadowRenderer(GLRenderer *r, client::GameMap *m)
		    : renderer(r), device(r->GetGLDevice()), map(m) {
			SPADES_MARK_FUNCTION();

			for (int i = 0; i < NumRays; i++) {
				Vector3 dir = MakeVector3(SampleRandomFloat(), SampleRandomFloat(), SampleRandomFloat());
				dir = dir.Normalize();
				dir += 0.01f;
				rays[i] = dir;
			}

			w = map->Width();
			h = map->Height();
			d = map->Depth();

			chunkW = w / ChunkSize;
			chunkH = h / ChunkSize;
			chunkD = d / ChunkSize;

			chunks = std::vector<Chunk>{static_cast<std::size_t>(chunkW * chunkH * chunkD)};

			for (size_t i = 0; i < chunks.size(); i++) {
				Chunk &c = chunks[i];
				float *data = (float *)c.data;
				std::fill(data, data + ChunkSize * ChunkSize * ChunkSize, 1.f);
			}

			for (int x = 0; x < chunkW; x++)
				for (int y = 0; y < chunkH; y++)
					for (int z = 0; z < chunkD; z++) {
						Chunk &c = GetChunk(x, y, z);
						c.cx = x;
						c.cy = y;
						c.cz = z;
					}

			SPLog("Chunk buffer allocated (%d bytes)", (int) sizeof(Chunk) * chunkW * chunkH * chunkD);

			// make texture
			texture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture3D, texture);
			device->TexParamater(IGLDevice::Texture3D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture3D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture3D, IGLDevice::TextureWrapS, IGLDevice::Repeat);
			device->TexParamater(IGLDevice::Texture3D, IGLDevice::TextureWrapT, IGLDevice::Repeat);
			device->TexParamater(IGLDevice::Texture3D, IGLDevice::TextureWrapR,
			                     IGLDevice::ClampToEdge);
			device->TexImage3D(IGLDevice::Texture3D, 0, IGLDevice::Red, w, h, d + 1, 0,
			                   IGLDevice::Red, IGLDevice::FloatType, NULL);

			SPLog("Chunk texture allocated");

			std::vector<float> v;
			v.resize(w * h);
			std::fill(v.begin(), v.end(), 1.f);
			for (int i = 0; i < d + 1; i++) {
				device->TexSubImage3D(IGLDevice::Texture3D, 0, 0, 0, i, w, h, 1, IGLDevice::Red,
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
			device->DeleteTexture(texture);
		}

		float GLAmbientShadowRenderer::Evaluate(IntVector3 ipos) {
			SPADES_MARK_FUNCTION_DEBUG();

			float sum = 0;
			Vector3 pos = MakeVector3((float)ipos.x, (float)ipos.y, (float)ipos.z);

			float muzzleDiff = 0.02f;

			// check allowed ray direction
			uint8_t directions[8] = {0, 1, 2, 3, 4, 5, 6, 7};
			int numDirections = 0;
			for (int x = -1; x <= 0; x++)
				for (int y = -1; y <= 0; y++)
					for (int z = -1; z <= 0; z++) {
						if (!map->IsSolidWrapped(ipos.x + x, ipos.y + y, ipos.z + z)) {
							unsigned int bits = 0;
							if (x)
								bits |= 1;
							if (y)
								bits |= 2;
							if (z)
								bits |= 4;
							directions[numDirections++] = bits;
						}
					}
			if (numDirections == 0)
				numDirections = 8;

			int dirId = 0;

			for (int i = 0; i < NumRays; i++) {
				Vector3 dir = rays[i];

				unsigned int bits = directions[dirId];
				if (bits & 1)
					dir.x = -dir.x;
				if (bits & 2)
					dir.y = -dir.y;
				if (bits & 4)
					dir.z = -dir.z;

				dirId++;
				if (dirId >= numDirections)
					dirId = 0;

				Vector3 muzzle = pos + dir * muzzleDiff;
				IntVector3 hitBlock;

				float brightness = 1.f;
				if (map->IsSolidWrapped((int)floorf(muzzle.x), (int)floorf(muzzle.y),
				                        (int)floorf(muzzle.z))) {
					if (numDirections < 8)
						SPAssert(false);
					continue;
				}
				if (map->CastRay(muzzle, dir, 18.f, hitBlock)) {
					Vector3 centerPos =
					  MakeVector3(hitBlock.x + .5f, hitBlock.y + .5f, hitBlock.z + .5f);
					float dist = (centerPos - muzzle).GetPoweredLength();
					brightness = dist * 0.02f; // 1/7/7
					if (brightness > 1.f)
						brightness = 1.f;
				}

				sum += brightness;
			}

			sum *= 1.f / (float)NumRays;
			sum *= (float)numDirections / 4.f;

			return sum;
		}

		void GLAmbientShadowRenderer::GameMapChanged(int x, int y, int z, client::GameMap *map) {
			SPADES_MARK_FUNCTION_DEBUG();
			if (map != this->map)
				return;

			Invalidate(x - 8, y - 8, z - 8, x + 8, y + 8, z + 8);
		}

		void GLAmbientShadowRenderer::Invalidate(int minX, int minY, int minZ, int maxX, int maxY,
		                                         int maxZ) {
			SPADES_MARK_FUNCTION_DEBUG();
			if (minZ < 0)
				minZ = 0;
			if (maxZ > d - 1)
				maxZ = d - 1;
			if (minX > maxX || minY > maxY || minZ > maxZ)
				return;

			// these should be floor div
			int cx1 = minX >> ChunkSizeBits;
			int cy1 = minY >> ChunkSizeBits;
			int cz1 = minZ >> ChunkSizeBits;
			int cx2 = maxX >> ChunkSizeBits;
			int cy2 = maxY >> ChunkSizeBits;
			int cz2 = maxZ >> ChunkSizeBits;

			for (int cx = cx1; cx <= cx2; cx++)
				for (int cy = cy1; cy <= cy2; cy++)
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

		int GLAmbientShadowRenderer::GetNumDirtyChunks() {
			int cnt = 0;
			for (size_t i = 0; i < chunks.size(); i++) {
				Chunk &c = chunks[i];
				if (c.dirty)
					cnt++;
			}
			return cnt;
		}

		void GLAmbientShadowRenderer::Update() {
			if (GetNumDirtyChunks() > 0 && (dispatch == NULL || dispatch->done)) {
				if (dispatch) {
					dispatch->Join();
					delete dispatch;
				}
				dispatch = new UpdateDispatch(this);
				dispatch->Start();
			}

			// Count the number of chunks that need to be uploaded to GPU.
			// This value is approximate but it should be okay for profiling use
			int cnt = 0;
			for (size_t i = 0; i < chunks.size(); i++) {
				if (!chunks[i].transferDone.load())
					cnt++;
			}
			GLProfiler::Context profiler(renderer->GetGLProfiler(), "Large Ambient Occlusion [>= %d chunk(s)]", cnt);

			device->BindTexture(IGLDevice::Texture3D, texture);
			for (size_t i = 0; i < chunks.size(); i++) {
				Chunk &c = chunks[i];
				if (!c.transferDone.exchange(true)) {
					device->TexSubImage3D(IGLDevice::Texture3D, 0, c.cx * ChunkSize,
					                      c.cy * ChunkSize, c.cz * ChunkSize + 1, ChunkSize,
					                      ChunkSize, ChunkSize, IGLDevice::Red,
					                      IGLDevice::FloatType, c.data);
				}
			}
		}

		void GLAmbientShadowRenderer::UpdateDirtyChunks() {
			int dirtyChunkIds[256];
			int numDirtyChunks = 0;
			int nearDirtyChunks = 0;

			// first, check only chunks in near range
			Vector3 eyePos = renderer->GetSceneDef().viewOrigin;
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
					if (numDirtyChunks >= 256)
						break;
				}
			}

			// far chunks
			if (numDirtyChunks == 0) {
				for (size_t i = 0; i < chunks.size(); i++) {
					Chunk &c = chunks[i];
					if (c.dirty) {
						dirtyChunkIds[numDirtyChunks++] = static_cast<int>(i);
						if (numDirtyChunks >= 256)
							break;
					}
				}
			}

			// limit update count per frame
			for (int i = 0; i < 8; i++) {
				if (numDirtyChunks <= 0)
					break;
                int idx = SampleRandomInt(0, numDirtyChunks - 1);
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

			for (int z = c.dirtyMinZ; z <= c.dirtyMaxZ; z++)
				for (int y = c.dirtyMinY; y <= c.dirtyMaxY; y++)
					for (int x = c.dirtyMinX; x <= c.dirtyMaxX; x++) {
						IntVector3 pos;
						pos.x = (x + originX);
						pos.y = (y + originY);
						pos.z = (z + originZ);

						c.data[z][y][x] = Evaluate(pos);
					}

			c.dirty = false;
			c.transferDone = false;
		}
	}
}
