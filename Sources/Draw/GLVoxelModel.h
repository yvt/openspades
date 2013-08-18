//
//  GLVoxelModel.h
//  OpenSpades
//
//  Created by yvt on 7/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

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
			
			uint8_t calcAOID(VoxelModel *,
							 int x, int y, int z,
							 int ux, int uy, int uz,
							 int vx, int vy, int vz);
			void EmitFace(VoxelModel *,
						  int x, int y, int z,
						  int nx, int ny, int nz,
						  uint32_t color);
			void BuildVertices(VoxelModel *);
		public:
			GLVoxelModel(VoxelModel *, GLRenderer *r);
			virtual ~GLVoxelModel();
			
			virtual void RenderShadowMapPass(std::vector<client::ModelRenderParam> params);
			
			virtual void RenderSunlightPass(std::vector<client::ModelRenderParam> params);
			
			virtual void RenderDynamicLightPass(std::vector<client::ModelRenderParam> params, std::vector<GLDynamicLight> lights);
		};
	}
}
