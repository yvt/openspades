//
//  GLOptimizedVoxelModel.h
//  OpenSpades
//
//  Created by yvt on 7/28/13.
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
		class GLOptimizedVoxelModel: public GLModel {
			class SliceGenerator;
			struct Vertex {
				uint8_t x, y, z;
				uint8_t padding;
				
				// texture coord
				uint16_t u, v;
				
				// normal
				int8_t nx, ny, nz;
				
				uint8_t padding2;
			};
			
			GLRenderer *renderer;
			IGLDevice *device;
			GLProgram *program;
			GLProgram *dlightProgram;
			GLProgram *shadowMapProgram;
			GLImage *image;
			GLImage *aoImage;
			
			IGLDevice::UInteger buffer;
			IGLDevice::UInteger idxBuffer;
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			std::vector<uint16_t> bmpIndex; // bmp id for vertex (not index)
			std::vector<Bitmap *> bmps;
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
			// v major
			void EmitSlice(uint8_t *slice,
						   int usize, int vsize,
						   int sx, int sy, int sz,
						   int ux, int uy, int uz,
						   int vx, int vy, int vz,
						   int mx, int my, int mz,
						   bool flip,
						   VoxelModel *);
			void BuildVertices(VoxelModel *);
			void GenerateTexture();
		public:
			GLOptimizedVoxelModel(VoxelModel *, GLRenderer *r);
			virtual ~GLOptimizedVoxelModel();
			
			virtual void RenderShadowMapPass(std::vector<client::ModelRenderParam> params);
			
			virtual void RenderSunlightPass(std::vector<client::ModelRenderParam> params);
			
			virtual void RenderDynamicLightPass(std::vector<client::ModelRenderParam> params, std::vector<GLDynamicLight> lights);
		};
	}
}
