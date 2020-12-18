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

#include "GLModel.h"
#include "IGLDevice.h"
#include <Core/VoxelModel.h>

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLProgram;
		class GLImage;
		class GLOptimizedVoxelModel : public GLModel {
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

			GLProgram *optimizedVoxelModelOutlinesProgram;
			GLProgram *optimizedVoxelModelOccludedProgram;
			GLProgram *optimizedVoxelModelOcclusionTestProgram;

			IGLDevice::UInteger buffer;
			IGLDevice::UInteger idxBuffer;
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			std::vector<uint16_t> bmpIndex; // bmp id for vertex (not index)
			std::vector<Bitmap *> bmps;
			unsigned int numIndices;

			Vector3 origin;
			float radius;

			AABB3 boundingBox;

			uint8_t calcAOID(VoxelModel *, int x, int y, int z, int ux, int uy, int uz, int vx,
			                 int vy, int vz);
			void EmitFace(VoxelModel *, int x, int y, int z, int nx, int ny, int nz,
			              uint32_t color);
			// v major
			void EmitSlice(uint8_t *slice, int usize, int vsize, int sx, int sy, int sz, int ux,
			               int uy, int uz, int vx, int vy, int vz, int mx, int my, int mz,
			               bool flip, VoxelModel *);
			void BuildVertices(VoxelModel *);
			void GenerateTexture();

		protected:
			~GLOptimizedVoxelModel();

		public:
			GLOptimizedVoxelModel(VoxelModel *, GLRenderer *r);

			static void PreloadShaders(GLRenderer *);

			void Prerender(std::vector<client::ModelRenderParam> params, bool ghostPass) override;

			void RenderShadowMapPass(std::vector<client::ModelRenderParam> params) override;

			void RenderSunlightPass(std::vector<client::ModelRenderParam> params,
			                        bool ghostPass, bool farRender) override;

			void RenderDynamicLightPass(std::vector<client::ModelRenderParam> params,
			                            std::vector<GLDynamicLight> lights,
			                            bool farRender) override;

			AABB3 GetBoundingBox() override { return boundingBox; }

			virtual void RenderOutlinesPass(std::vector<client::ModelRenderParam> params,
			                                Vector3 outlineColor, bool fog, bool farRender);
			virtual void RenderOccludedPass(std::vector<client::ModelRenderParam> params,
			                                bool farRender);
			virtual void RenderOcclusionTestPass(std::vector<client::ModelRenderParam> params,
			                                     bool farRender);
		};
	}
}
