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

#include <Client/IGameMapListener.h>
#include <Client/IRenderer.h>
#include <Core/Math.h>
#include "GLDynamicLight.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLMapChunk;
		class GLProgram;
		class GLImage;
		class GLMapRenderer {

			friend class GLMapChunk;

		protected:
			GLRenderer *renderer;
			IGLDevice *device;

			GLProgram *depthonlyProgram;
			GLProgram *basicProgram;
			GLProgram *dlightProgram;
			GLProgram *backfaceProgram;
			GLImage *aoImage;

			IGLDevice::UInteger squareVertexBuffer;

			struct ChunkRenderInfo {
				bool rendered;
				float distance;
			};
			GLMapChunk **chunks;
			ChunkRenderInfo *chunkInfos;

			client::GameMap *gameMap;

			int numChunkWidth, numChunkHeight;
			int numChunkDepth, numChunks;

			inline int GetChunkIndex(int x, int y, int z) {
				return (x * numChunkHeight + y) * numChunkDepth + z;
			}

			inline GLMapChunk *GetChunk(int x, int y, int z) {
				return chunks[GetChunkIndex(x, y, z)];
			}

			void RealizeChunks(Vector3 eye);

			void DrawColumnDepth(int cx, int cy, int cz, Vector3 eye);
			void DrawColumnSunlight(int cx, int cy, int cz, Vector3 eye);
			void DrawColumnDLight(int cx, int cy, int cz, Vector3 eye,
			                      const std::vector<GLDynamicLight> &lights);

			void RenderBackface();

		public:
			GLMapRenderer(client::GameMap *, GLRenderer *);
			virtual ~GLMapRenderer();

			static void PreloadShaders(GLRenderer *);

			void GameMapChanged(int x, int y, int z, client::GameMap *);

			client::GameMap *GetMap() { return gameMap; }

			void Realize();
			void Prerender();
			void RenderSunlightPass();
			void RenderDynamicLightPass(std::vector<GLDynamicLight> lights);
		};
	}
}
