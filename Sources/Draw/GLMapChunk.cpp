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

#include <algorithm>
#include <cstddef>

#include "GLDynamicLightShader.h"
#include "GLImage.h"
#include "GLMapChunk.h"
#include "GLMapRenderer.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "GLRenderer.h"
#include "IGLDevice.h"
#include <AngelScript/include/angelscript.h> // for asOFFSET. somehow `offsetof` fails on gcc-4.8
#include <Client/GameMap.h>
#include <Core/Debug.h>
#include <Core/Settings.h>

SPADES_SETTING(cg_textures);
SPADES_SETTING(cg_multiTextures);

namespace spades {
	namespace draw {
		GLMapChunk::GLMapChunk(spades::draw::GLMapRenderer *r, client::GameMap *mp, int cx, int cy,
		                       int cz) {
			SPADES_MARK_FUNCTION();

			renderer = r;
			device = r->device;
			map = mp;
			chunkX = cx;
			chunkY = cy;
			chunkZ = cz;
			needsUpdate = true;
			realized = false;

			centerPos =
			  MakeVector3(cx * Size + Size / 2, cy * Size + Size / 2, cz * Size + Size / 2);
			radius = (float)Size * 0.5f * sqrtf(3.f);
			aabb = AABB3(cx * Size, cy * Size, cz * Size, Size, Size, Size);

			buffer = 0;
			iBuffer = 0;
		}

		GLMapChunk::~GLMapChunk() { SetRealized(false); }

		void GLMapChunk::SetRealized(bool b) {
			SPADES_MARK_FUNCTION_DEBUG();

			if (realized == b)
				return;

			if (!b) {
				if (buffer) {
					device->DeleteBuffer(buffer);
					buffer = 0;
				}
				if (iBuffer) {
					device->DeleteBuffer(iBuffer);
					iBuffer = 0;
				}
				std::vector<Vertex> i;
				i.swap(vertices);

				std::vector<uint16_t> i2;
				i2.swap(indices);
			} else {
				needsUpdate = true;
			}

			realized = b;
		}

		uint8_t GLMapChunk::calcAOID(int x, int y, int z, int ux, int uy, int uz, int vx, int vy,
		                             int vz) {
			int v = 0;
			if (IsSolid(x - ux, y - uy, z - uz))
				v |= 1;
			if (IsSolid(x + ux, y + uy, z + uz))
				v |= 1 << 1;
			if (IsSolid(x - vx, y - vy, z - vz))
				v |= 1 << 2;
			if (IsSolid(x + vx, y + vy, z + vz))
				v |= 1 << 3;
			if (IsSolid(x - ux + vx, y - uy + vy, z - uz + vz))
				v |= 1 << 4;
			if (IsSolid(x - ux - vx, y - uy - vy, z - uz - vz))
				v |= 1 << 5;
			if (IsSolid(x + ux + vx, y + uy + vy, z + uz + vz))
				v |= 1 << 6;
			if (IsSolid(x + ux - vx, y + uy - vy, z + uz - vz))
				v |= 1 << 7;
			return (uint8_t)v;
		}

