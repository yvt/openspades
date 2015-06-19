/*
 Copyright (c) 2013 OpenSpades Developers
 
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

#include "GLModel.h"
#include "../Core/VoxelModel.h"
#include <vector>
#include "IGLDevice.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLProgram;
		class GLImage;
		class GLVoxelModel: public GLModel {
			struct Vertex {
				uint8_t x, y, z;
				uint8_t aoID;
				
				// texture coord
				uint16_t u, v;
				
				// color
				uint8_t red, green, blue;
				uint8_t diffuse;
				
				// normal
				uint8_t nx, ny, nz;
			};
			
			GLRenderer *renderer;
			IGLDevice *device;
			GLProgram *program;
			GLProgram *dlightProgram;
			GLProgram *shadowMapProgram;
			GLImage *aoImage;
			
			IGLDevice::UInteger buffer;
			IGLDevice::UInteger idxBuffer;
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			unsigned int numIndices;
			
			Vector3 origin;
			float radius;
			
			AABB3 boundingBox;
			
			uint8_t calcAOID(VoxelModel *,
							 int x, int y, int z,
							 int ux, int uy, int uz,
							 int vx, int vy, int vz);
			void EmitFace(VoxelModel *,
						  int x, int y, int z,
						  int nx, int ny, int nz,
						  uint32_t color);
			void BuildVertices(VoxelModel *);
		protected:
			virtual ~GLVoxelModel();
		public:
			GLVoxelModel(VoxelModel *, GLRenderer *r);
			
			static void PreloadShaders(GLRenderer *);
			
			virtual void RenderShadowMapPass(std::vector<client::ModelRenderParam> params);
			
			virtual void RenderSunlightPass(std::vector<client::ModelRenderParam> params);
			
			virtual void RenderDynamicLightPass(std::vector<client::ModelRenderParam> params, std::vector<GLDynamicLight> lights);
			
			virtual AABB3 GetBoundingBox() { return boundingBox; }
		};
	}
}
