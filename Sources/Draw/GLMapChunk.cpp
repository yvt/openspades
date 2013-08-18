//
//  GLMapChunk.cpp
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLMapChunk.h"
#include <algorithm>
#include "GLMapRenderer.h"
#include "IGLDevice.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "../Core/Debug.h"
#include "GLRenderer.h"
#include "../Client/GameMap.h"
#include "../Core/Settings.h"
#include "GLDynamicLightShader.h"

SPADES_SETTING(r_water, "1");

namespace spades {
	namespace draw {
		GLMapChunk::GLMapChunk(spades::draw::GLMapRenderer *r, client::GameMap *mp,
							   int cx, int cy, int cz){
			SPADES_MARK_FUNCTION();
			
			renderer = r;
			device = r->device;
			map = mp;
			chunkX = cx;
			chunkY = cy;
			chunkZ = cz;
			needsUpdate = true;
			realized = false;
			
			centerPos = MakeVector3(cx * Size + Size / 2,
								cy * Size + Size / 2,
								cz * Size + Size / 2);
			radius = (float)Size * 0.5f * sqrtf(3.f);
			aabb = AABB3(cx * Size, cy * Size, cz * Size,
							 Size, Size, Size);
			
			buffer = 0;
			iBuffer = 0;
			
		}
		
		GLMapChunk::~GLMapChunk() {
			SetRealized(false);
		}
		
		void GLMapChunk::SetRealized(bool b){
			SPADES_MARK_FUNCTION_DEBUG();
			
			if(realized == b)
				return;
			
			if(!b){
				if(buffer){
					device->DeleteBuffer(buffer);
					buffer = 0;
				}
				if(iBuffer){
					device->DeleteBuffer(iBuffer);
					iBuffer = 0;
				}
				std::vector<Vertex> i;
				i.swap(vertices);
				
				std::vector<uint16_t> i2;
				i2.swap(indices);
			}else{
				needsUpdate = true;
			}
			
			realized = b;
		}
		