		/**
		 * @param aoX Global X coordinate of the cell to evaluate ambient occlusion.
		 * @param aoY Global Y coordinate of the cell to evaluate ambient occlusion.
		 * @param aoZ Global Z coordinate of the cell to evaluate ambient occlusion.
		 * @param x Chunk local X coordinate
		 * @param y Chunk local Y coordinate
		 * @param z Chunk local Z coordinate
		 * @param tNumX Multi-texture X coordinate
		 * @param tNumY Multi-texture Y coordinate
		 */
		void GLMapChunk::EmitVertex(int x, int y, int z, int aoX, int aoY, int aoZ, int ux, int uy,
		                            int vx, int vy, uint32_t color,
		                            int tNumX, int tNumY,
		                            int nx, int ny, int nz) {
			SPADES_MARK_FUNCTION_DEBUG();

			int uz = (ux == 0 && uy == 0) ? 1 : 0;
			int vz = (vx == 0 && vy == 0) ? 1 : 0;
			Vertex inst;
			// evaluate ambient occlusion
			unsigned int aoID = calcAOID(aoX, aoY, aoZ, ux, uy, uz, vx, vy, vz);

			if (nz == 1 || ny == 1) {
				inst.shading = 0;
			} else if (nx == 1 || nx == -1) {
				inst.shading = 0; // 50;
			} else if (nz == -1) {
				inst.shading = 220;
			} else {
				inst.shading = 255;
			}

			inst.x = x;
			inst.y = y;
			inst.z = z;
			inst.colorRed = (uint8_t)(color);
			inst.colorGreen = (uint8_t)(color >> 8);
			inst.colorBlue = (uint8_t)(color >> 16);

			inst.nx = nx;
			inst.ny = ny;
			inst.nz = nz;

			// fixed position to avoid self-shadow glitch
			inst.sx = (x << 1) + ux + vx;
			inst.sy = (y << 1) + uy + vy;
			inst.sz = (z << 1) + uz + vz;

			unsigned int aoTexX = aoID & 15;
			unsigned int aoTexY = aoID >> 4;
			aoTexX *= 16;
			aoTexY *= 16;

			uint16_t idx = (uint16_t)vertices.size();
			inst.x = x;
			inst.y = y;
			inst.z = z;
			inst.aoX = aoTexX;
			inst.aoY = aoTexY;

			// Add the vertices differently depending on texture mode
			if (!renderer->previous_cg_textures) {
				// don't bother with ux/uy
				vertices.push_back(inst);

				inst.x = x + ux;
				inst.y = y + uy;
				inst.z = z + uz;
				inst.aoX = aoTexX + 15;
				inst.aoY = aoTexY;
				vertices.push_back(inst);

				inst.x = x + vx;
				inst.y = y + vy;
				inst.z = z + vz;
				inst.aoX = aoTexX;
				inst.aoY = aoTexY + 15;
				vertices.push_back(inst);

				inst.x = x + ux + vx;
				inst.y = y + uy + vy;
				inst.z = z + uz + vz;
				inst.aoX = aoTexX + 15;
				inst.aoY = aoTexY + 15;
				vertices.push_back(inst);
			} else if (renderer->previous_cg_multiTextures) {
				float ulen = 1.0f / 8.0f;

				// add the vertices

				inst.ux = (tNumX + 1) * ulen;
				inst.uy = tNumY * ulen;
				vertices.push_back(inst);

				inst.x = x + ux;
				inst.y = y + uy;
				inst.z = z + uz;
				inst.aoX = aoTexX + 15;
				inst.aoY = aoTexY;
				inst.ux = (tNumX + 1) * ulen;
				inst.uy = (tNumY + 1) * ulen;
				vertices.push_back(inst);

				inst.x = x + vx;
				inst.y = y + vy;
				inst.z = z + vz;
				inst.aoX = aoTexX;
				inst.aoY = aoTexY + 15;
				inst.ux = tNumX * ulen;
				inst.uy = tNumY * ulen;
				vertices.push_back(inst);

				inst.x = x + ux + vx;
				inst.y = y + uy + vy;
				inst.z = z + uz + vz;
				inst.aoX = aoTexX + 15;
				inst.aoY = aoTexY + 15;
				inst.ux = tNumX * ulen;
				inst.uy = (tNumY + 1) * ulen;
				vertices.push_back(inst);
			} else { // i.e. single texture mode
				inst.ux = 1;
				inst.uy = 0;
				vertices.push_back(inst);

				inst.x = x + ux;
				inst.y = y + uy;
				inst.z = z + uz;
				inst.aoX = aoTexX + 15;
				inst.aoY = aoTexY;
				inst.ux = 1;
				inst.uy = 1;
				vertices.push_back(inst);

				inst.x = x + vx;
				inst.y = y + vy;
				inst.z = z + vz;
				inst.aoX = aoTexX;
				inst.aoY = aoTexY + 15;
				inst.ux = 0;
				inst.uy = 0;
				vertices.push_back(inst);

				inst.x = x + ux + vx;
				inst.y = y + uy + vy;
				inst.z = z + uz + vz;
				inst.aoX = aoTexX + 15;
				inst.aoY = aoTexY + 15;
				inst.ux = 0;
				inst.uy = 1;
				vertices.push_back(inst);
			}

			indices.push_back(idx);
			indices.push_back(idx + 1);
			indices.push_back(idx + 2);
			indices.push_back(idx + 1);
			indices.push_back(idx + 3);
			indices.push_back(idx + 2);
		}

