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

#include "GLDynamicLight.h"
#include "IGLDevice.h"
#include <Client/GameMap.h>
#include <Client/IRenderer.h>
#include <Core/Math.h>

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

				int8_t sx, sy, sz;
				uint8_t pad3;

				float ux, uy;
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

			uint8_t calcAOID(int x, int y, int z, int ux, int uy, int uz, int vx, int vy, int vz);

			void EmitVertex(int x, int y, int z, int aoX, int aoY, int aoZ, int ux, int uy,
			                int vx, int vy, uint32_t color,
			                int tNumX, int tNumY,
			                int nx, int ny, int nz);

			bool IsSolid(int x, int y, int z);

			void Update();

		public:
			enum { Size = 16, SizeBits = 4 };
			GLMapChunk(GLMapRenderer *, client::GameMap *mp, int cx, int cy, int cz);
			~GLMapChunk();

			void SetNeedsUpdate() { needsUpdate = true; }

			void SetRealized(bool);

			float DistanceFromEye(const Vector3 &eye);

			void RenderSunlightPass();
			void RenderDepthPass();
			void RenderDLightPass(std::vector<GLDynamicLight> lights);

			void RenderOutlinesPass();
		};
	}
}
