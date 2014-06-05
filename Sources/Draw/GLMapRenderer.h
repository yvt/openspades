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

#include "../Client/IGameMapListener.h"
#include "../Core/Math.h"
#include "IGLDevice.h"
#include "../Client/IRenderer.h"
#include "GLDynamicLight.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLMapChunk;
		class GLProgram;
		class GLImage;
		class GLMapFastChunk;
		class GLMapRenderer{
			
			friend class GLMapChunk;
			friend class GLMapFastChunk;
			
		protected:
			GLRenderer *renderer;
			IGLDevice *device;
			
			GLProgram *basicProgram;
			GLProgram *dlightProgram;
			GLProgram *fastBasicProgram;
			GLProgram *fastDlightProgram;
			GLProgram *backfaceProgram;
			GLImage *aoImage;
			GLImage *detailImage;
			
			IGLDevice::UInteger squareVertexBuffer;
			
			struct ChunkRenderInfo {
				bool rendered;
				float distance;
			};
			std::vector<GLMapChunk *> chunks;
			std::vector<GLMapFastChunk *> fastChunks;
			std::vector<ChunkRenderInfo> chunkInfos;
			
			client::GameMap *gameMap;
			
			int numChunkWidth, numChunkHeight;
			int numChunkDepth, numChunks;
			int numFastChunkWidth;
			int numFastChunkHeight;
			int numFastChunks;
			
			inline int GetChunkIndex(int x, int y, int z){
				return (x * numChunkHeight + y) * numChunkDepth + z;
			}
			inline int GetFastChunkIndex(int x, int y){
				return x * numFastChunkHeight + y;
			}
			
			inline GLMapChunk& GetChunk(int x, int y, int z) {
				return *chunks[GetChunkIndex(x, y, z)];
			}
			
			inline GLMapFastChunk& GetFastChunk(int x, int y)
			{
				return *fastChunks[GetFastChunkIndex(x, y)];
			}
			
			void RealizeChunks(Vector3 eye);
			void RealizeFastChunks(Vector3 eye);
			
			int GetVisibleDistance();
			int GetFastRenderDistance();
			
			void DrawColumnSunlight(int cx, int cy, int cz, Vector3 eye, bool fast);
			void DrawColumnDLight(int cx, int cy, int cz, Vector3 eye, const std::vector<GLDynamicLight>& lights, bool fast);
			
			void RenderBackface();
			
		public:
			GLMapRenderer(client::GameMap *, GLRenderer *);
			virtual ~GLMapRenderer();
			
			static void PreloadShaders(GLRenderer *);
			
			void GameMapChanged(int x, int y, int z, client::GameMap *);
			
			client::GameMap *GetMap() { return gameMap; }
			
			void Prerender();
			void RenderSunlightPass();
			void RenderDynamicLightPass(std::vector<GLDynamicLight> lights);
		};
	}
}
