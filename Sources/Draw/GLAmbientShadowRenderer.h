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

#pragma once

#include <vector>
#include <atomic>

#include <Core/Debug.h>
#include <Core/Math.h>
#include "IGLDevice.h"

namespace spades {
	namespace client {
		class GameMap;
	}
	namespace draw {
		class GLRenderer;
		class IGLDevice;
		class GLAmbientShadowRenderer {

			class UpdateDispatch;
			enum { NumRays = 16, ChunkSize = 16, ChunkSizeBits = 4 };
			GLRenderer *renderer;
			IGLDevice *device;
			client::GameMap *map;
			Vector3 rays[NumRays];

			struct Chunk {
				int cx, cy, cz;
				float data[ChunkSize][ChunkSize][ChunkSize];
				bool dirty = true;
				int dirtyMinX = 0, dirtyMaxX = ChunkSize - 1;
				int dirtyMinY = 0, dirtyMaxY = ChunkSize - 1;
				int dirtyMinZ = 0, dirtyMaxZ = ChunkSize - 1;

				std::atomic<bool> transferDone {true};
			};

			IGLDevice::UInteger texture;

			int w, h, d;
			int chunkW, chunkH, chunkD;

			std::vector<Chunk> chunks;

			inline Chunk &GetChunk(int cx, int cy, int cz) {
				SPAssert(cx >= 0);
				SPAssert(cx < chunkW);
				SPAssert(cy >= 0);
				SPAssert(cy < chunkH);
				SPAssert(cz >= 0);
				SPAssert(cz < chunkD);
				return chunks[(cx + cy * chunkW) * chunkD + cz];
			}

			inline Chunk &GetChunkWrapped(int cx, int cy, int cz) {
				// FIXME: support for non-POT dimensions?
				return GetChunk(cx & (chunkW - 1), cy & (chunkH - 1), cz);
			}

			void Invalidate(int minX, int minY, int minZ, int maxX, int maxY, int maxZ);

			void UpdateChunk(int cx, int cy, int cz);
			void UpdateDirtyChunks();
			int GetNumDirtyChunks();

			UpdateDispatch *dispatch;

		public:
			GLAmbientShadowRenderer(GLRenderer *renderer, client::GameMap *map);
			~GLAmbientShadowRenderer();

			float Evaluate(IntVector3);

			void GameMapChanged(int x, int y, int z, client::GameMap *);

			void Update();

			IGLDevice::UInteger GetTexture() { return texture; }
		};
	}
}
