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
		class GLVoxelModel : public GLModel {
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

			GLProgram *voxelModelOutlinesProgram;
			GLProgram *voxelModelOccludedProgram;
			GLProgram *voxelModelOcclusionTestProgram;

			IGLDevice::UInteger buffer;
			IGLDevice::UInteger idxBuffer;
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			unsigned int numIndices;

			Vector3 origin;
			float radius;

			AABB3 boundingBox;

			uint8_t calcAOID(VoxelModel *, int x, int y, int z, int ux, int uy, int uz, int vx,
			                 int vy, int vz);
			void EmitFace(VoxelModel *, int x, int y, int z, int nx, int ny, int nz,
			              uint32_t color);
			void BuildVertices(VoxelModel *);

		protected:
			~GLVoxelModel();

		public:
			GLVoxelModel(VoxelModel *, GLRenderer *r);

			static void PreloadShaders(GLRenderer *);

			void Prerender(std::vector<client::ModelRenderParam> params, bool ghostPass) override;

			void RenderShadowMapPass(std::vector<client::ModelRenderParam> params) override;

			void RenderSunlightPass(std::vector<client::ModelRenderParam> params,
			                        bool ghostPass,
			                        bool farRender) override;

			void RenderDynamicLightPass(std::vector<client::ModelRenderParam> params,
			                            std::vector<GLDynamicLight> lights,
			                            bool farRender) override;

			virtual void RenderOutlinesPass(std::vector<client::ModelRenderParam> params,
			                                Vector3 outlineColor, bool fog, bool farRender);
			virtual void RenderOccludedPass(std::vector<client::ModelRenderParam> params,
			                                bool farRender);
			virtual void RenderOcclusionTestPass(std::vector<client::ModelRenderParam> params,
			                                     bool farRender);

			AABB3 GetBoundingBox() override { return boundingBox; }
		};
	}
}
