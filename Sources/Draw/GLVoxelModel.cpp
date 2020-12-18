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

#include "GLVoxelModel.h"
#include "GLDynamicLightShader.h"
#include "GLImage.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLRenderer.h"
#include "GLShadowMapShader.h"
#include "GLShadowShader.h"
#include "IGLShadowMapRenderer.h"
#include <Core/Debug.h>

namespace spades {
	namespace draw {
		void GLVoxelModel::PreloadShaders(spades::draw::GLRenderer *renderer) {
			renderer->RegisterProgram("Shaders/VoxelModel.program");
			renderer->RegisterProgram("Shaders/VoxelModelDynamicLit.program");
			renderer->RegisterProgram("Shaders/VoxelModelShadowMap.program");
			renderer->RegisterImage("Gfx/AmbientOcclusion.png");
		}
		GLVoxelModel::GLVoxelModel(VoxelModel *m, GLRenderer *r) {
			SPADES_MARK_FUNCTION();

			renderer = r;
			device = r->GetGLDevice();

			program = renderer->RegisterProgram("Shaders/VoxelModel.program");
			dlightProgram = renderer->RegisterProgram("Shaders/VoxelModelDynamicLit.program");
			shadowMapProgram = renderer->RegisterProgram("Shaders/VoxelModelShadowMap.program");
			aoImage = (GLImage *)renderer->RegisterImage("Gfx/AmbientOcclusion.png");

			voxelModelOutlinesProgram =
			  renderer->RegisterProgram("Shaders/VoxelModelOutlines.program");
			voxelModelOccludedProgram =
			  renderer->RegisterProgram("Shaders/VoxelModelOccluded.program");
			voxelModelOcclusionTestProgram =
			  renderer->RegisterProgram("Shaders/VoxelModelOcclusionTest.program");

			BuildVertices(m);

			buffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->BufferData(IGLDevice::ArrayBuffer,
			                   static_cast<IGLDevice::Sizei>(vertices.size() * sizeof(Vertex)),
			                   vertices.data(), IGLDevice::StaticDraw);

			idxBuffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, idxBuffer);
			device->BufferData(IGLDevice::ArrayBuffer,
			                   static_cast<IGLDevice::Sizei>(indices.size() * sizeof(uint32_t)),
			                   indices.data(), IGLDevice::StaticDraw);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);

			origin = m->GetOrigin();
			origin -= .5f; // (0,0,0) is center of voxel (0,0,0)

			Vector3 minPos = {0, 0, 0};
			Vector3 maxPos = {(float)m->GetWidth(), (float)m->GetHeight(), (float)m->GetDepth()};
			minPos += origin;
			maxPos += origin;
			Vector3 maxDiff = {std::max(fabsf(minPos.x), fabsf(maxPos.x)),
			                   std::max(fabsf(minPos.y), fabsf(maxPos.y)),
			                   std::max(fabsf(minPos.z), fabsf(maxPos.z))};
			radius = maxDiff.GetLength();

			boundingBox.min = minPos;
			boundingBox.max = maxPos;