		bool GLMapChunk::IsSolid(int x, int y, int z) {
			if (z < 0)
				return false;
			if (z >= 64)
				return true;

			// FIXME: variable map size
			x &= 511;
			y &= 511;

			if (z == 63) {
				if (renderer->renderer->GetSettings().r_water) {
					return map->IsSolid(x, y, 62);
				} else {
					return map->IsSolid(x, y, 63);
				}
			} else {
				return map->IsSolid(x, y, z);
			}
		}

		void GLMapChunk::Update() {
			SPADES_MARK_FUNCTION();

			vertices.clear();
			indices.clear();
			if (buffer) {
				device->DeleteBuffer(buffer);
				buffer = 0;
			}
			if (iBuffer) {
				device->DeleteBuffer(iBuffer);
				iBuffer = 0;
			}

			int rchunkX = chunkX * Size;
			int rchunkY = chunkY * Size;
			int rchunkZ = chunkZ * Size;

			int x, y, z;
			for (x = 0; x < Size; x++) {
				for (y = 0; y < Size; y++) {
					for (z = 0; z < Size; z++) {
						int xx = x + rchunkX;
						int yy = y + rchunkY;
						int zz = z + rchunkZ;

						if (!IsSolid(xx, yy, zz))
							continue;

						uint32_t col = map->GetColor(xx, yy, zz);
						// col = 0xffffffff;

						// Determine which faces are being added up here instead of below
						const auto nsolid1 = !IsSolid(xx, yy, zz + 1);
						const auto nsolid2 = !IsSolid(xx, yy, zz - 1);
						const auto nsolid3 = !IsSolid(xx - 1, yy, zz);
						const auto nsolid4 = !IsSolid(xx + 1, yy, zz);
						const auto nsolid5 = !IsSolid(xx, yy - 1, zz);
						const auto nsolid6 = !IsSolid(xx, yy + 1, zz);

						// Compute the texture coords for this block (perhaps)
						int tNumX, tNumY;
						// Only do the computation if at least one face added
						if (nsolid1 || nsolid2 || nsolid3 || nsolid4 || nsolid5 || nsolid6) {
							// Only do the computation if in multi-texture mode
							if (renderer->previous_cg_multiTextures &&
							    renderer->previous_cg_textures) {
								const auto r = static_cast<int>(static_cast<uint8_t>(col));
								const auto g = static_cast<int>(static_cast<uint8_t>(col >> 8));
								const auto b = static_cast<int>(static_cast<uint8_t>(col >> 16));
								// Determine the r,g,b coords of the color
								const auto rCoord = r / 64;
								const auto gCoord = g / 64;
								const auto bCoord = b / 64;
								// Translate that into texture coords on the block
								tNumX = rCoord;
								tNumY = gCoord;
								if (bCoord == 1 || bCoord == 3) {
									tNumX += 4;
								}
								if (bCoord == 2 || bCoord == 3) {
									tNumY += 4;
								}
							}
						} else {
							continue; // no faces being added
						}

						// damaged block?
						int health = col >> 24;
						if (health < 100) {
							col &= 0xffffff;
							col &= 0xfefefe;
							col >>= 1;
						}

						if (nsolid1) {
							EmitVertex(x + 1, y, z + 1, xx, yy, zz + 1, -1, 0, 0, 1, col, tNumX, tNumY, 0, 0, 1);
						}
						if (nsolid2) {
							EmitVertex(x, y, z, xx, yy, zz - 1, 1, 0, 0, 1, col, tNumX, tNumY, 0, 0, -1);
						}
						if (nsolid3) {
							EmitVertex(x, y + 1, z, xx - 1, yy, zz, 0, 0, 0, -1, col, tNumX, tNumY, -1, 0, 0);
						}
						if (nsolid4) {
							EmitVertex(x + 1, y, z, xx + 1, yy, zz, 0, 0, 0, 1, col, tNumX, tNumY, 1, 0, 0);
						}
						if (nsolid5) {
							EmitVertex(x, y, z, xx, yy - 1, zz, 0, 0, 1, 0, col, tNumX, tNumY, 0, -1, 0);
						}
						if (nsolid6) {
							EmitVertex(x + 1, y + 1, z, xx, yy + 1, zz, 0, 0, -1, 0, col, tNumX, tNumY, 0, 1, 0);
						}
					}
				}
			}

			if (vertices.size() == 0)
				return;

			buffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);

