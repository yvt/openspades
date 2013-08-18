//
//  GLMapChunk.h
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Client/GameMap.h"
#include "../Core/Math.h"
#include <vector>
#include "IGLDevice.h"
#include "../Client/IRenderer.h"
#include "GLDynamicLight.h"

namespace spades {
	namespace draw {
		class GLMapRenderer;
		class IGLDevice;
		class GLMapChunk {
			struct Vertex {
				uint8_t x, y, z;
				uint8_t pad;
				
				uint16_t aoX, aoY;
				
				uint8_t colorRed;
				uint8_t colorGreen;
				uint8_t colorBlue;
				uint8_t shading;
				
				int8_t nx, ny, nz;
				uint8_t pad2;
			};
			
			GLMapRenderer *renderer;
			IGLDevice *device;
			client::GameMap *map;
			int chunkX, chunkY, chunkZ;
			AABB3 aabb;
			
			Vector3 centerPos;
			float radius;
			
			std::vector<Vertex> vertices;
			std::vector<uint16_t> indices;
			IGLDevice::UInteger buffer;
			IGLDevice::UInteger iBuffer;
			
			bool needsUpdate;
			bool realized;
			
			uint8_t calcAOID(int x, int y, int z,
							 int ux, int uy, int uz,
							 int vx, int vy, int vz);
			
			void EmitVertex(int aoX, int aoY, int aoZ,
							  int x, int y, int z,
							  int ux, int uy,
							  int vx, int vy,
							  uint32_t color,
							  int nx, int ny, int nz);
			
			bool IsSolid(int x, int y, int z);
			
			void Update();
		public:
			enum { Size = 16, SizeBits = 4 };
			GLMapChunk(GLMapRenderer *,
					   client::GameMap *mp,
					   int cx, int cy, int cz);
			~GLMapChunk();
			
			void SetNeedsUpdate() {needsUpdate = true;}
			
			void SetRealized(bool);
			
			float DistanceFromEye(const Vector3& eye);
			
			void RenderSunlightPass();
			void RenderDLightPass(std::vector<GLDynamicLight> lights);
		};
	}
}