			// clean up
			numIndices = (unsigned int)indices.size();
			std::vector<Vertex>().swap(vertices);
			std::vector<uint32_t>().swap(indices);
		}
		GLVoxelModel::~GLVoxelModel() {
			SPADES_MARK_FUNCTION();

			device->DeleteBuffer(idxBuffer);
			device->DeleteBuffer(buffer);
		}

		uint8_t GLVoxelModel::calcAOID(VoxelModel *m, int x, int y, int z, int ux, int uy, int uz,
		                               int vx, int vy, int vz) {
			int v = 0;
			if (m->IsSolid(x - ux, y - uy, z - uz))
				v |= 1;
			if (m->IsSolid(x + ux, y + uy, z + uz))
				v |= 1 << 1;
			if (m->IsSolid(x - vx, y - vy, z - vz))
				v |= 1 << 2;
			if (m->IsSolid(x + vx, y + vy, z + vz))
				v |= 1 << 3;
			if (m->IsSolid(x - ux + vx, y - uy + vy, z - uz + vz))
				v |= 1 << 4;
			if (m->IsSolid(x - ux - vx, y - uy - vy, z - uz - vz))
				v |= 1 << 5;
			if (m->IsSolid(x + ux + vx, y + uy + vy, z + uz + vz))
				v |= 1 << 6;
			if (m->IsSolid(x + ux - vx, y + uy - vy, z + uz - vz))
				v |= 1 << 7;
			return (uint8_t)v;
		}

		void GLVoxelModel::EmitFace(spades::VoxelModel *model, int x, int y, int z, int nx, int ny,
		                            int nz, uint32_t color) {
			SPADES_MARK_FUNCTION_DEBUG();
			// decide face tangent
			int ux = ny, uy = nz, uz = nx;
			int vx = nz, vy = nx, vz = ny;
			if (nx < 0 || ny < 0 || nz < 0) {
				vx = -vx;
				vy = -vy;
				vz = -vz;
			}
			// now cross(u, v) = n (somehow)
			SPAssert(ux * vx + uy * vy + uz * vz == 0);
			SPAssert(ux * nx + uy * ny + uz * nz == 0);
			SPAssert(nx * vx + ny * vy + nz * vz == 0);
			SPAssert(uy * vz - uz * vy == -nx);
			SPAssert(uz * vx - ux * vz == -ny);
			SPAssert(ux * vy - uy * vx == -nz);

			// decide face origin
			int fx = 0, fy = 0, fz = 0;
			if (ux == -1 || vx == -1) {
				fx = 1;
				SPAssert(ux + vx == -1);
			}
			if (uy == -1 || vy == -1) {
				fy = 1;
				SPAssert(uy + vy == -1);
			}
			if (uz == -1 || vz == -1) {
				fz = 1;
				SPAssert(uz + vz == -1);
			}
			SPAssert(nx * fx + ny * fy + nz * fz == 0);

			uint8_t aoID = calcAOID(model, x + nx, y + ny, z + nz, ux, uy, uz, vx, vy, vz);

			Vertex v;
			uint32_t idx = (uint32_t)vertices.size();

			v.aoID = aoID;
			v.red = (uint8_t)(color);
			v.green = (uint8_t)(color >> 8);
			v.blue = (uint8_t)(color >> 16);
			v.diffuse = 255;
			v.nx = nx;
			v.ny = ny;
			v.nz = nz;

			x += fx;
			y += fy;
			z += fz;

			if (nx > 0)
				x++;
			if (ny > 0)
				y++;
			if (nz > 0)
				z++;

			SPAssert(x >= 0);
			SPAssert(y >= 0);
			SPAssert(y >= 0);
			SPAssert(x + ux >= 0);
			SPAssert(y + uy >= 0);
			SPAssert(z + uz >= 0);
			SPAssert(x + vx >= 0);
			SPAssert(y + vy >= 0);
			SPAssert(z + vz >= 0);
			SPAssert(x + ux + vx >= 0);
			SPAssert(y + uy + vy >= 0);
			SPAssert(z + uz + vz >= 0);

			v.u = 0;
			v.v = 0;
			v.x = x;
			v.y = y;
			v.z = z;
			vertices.push_back(v);

			v.u = 1;
			v.v = 0;
			v.x = x + ux;
			v.y = y + uy;
			v.z = z + uz;
			vertices.push_back(v);

			v.u = 0;
			v.v = 1;
			v.x = x + vx;
			v.y = y + vy;
			v.z = z + vz;
			vertices.push_back(v);

			v.u = 1;
			v.v = 1;
			v.x = x + ux + vx;
			v.y = y + uy + vy;
			v.z = z + uz + vz;
			vertices.push_back(v);

			indices.push_back(idx);
			indices.push_back(idx + 1);
			indices.push_back(idx + 2);
			indices.push_back(idx + 1);
			indices.push_back(idx + 3);
			indices.push_back(idx + 2);
		}

		void GLVoxelModel::BuildVertices(spades::VoxelModel *model) {
			SPADES_MARK_FUNCTION();

			SPAssert(vertices.empty());
			SPAssert(indices.empty());

			int w = model->GetWidth();
			int h = model->GetHeight();
			int d = model->GetDepth();
			for (int x = 0; x < w; x++) {
				for (int y = 0; y < h; y++) {
					for (int z = 0; z < d; z++) {
						if (!model->IsSolid(x, y, z))
							continue;

						uint32_t color = model->GetColor(x, y, z);
						color |= 0xff000000UL;

						if (!model->IsSolid(x - 1, y, z))
							EmitFace(model, x, y, z, -1, 0, 0, color);
						if (!model->IsSolid(x + 1, y, z))
							EmitFace(model, x, y, z, 1, 0, 0, color);
						if (!model->IsSolid(x, y - 1, z))
							EmitFace(model, x, y, z, 0, -1, 0, color);
						if (!model->IsSolid(x, y + 1, z))
							EmitFace(model, x, y, z, 0, 1, 0, color);
						if (!model->IsSolid(x, y, z - 1))
							EmitFace(model, x, y, z, 0, 0, -1, color);
						if (!model->IsSolid(x, y, z + 1))
							EmitFace(model, x, y, z, 0, 0, 1, color);
					}
				}
			}
		}

		void GLVoxelModel::Prerender(std::vector<client::ModelRenderParam> params, bool ghostPass) {
			SPADES_MARK_FUNCTION();

			RenderSunlightPass(params, ghostPass, false);
		}

		void GLVoxelModel::RenderShadowMapPass(std::vector<client::ModelRenderParam> params) {
			SPADES_MARK_FUNCTION();

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			shadowMapProgram->Use();

			static GLShadowMapShader shadowMapShader;
			shadowMapShader(renderer, shadowMapProgram, 0);

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(shadowMapProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");

			positionAttribute(shadowMapProgram);
			normalAttribute(shadowMapProgram);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)0);
			if (normalAttribute() != -1) {
				device->VertexAttribPointer(normalAttribute(), 3, IGLDevice::Byte, false,
				                            sizeof(Vertex), (void *)12);
			}
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), true);
			if (normalAttribute() != -1)
				device->EnableVertexAttribArray(normalAttribute(), true);

			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (size_t i = 0; i < params.size(); i++) {
				const client::ModelRenderParam &param = params[i];

				if (!param.castShadow || param.ghost) {
					continue;
				}

				// frustrum cull
				float rad = radius;
				rad *= param.matrix.GetAxis(0).GetLength();

				if (param.depthHack)
					continue;

				if (!renderer->GetShadowMapRenderer()->SphereCull(param.matrix.GetOrigin(), rad)) {
					continue;
				}

				Matrix4 modelMatrix = param.matrix;

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(shadowMapProgram);
				modelMatrixU.SetValue(modelMatrix);

				modelMatrix.m[12] = 0.f;
				modelMatrix.m[13] = 0.f;
				modelMatrix.m[14] = 0.f;
				static GLProgramUniform modelNormalMatrix("modelNormalMatrix");
				modelNormalMatrix(shadowMapProgram);
				modelNormalMatrix.SetValue(modelMatrix);

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     (void *)0);
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), false);
			if (normalAttribute() != -1)
				device->EnableVertexAttribArray(normalAttribute(), false);

			device->ActiveTexture(0);
			device->BindTexture(IGLDevice::Texture2D, 0);
		}

		void GLVoxelModel::RenderSunlightPass(std::vector<client::ModelRenderParam> params, bool ghostPass,
		                                      bool farRender) {
			SPADES_MARK_FUNCTION();

			device->ActiveTexture(0);
			aoImage->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			program->Use();

			static GLShadowShader shadowShader;
			shadowShader(renderer, program, 1);

			static GLProgramUniform fogDistance("fogDistance");
			fogDistance(program);
			fogDistance.SetValue(renderer->GetFogDistance());

			static GLProgramUniform fogColor("fogColor");
			fogColor(program);
			Vector3 fogCol = renderer->GetFogColorForSolidPass();
			fogCol *= fogCol;
			fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);

			static GLProgramUniform aoUniform("ambientOcclusionTexture");
			aoUniform(program);
			aoUniform.SetValue(0);

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(program);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			static GLProgramUniform sunLightDirection("sunLightDirection");
			sunLightDirection(program);
			Vector3 sunPos = MakeVector3(0, -1, -1);
			sunPos = sunPos.Normalize();
			sunLightDirection.SetValue(sunPos.x, sunPos.y, sunPos.z);

			static GLProgramUniform viewOriginVector("viewOriginVector");
			viewOriginVector(program);
			const auto &viewOrigin = renderer->GetSceneDef().viewOrigin;
			viewOriginVector.SetValue(viewOrigin.x, viewOrigin.y, viewOrigin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute textureCoordAttribute("textureCoordAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");

			positionAttribute(program);
			textureCoordAttribute(program);
			colorAttribute(program);
			normalAttribute(program);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)0);
			device->VertexAttribPointer(textureCoordAttribute(), 2, IGLDevice::UnsignedShort, false,
			                            sizeof(Vertex), (void *)4);
			device->VertexAttribPointer(colorAttribute(), 4, IGLDevice::UnsignedByte, true,
			                            sizeof(Vertex), (void *)8);
			device->VertexAttribPointer(normalAttribute(), 3, IGLDevice::Byte, false,
			                            sizeof(Vertex), (void *)12);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(textureCoordAttribute(), true);
			device->EnableVertexAttribArray(colorAttribute(), true);
			device->EnableVertexAttribArray(normalAttribute(), true);

			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (size_t i = 0; i < params.size(); i++) {
				const client::ModelRenderParam &param = params[i];

				if (param.ghost != ghostPass) {
					continue;
				}

				// frustrum cull
				float rad = radius;
				if (!farRender) {
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				static GLProgramUniform customColor("customColor");
				customColor(program);
				customColor.SetValue(param.customColor.x, param.customColor.y, param.customColor.z);

				Matrix4 modelMatrix = param.matrix;
				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(program);
				projectionViewModelMatrix.SetValue((farRender
				                                      ? renderer->farProjectionViewMatrix
				                                      : renderer->GetProjectionViewMatrix()) *
				                                   modelMatrix);

				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				viewModelMatrix(program);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * modelMatrix);

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(program);
				modelMatrixU.SetValue(modelMatrix);

				modelMatrix.m[12] = 0.f;
				modelMatrix.m[13] = 0.f;
				modelMatrix.m[14] = 0.f;
				static GLProgramUniform modelNormalMatrix("modelNormalMatrix");
				modelNormalMatrix(program);
				modelNormalMatrix.SetValue(modelMatrix);

				static GLProgramUniform modelOpacity("modelOpacity");
				modelOpacity(program);
				modelOpacity.SetValue(param.opacity);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     (void *)0);
				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(textureCoordAttribute(), false);
			device->EnableVertexAttribArray(colorAttribute(), false);
			device->EnableVertexAttribArray(normalAttribute(), false);

			device->ActiveTexture(0);
			device->BindTexture(IGLDevice::Texture2D, 0);
		}

		void GLVoxelModel::RenderDynamicLightPass(std::vector<client::ModelRenderParam> params,
		                                          std::vector<GLDynamicLight> lights,
		                                          bool farRender) {
			SPADES_MARK_FUNCTION();

			device->ActiveTexture(0);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			dlightProgram->Use();

			static GLDynamicLightShader dlightShader;

			static GLProgramUniform fogDistance("fogDistance");
			fogDistance(dlightProgram);
			fogDistance.SetValue(renderer->GetFogDistance());

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(dlightProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			static GLProgramUniform sunLightDirection("sunLightDirection");
			sunLightDirection(dlightProgram);
			Vector3 sunPos = MakeVector3(0, -1, -1);
			sunPos = sunPos.Normalize();
			sunLightDirection.SetValue(sunPos.x, sunPos.y, sunPos.z);

			static GLProgramUniform viewOriginVector("viewOriginVector");
			viewOriginVector(dlightProgram);
			const auto &viewOrigin = renderer->GetSceneDef().viewOrigin;
			viewOriginVector.SetValue(viewOrigin.x, viewOrigin.y, viewOrigin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");

			positionAttribute(dlightProgram);
			colorAttribute(dlightProgram);
			normalAttribute(dlightProgram);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)0);
			device->VertexAttribPointer(colorAttribute(), 4, IGLDevice::UnsignedByte, true,
			                            sizeof(Vertex), (void *)8);
			device->VertexAttribPointer(normalAttribute(), 3, IGLDevice::Byte, false,
			                            sizeof(Vertex), (void *)12);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(colorAttribute(), true);
			device->EnableVertexAttribArray(normalAttribute(), true);

			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (size_t i = 0; i < params.size(); i++) {
				const client::ModelRenderParam &param = params[i];

				if (param.ghost)
					continue;

				// frustrum cull
				float rad = radius;
				if (!farRender) {
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				static GLProgramUniform customColor("customColor");
				customColor(dlightProgram);
				customColor.SetValue(param.customColor.x, param.customColor.y, param.customColor.z);

				Matrix4 modelMatrix = param.matrix;
				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(dlightProgram);
				projectionViewModelMatrix.SetValue((farRender
				                                      ? renderer->farProjectionViewMatrix
				                                      : renderer->GetProjectionViewMatrix()) *
				                                   modelMatrix);

				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				viewModelMatrix(dlightProgram);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * modelMatrix);

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(dlightProgram);
				modelMatrixU.SetValue(modelMatrix);

				modelMatrix.m[12] = 0.f;
				modelMatrix.m[13] = 0.f;
				modelMatrix.m[14] = 0.f;
				static GLProgramUniform modelNormalMatrix("modelNormalMatrix");
				modelNormalMatrix(dlightProgram);
				modelNormalMatrix.SetValue(modelMatrix);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}
				for (size_t i = 0; i < lights.size(); i++) {
					if (!lights[i].SphereCull(param.matrix.GetOrigin(), rad))
						continue;

					dlightShader(renderer, dlightProgram, lights[i], 0);

					device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
					                     (void *)0);
				}
				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);

			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(colorAttribute(), false);
			device->EnableVertexAttribArray(normalAttribute(), false);

			device->ActiveTexture(0);
		}

		void GLVoxelModel::RenderOutlinesPass(std::vector<client::ModelRenderParam> params,
		                                      Vector3 outlineColor, bool fog, bool farRender) {
			SPADES_MARK_FUNCTION();

			device->ActiveTexture(0);

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			voxelModelOutlinesProgram->Use();

			static GLProgramUniform fogColor("fogColor");
			fogColor(voxelModelOutlinesProgram);
			auto fogCol = renderer->GetFogColorForSolidPass();
			if (!fog) {
				fogCol = outlineColor;
			}
			fogCol *= fogCol;
			fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);

			static GLProgramUniform outlineColorUniform("outlineColor");
			outlineColorUniform(voxelModelOutlinesProgram);
			outlineColor *= outlineColor;
			outlineColorUniform.SetValue(outlineColor.x, outlineColor.y, outlineColor.z);

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(voxelModelOutlinesProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			static GLProgramAttribute positionAttribute("positionAttribute");
			positionAttribute(voxelModelOutlinesProgram);

			static GLProgramUniform viewOriginVector("viewOriginVector");
			viewOriginVector(voxelModelOutlinesProgram);
			const auto &viewOrigin = renderer->GetSceneDef().viewOrigin;
			viewOriginVector.SetValue(viewOrigin.x, viewOrigin.y, viewOrigin.z);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)0);

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (auto &param : params) {
				// frustrum cull
				if (!farRender) {
					auto rad = radius;
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				auto modelMatrix = param.matrix;

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(voxelModelOutlinesProgram);
				modelMatrixU.SetValue(modelMatrix);

				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(voxelModelOutlinesProgram);
				const auto &pvMat = farRender
				                    ? renderer->farProjectionViewMatrix
					                : renderer->GetProjectionViewMatrix();
				projectionViewModelMatrix.SetValue(pvMat * modelMatrix);

				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				viewModelMatrix(voxelModelOutlinesProgram);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * modelMatrix);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     nullptr);

				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), false);
		}

		void GLVoxelModel::RenderOccludedPass(std::vector<client::ModelRenderParam> params,
		                                      bool farRender) {
			SPADES_MARK_FUNCTION();

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			voxelModelOccludedProgram->Use();

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(voxelModelOccludedProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			positionAttribute(voxelModelOccludedProgram);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)0);

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (auto &param : params) {
				// frustrum cull
				if (!farRender) {
					auto rad = radius;
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				auto modelMatrix = param.matrix;
				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(voxelModelOccludedProgram);
				const auto &pvMat = farRender
				                    ? renderer->farProjectionViewMatrix
					                : renderer->GetProjectionViewMatrix();
				projectionViewModelMatrix.SetValue(pvMat * modelMatrix);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     nullptr);
				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), false);
		}

		void GLVoxelModel::RenderOcclusionTestPass(std::vector<client::ModelRenderParam> params,
		                                           bool farRender) {
			SPADES_MARK_FUNCTION();

			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);

			voxelModelOcclusionTestProgram->Use();

			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(voxelModelOcclusionTestProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);

			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			positionAttribute(voxelModelOcclusionTestProgram);

			static GLProgramUniform viewOriginVector("viewOriginVector");
			viewOriginVector(voxelModelOcclusionTestProgram);
			const auto &viewOrigin = renderer->GetSceneDef().viewOrigin;
			viewOriginVector.SetValue(viewOrigin.x, viewOrigin.y, viewOrigin.z);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 4, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), nullptr);

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);

			for (auto &param : params) {
				// frustrum cull
				if (!farRender) {
					auto rad = radius;
					rad *= param.matrix.GetAxis(0).GetLength();
					if (!renderer->SphereFrustrumCull(param.matrix.GetOrigin(), rad)) {
						continue;
					}
				}

				auto modelMatrix = param.matrix;

				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(voxelModelOcclusionTestProgram);
				modelMatrixU.SetValue(modelMatrix);

				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(voxelModelOcclusionTestProgram);
				const auto &pvMat = farRender
					                ? renderer->farProjectionViewMatrix
					                : renderer->GetProjectionViewMatrix();
				projectionViewModelMatrix.SetValue(pvMat * modelMatrix);

				if (param.depthHack) {
					device->DepthRange(0.f, 0.1f);
				}

				device->DrawElements(IGLDevice::Triangles, numIndices, IGLDevice::UnsignedInt,
				                     nullptr);
				if (param.depthHack) {
					device->DepthRange(0.f, 1.f);
				}
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
			device->EnableVertexAttribArray(positionAttribute(), false);
		}
	}
}