			device->BufferData(IGLDevice::ArrayBuffer,
			                   static_cast<IGLDevice::Sizei>(vertices.size() * sizeof(Vertex)),
			                   vertices.data(), IGLDevice::DynamicDraw);

			if (!indices.empty()) {
				iBuffer = device->GenBuffer();
				device->BindBuffer(IGLDevice::ArrayBuffer, iBuffer);

				device->BufferData(IGLDevice::ArrayBuffer,
				                   static_cast<IGLDevice::Sizei>(indices.size() * sizeof(uint16_t)),
				                   indices.data(), IGLDevice::DynamicDraw);
			}
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
		}

		void GLMapChunk::RenderDepthPass() {
			SPADES_MARK_FUNCTION();
			Vector3 eye = renderer->renderer->GetSceneDef().viewOrigin;

			if (!realized)
				return;
			if (needsUpdate) {
				Update();
				needsUpdate = false;
			}
			if (!buffer) {
				// empty chunk
				return;
			}
			AABB3 bx = aabb;

			Vector3 diff = eye - centerPos;
			float sx = 0.f, sy = 0.f;
			// FIXME: variable map size?
			if (diff.x > 256.f)
				sx += 512.f;
			if (diff.y > 256.f)
				sy += 512.f;
			if (diff.x < -256.f)
				sx -= 512.f;
			if (diff.y < -256.f)
				sy -= 512.f;

			bx.min.x += sx;
			bx.min.y += sy;
			bx.max.x += sx;
			bx.max.y += sy;

			if (!renderer->renderer->BoxFrustrumCull(bx))
				return;

			GLProgram *depthonlyProgram = renderer->depthonlyProgram;

			static GLProgramUniform chunkPosition("chunkPosition");

			chunkPosition(depthonlyProgram);
			chunkPosition.SetValue((float)(chunkX * Size) + sx, (float)(chunkY * Size) + sy,
			                       (float)(chunkZ * Size));

			static GLProgramAttribute positionAttribute("positionAttribute");

			positionAttribute(depthonlyProgram);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 3, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)asOFFSET(Vertex, x));

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, iBuffer);
			device->DrawElements(IGLDevice::Triangles,
			                     static_cast<IGLDevice::Sizei>(indices.size()),
			                     IGLDevice::UnsignedShort, NULL);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
		}
		void GLMapChunk::RenderSunlightPass() {
			SPADES_MARK_FUNCTION();
			Vector3 eye = renderer->renderer->GetSceneDef().viewOrigin;

			if (!realized)
				return;
			if (needsUpdate) {
				Update();
				needsUpdate = false;
			}
			if (!buffer) {
				// empty chunk
				return;
			}
			AABB3 bx = aabb;

			Vector3 diff = eye - centerPos;
			float sx = 0.f, sy = 0.f;
			// FIXME: variable map size?
			if (diff.x > 256.f)
				sx += 512.f;
			if (diff.y > 256.f)
				sy += 512.f;
			if (diff.x < -256.f)
				sx -= 512.f;
			if (diff.y < -256.f)
				sy -= 512.f;

			bx.min.x += sx;
			bx.min.y += sy;
			bx.max.x += sx;
			bx.max.y += sy;

			if (!renderer->renderer->BoxFrustrumCull(bx))
				return;

			GLProgram *basicProgram = renderer->basicProgram;

			static GLProgramUniform chunkPosition("chunkPosition");

			chunkPosition(basicProgram);
			chunkPosition.SetValue((float)(chunkX * Size) + sx, (float)(chunkY * Size) + sy,
			                       (float)(chunkZ * Size));

			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute ambientOcclusionCoordAttribute(
			  "ambientOcclusionCoordAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");
			static GLProgramAttribute fixedPositionAttribute("fixedPositionAttribute");

			static GLProgramAttribute blockTexCoordAttribute("blockTexCoordAttribute");
			if (renderer->previous_cg_textures) {
				blockTexCoordAttribute(basicProgram);
			}

			positionAttribute(basicProgram);
			ambientOcclusionCoordAttribute(basicProgram);
			colorAttribute(basicProgram);
			normalAttribute(basicProgram);
			fixedPositionAttribute(basicProgram);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 3, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)asOFFSET(Vertex, x));
			if (ambientOcclusionCoordAttribute() != -1)
				device->VertexAttribPointer(ambientOcclusionCoordAttribute(), 2,
				                            IGLDevice::UnsignedShort, false, sizeof(Vertex),
				                            (void *)asOFFSET(Vertex, aoX));
			device->VertexAttribPointer(colorAttribute(), 4, IGLDevice::UnsignedByte, true,
			                            sizeof(Vertex), (void *)asOFFSET(Vertex, colorRed));
			if (normalAttribute() != -1)
				device->VertexAttribPointer(normalAttribute(), 3, IGLDevice::Byte, false,
				                            sizeof(Vertex), (void *)asOFFSET(Vertex, nx));

			device->VertexAttribPointer(fixedPositionAttribute(), 3, IGLDevice::Byte, false,
			                            sizeof(Vertex), (void *)asOFFSET(Vertex, sx));

			if (renderer->previous_cg_textures) {
				device->VertexAttribPointer(blockTexCoordAttribute(), 2, IGLDevice::FloatType,
				                            false, sizeof(Vertex), (void *)asOFFSET(Vertex, ux));
			}

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, iBuffer);
			device->DrawElements(IGLDevice::Triangles,
			                     static_cast<IGLDevice::Sizei>(indices.size()),
			                     IGLDevice::UnsignedShort, NULL);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
		}

		void GLMapChunk::RenderDLightPass(std::vector<GLDynamicLight> lights) {
			SPADES_MARK_FUNCTION();
			Vector3 eye = renderer->renderer->GetSceneDef().viewOrigin;

			if (!realized)
				return;
			if (needsUpdate) {
				Update();
				needsUpdate = false;
			}
			if (!buffer) {
				// empty chunk
				return;
			}
			AABB3 bx = aabb;

			Vector3 diff = eye - centerPos;
			float sx = 0.f, sy = 0.f;
			// FIXME: variable map size?
			if (diff.x > 256.f)
				sx += 512.f;
			if (diff.y > 256.f)
				sy += 512.f;
			if (diff.x < -256.f)
				sx -= 512.f;
			if (diff.y < -256.f)
				sy -= 512.f;

			bx.min.x += sx;
			bx.min.y += sy;
			bx.max.x += sx;
			bx.max.y += sy;

			if (!renderer->renderer->BoxFrustrumCull(bx))
				return;

			GLProgram *program = renderer->dlightProgram;

			static GLProgramUniform chunkPosition("chunkPosition");

			chunkPosition(program);
			chunkPosition.SetValue((float)(chunkX * Size) + sx, (float)(chunkY * Size) + sy,
			                       (float)(chunkZ * Size));

			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");

			positionAttribute(program);
			colorAttribute(program);
			normalAttribute(program);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 3, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), (void *)asOFFSET(Vertex, x));
			device->VertexAttribPointer(colorAttribute(), 4, IGLDevice::UnsignedByte, true,
			                            sizeof(Vertex), (void *)asOFFSET(Vertex, colorRed));
			device->VertexAttribPointer(normalAttribute(), 3, IGLDevice::Byte, false,
			                            sizeof(Vertex), (void *)asOFFSET(Vertex, nx));

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, iBuffer);
			for (size_t i = 0; i < lights.size(); i++) {

				static GLDynamicLightShader lightShader;
				lightShader(renderer->renderer, program, lights[i], 1);

				if (!lights[i].Cull(bx))
					continue;

				device->DrawElements(IGLDevice::Triangles,
				                     static_cast<IGLDevice::Sizei>(indices.size()),
				                     IGLDevice::UnsignedShort, NULL);
			}

			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
		}

		void GLMapChunk::RenderOutlinesPass() {
			SPADES_MARK_FUNCTION();
			const auto eye = renderer->renderer->GetSceneDef().viewOrigin;

			if (!realized)
				return;
			if (needsUpdate) {
				Update();
				needsUpdate = false;
			}
			if (!buffer) {
				// empty chunk
				return;
			}
			auto bx = aabb;

			const auto diff = eye - centerPos;
			auto sx = 0.f, sy = 0.f;
			// FIXME: variable map size?
			if (diff.x > 256.f)
				sx += 512.f;
			if (diff.y > 256.f)
				sy += 512.f;
			if (diff.x < -256.f)
				sx -= 512.f;
			if (diff.y < -256.f)
				sy -= 512.f;

			bx.min.x += sx;
			bx.min.y += sy;
			bx.max.x += sx;
			bx.max.y += sy;

			if (!renderer->renderer->BoxFrustrumCull(bx))
				return;

			auto *const outlinesProgram = renderer->basicOutlinesProgram;

			static GLProgramUniform chunkPosition("chunkPosition");

			chunkPosition(outlinesProgram);
			chunkPosition.SetValue(static_cast<float>(chunkX * Size) + sx, static_cast<float>(chunkY * Size) + sy,
			                       static_cast<float>(chunkZ * Size));

			static GLProgramAttribute positionAttribute("positionAttribute");
			positionAttribute(outlinesProgram);

			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 3, IGLDevice::UnsignedByte, false,
			                            sizeof(Vertex), reinterpret_cast<void *>(asOFFSET(Vertex, x)));

			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, iBuffer);
			device->DrawElements(IGLDevice::Triangles, indices.size(), IGLDevice::UnsignedShort,
			                     nullptr);
			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
		}

		float GLMapChunk::DistanceFromEye(const Vector3 &eye) {
			Vector3 diff = eye - centerPos;

			// FIXME: variable map size
			if (diff.x < -256.f)
				diff.x += 512.f;
			if (diff.y < -256.f)
				diff.y += 512.f;

			if (diff.x > 256.f)
				diff.x -= 512.f;
			if (diff.y > 256.f)
				diff.y -= 512.f;

			float dist = fabsf(diff.x);
			dist = std::max(dist, fabsf(diff.y));
			// note: there's no vertical fog

			// return std::max(diff.GetLength() - radius, 0.f);
			return std::max(dist - ((float)Size * .5f), 0.f);
		}
	}
}