		uint8_t GLMapChunk::calcAOID(int x, int y, int z,
						 int ux, int uy, int uz,
						 int vx, int vy, int vz){
			int v = 0;
			if(IsSolid(x - ux, y - uy, z - uz))
				v |= 1;
			if(IsSolid(x + ux, y + uy, z + uz))
				v |= 1 << 1;
			if(IsSolid(x - vx, y - vy, z - vz))
				v |= 1 << 2;
			if(IsSolid(x + vx, y + vy, z + vz))
				v |= 1 << 3;
			if(IsSolid(x - ux + vx, y - uy + vy, z - uz + vz))
				v |= 1 << 4;
			if(IsSolid(x - ux - vx, y - uy - vy, z - uz - vz))
				v |= 1 << 5;
			if(IsSolid(x + ux + vx, y + uy + vy, z + uz + vz))
				v |= 1 << 6;
			if(IsSolid(x + ux - vx, y + uy - vy, z + uz - vz))
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
		 */
		void GLMapChunk::EmitVertex(int x, int y, int z,
									  int aoX, int aoY, int aoZ,
									  int ux, int uy, int vx, int vy,
									  uint32_t color,
									  int nx, int ny, int nz) {
			SPADES_MARK_FUNCTION_DEBUG();
			
			int uz = (ux == 0 && uy == 0) ? 1 : 0;
			int vz = (vx == 0 && vy == 0) ? 1 : 0;
			Vertex inst;
			// evaluate ambient occlusion
			unsigned int aoID = calcAOID(aoX, aoY, aoZ,
								 ux, uy, uz, vx, vy, vz);
			
			if(nz == 1 || ny == 1){
				inst.shading = 0;
			}else if(nx == 1 || nx == -1){
				inst.shading = 0;//50;
			}else if(nz == -1){
				inst.shading = 220;
			}else{
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
			
			unsigned int aoTexX = aoID & 15;
			unsigned int aoTexY = aoID >> 4;
			aoTexX *= 16; aoTexY *= 16;
			
			uint16_t idx = (uint16_t)vertices.size();
			inst.x = x; inst.y = y; inst.z = z;
			inst.aoX = aoTexX; inst.aoY = aoTexY;
			vertices.push_back(inst);
			inst.x = x + ux; inst.y = y + uy; inst.z = z + uz;
			inst.aoX = aoTexX + 15; inst.aoY = aoTexY;
			vertices.push_back(inst);
			inst.x = x + vx; inst.y = y + vy; inst.z = z + vz;
			inst.aoX = aoTexX; inst.aoY = aoTexY + 15;
			vertices.push_back(inst);
			inst.x = x + ux + vx; inst.y = y + uy + vy; inst.z = z + uz + vz;
			inst.aoX = aoTexX + 15; inst.aoY = aoTexY + 15;
			vertices.push_back(inst);
			
			indices.push_back(idx);
			indices.push_back(idx+1);
			indices.push_back(idx+2);
			indices.push_back(idx+1);
			indices.push_back(idx+3);
			indices.push_back(idx+2);
			
		}
		
		bool GLMapChunk::IsSolid(int x, int y, int z) {
			if(z < 0) return false;
			if(z >= 64) return true;
			
			// FIXME: variable map size
			x &= 511;
			y &= 511;
			
			if(z == 63){
				if(r_water){
					return map->IsSolid(x, y, 62);
				}else{
					return map->IsSolid(x, y, 63);
				}
			}else{
				return map->IsSolid(x, y, z);
			}
		}
		
		void GLMapChunk::Update() {
			SPADES_MARK_FUNCTION();
			
			vertices.clear();
			indices.clear();
			if(buffer){
				device->DeleteBuffer(buffer);
				buffer = 0;
			}
			if(iBuffer){
				device->DeleteBuffer(iBuffer);
				iBuffer = 0;
			}
			
			int rchunkX = chunkX * Size;
			int rchunkY = chunkY * Size;
			int rchunkZ = chunkZ * Size;
			
			int x, y, z;
			for(x = 0; x < Size; x++){
				for(y = 0; y < Size; y++){
					for(z = 0; z < Size; z++){
						int xx = x + rchunkX;
						int yy = y + rchunkY;
						int zz = z + rchunkZ;
						
						if(!IsSolid(xx, yy, zz))
							continue;
						
						uint32_t col = map->GetColor(xx, yy, zz);
						//col = 0xffffffff;
						
						// damaged block?
						int health = col >> 24;
						if(health < 100){
							col &= 0xffffff;
							col &= 0xfefefe;
							col >>= 1;
						}
						
						if(!IsSolid(xx, yy, zz + 1)){
							EmitVertex(x + 1, y, z + 1, xx, yy, zz + 1,
										 -1,0, 0,1,
										 col,
										 0, 0, 1);
						}
						if(!IsSolid(xx, yy, zz - 1)){
							EmitVertex(x, y, z , xx, yy, zz - 1,
										 1,0, 0,1,
										 col,
										 0, 0, -1);
						}
						if(!IsSolid(xx - 1, yy, zz)){
							EmitVertex(x, y + 1, z, xx - 1, yy, zz,
										 0,0, 0,-1,
										 col,
										 -1, 0, 0);
						}
						if(!IsSolid(xx + 1, yy, zz)){
							EmitVertex(x + 1, y , z, xx + 1, yy, zz,
										 0,0, 0,1,
										 col,
										 1, 0, 0);
						}
						if(!IsSolid(xx, yy - 1, zz)){
							EmitVertex(x, y, z, xx, yy - 1, zz,
										 0,0, 1,0,
										 col,
										 0, -1, 0);
						}
						if(!IsSolid(xx, yy + 1, zz)){
							EmitVertex(x + 1, y + 1, z, xx, yy + 1, zz,
										 0,0, -1,0,
										 col,
										 0, 1, 0);
						}
					}
				}
			}
			
			if(vertices.size() == 0)
				return;
			
			buffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			
			device->BufferData(IGLDevice::ArrayBuffer, vertices.size() * sizeof(Vertex),
							   vertices.data(), IGLDevice::DynamicDraw);
			
			if(!indices.empty()){
				iBuffer = device->GenBuffer();
				device->BindBuffer(IGLDevice::ArrayBuffer, iBuffer);
				
				device->BufferData(IGLDevice::ArrayBuffer, indices.size() * sizeof(uint16_t),
								   indices.data(), IGLDevice::DynamicDraw);
				
			}
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			
		}
		
		void GLMapChunk::RenderSunlightPass() {
			SPADES_MARK_FUNCTION();
			Vector3 eye = renderer->renderer->GetSceneDef().viewOrigin;
			
			if(!realized)
				return;
			if(needsUpdate){
				Update();
				needsUpdate = false;
			}
			if(!buffer){
				// empty chunk
				return;
			}
			AABB3 bx = aabb;
			
			
			Vector3 diff = eye - centerPos;
			float sx = 0.f, sy = 0.f;
			// FIXME: variable map size?
			if(diff.x > 256.f) sx += 512.f;
			if(diff.y > 256.f) sy += 512.f;
			if(diff.x < -256.f) sx -= 512.f;
			if(diff.y < -256.f) sy -= 512.f;
			
			bx.min.x += sx; bx.min.y += sy;
			bx.max.x += sx; bx.max.y += sy;
			
			if(!renderer->renderer->BoxFrustrumCull(bx))
				return;
			
			GLProgram *basicProgram = renderer->basicProgram;
			
			static GLProgramUniform chunkPosition("chunkPosition");
			
			chunkPosition(basicProgram);
			chunkPosition.SetValue((float)(chunkX * Size) + sx,
								   (float)(chunkY * Size) + sy,
								   (float)(chunkZ * Size));
			
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute ambientOcclusionCoordAttribute("ambientOcclusionCoordAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");
			
			positionAttribute(basicProgram);
			ambientOcclusionCoordAttribute(basicProgram);
			colorAttribute(basicProgram);
			normalAttribute(basicProgram);
			
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 3,
										IGLDevice::UnsignedByte, false,
										sizeof(Vertex), (void *)0);
			if(ambientOcclusionCoordAttribute() != -1)
				device->VertexAttribPointer(ambientOcclusionCoordAttribute(), 2,
										IGLDevice::UnsignedShort, false,
										sizeof(Vertex), (void *)4);
			device->VertexAttribPointer(colorAttribute(), 4,
										IGLDevice::UnsignedByte, true,
										sizeof(Vertex), (void *)8);
			if(normalAttribute() != -1)
				device->VertexAttribPointer(normalAttribute(), 3,
											IGLDevice::Byte, false,
											sizeof(Vertex), (void *)12);
			
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->BindBuffer(IGLDevice::ElementArrayBuffer,
							   iBuffer);
			device->DrawElements(IGLDevice::Triangles,
								 indices.size(),
								 IGLDevice::UnsignedShort, NULL);
			device->BindBuffer(IGLDevice::ElementArrayBuffer,
							   0);
		
		}
		
		void GLMapChunk::RenderDLightPass(std::vector<GLDynamicLight> lights) {
			SPADES_MARK_FUNCTION();
			Vector3 eye = renderer->renderer->GetSceneDef().viewOrigin;
			
			if(!realized)
				return;
			if(needsUpdate){
				Update();
				needsUpdate = false;
			}
			if(!buffer){
				// empty chunk
				return;
			}
			AABB3 bx = aabb;
			
			
			Vector3 diff = eye - centerPos;
			float sx = 0.f, sy = 0.f;
			// FIXME: variable map size?
			if(diff.x > 256.f) sx += 512.f;
			if(diff.y > 256.f) sy += 512.f;
			if(diff.x < -256.f) sx -= 512.f;
			if(diff.y < -256.f) sy -= 512.f;
			
			bx.min.x += sx; bx.min.y += sy;
			bx.max.x += sx; bx.max.y += sy;
			
			if(!renderer->renderer->BoxFrustrumCull(bx))
				return;
			
			GLProgram *program = renderer->dlightProgram;
			
			static GLProgramUniform chunkPosition("chunkPosition");
			
			chunkPosition(program);
			chunkPosition.SetValue((float)(chunkX * Size) + sx,
								   (float)(chunkY * Size) + sy,
								   (float)(chunkZ * Size));
			
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute colorAttribute("colorAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");
			
			positionAttribute(program);
			colorAttribute(program);
			normalAttribute(program);
			
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(), 3,
										IGLDevice::UnsignedByte, false,
										sizeof(Vertex), (void *)0);
			device->VertexAttribPointer(colorAttribute(), 4,
										IGLDevice::UnsignedByte, true,
										sizeof(Vertex), (void *)8);
			device->VertexAttribPointer(normalAttribute(), 3,
										IGLDevice::Byte, false,
										sizeof(Vertex), (void *)12);
			
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			device->BindBuffer(IGLDevice::ElementArrayBuffer,
							   iBuffer);
			for(size_t i = 0; i < lights.size(); i++){
				
				static GLDynamicLightShader lightShader;
				lightShader(renderer->renderer, program,lights[i], 1);
				
				if(!GLDynamicLightShader::Cull(lights[i], bx))
					continue;
				
				device->DrawElements(IGLDevice::Triangles,
									 indices.size(),
									 IGLDevice::UnsignedShort, NULL);
			}
			
			device->BindBuffer(IGLDevice::ElementArrayBuffer,
							   0);
		
		}

		
		float GLMapChunk::DistanceFromEye(const Vector3 &eye) {
			Vector3 diff = eye - centerPos;
			
			// FIXME: variable map size
			if(diff.x < -256.f) diff.x += 512.f;
			if(diff.y < -256.f) diff.y += 512.f;
			
			if(diff.x > 256.f) diff.x -= 512.f;
			if(diff.y > 256.f) diff.y -= 512.f;
			
			float dist = fabsf(diff.x);
			dist = std::max(dist, fabsf(diff.y));
			dist = std::max(dist, fabsf(diff.z));
			
			//return std::max(diff.GetLength() - radius, 0.f);
			return std::max(dist - ((float)Size * .5f), 0.f);
		}
	}
}
