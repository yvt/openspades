//
//  GLRadiosityRenderer.cpp
//  OpenSpades
//
//  Created by yvt on 8/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//


#include "GLRadiosityRenderer.h"
#include "GLRenderer.h"
#include "../Client/GameMap.h"
#include <stdlib.h>
#include "GLMapShadowRenderer.h"

#include "../Core/ConcurrentDispatch.h"
#ifdef __APPLE__
#include <xmmintrin.h>
#endif

namespace spades {
	namespace draw {
		class GLRadiosityRenderer::UpdateDispatch:
		public ConcurrentDispatch{
			GLRadiosityRenderer *renderer;
		public:
			
			volatile bool done;
			UpdateDispatch(GLRadiosityRenderer *r):
			renderer(r){
				done = false;
			}
			virtual void Run() {
				SPADES_MARK_FUNCTION();
				
				
				// Enable FPE
#if 1
#ifdef __APPLE__
				short fpflags = 0x1332; // Default FP flags, change this however you want.
				__asm__("fnclex\n\tfldcw %0\n\t": "=m"(fpflags));
				
				_MM_SET_EXCEPTION_MASK(_MM_GET_EXCEPTION_MASK() & ~_MM_MASK_INVALID);
#endif
#endif
				
				renderer->UpdateDirtyChunks();
				
				done = true;
			}
		};
		
		GLRadiosityRenderer::GLRadiosityRenderer(GLRenderer *r,
														 client::GameMap *m):
		renderer(r), device(r->GetGLDevice()), map(m){
			SPADES_MARK_FUNCTION();
			
			w = map->Width();
			h = map->Height();
			d = map->Depth();
			
			chunkW = w / ChunkSize;
			chunkH = h / ChunkSize;
			chunkD = d / ChunkSize;
			
			chunks.resize(chunkW * chunkH * chunkD);
			
			for(int i = 0; i < chunks.size(); i++){
				Chunk& c = chunks[i];
				c.dirty = true;
				c.dirtyMinX = 0;
				c.dirtyMinY = 0;
				c.dirtyMinZ = 0;
				c.dirtyMaxX = ChunkSize - 1;
				c.dirtyMaxY = ChunkSize - 1;
				c.dirtyMaxZ = ChunkSize - 1;
				c.transfered = true;
				
				uint16_t *data;
				
				data = (uint16_t *)c.dataFlat;
				std::fill(data, data + ChunkSize*ChunkSize*ChunkSize,
						  0x4210);
				
				data = (uint16_t *)c.dataX;
				std::fill(data, data + ChunkSize*ChunkSize*ChunkSize,
						  0x4210);
				
				data = (uint16_t *)c.dataY;
				std::fill(data, data + ChunkSize*ChunkSize*ChunkSize,
						  0x4210);
				
				data = (uint16_t *)c.dataZ;
				std::fill(data, data + ChunkSize*ChunkSize*ChunkSize,
						  0x4210);
			}
			
			for(int x = 0; x < chunkW; x++)
				for(int y = 0; y < chunkH; y++)
					for(int z = 0; z < chunkD; z++){
						Chunk& c = GetChunk(x, y, z);
						c.cx = x; c.cy = y; c.cz = z;
					}
			
			// make texture
			textureFlat = device->GenTexture();
			textureX = device->GenTexture();
			textureY = device->GenTexture();
			textureZ = device->GenTexture();
			
			IGLDevice::UInteger texs[] = {textureFlat, textureX, textureY, textureZ};
			
			for(int i = 0; i < 4; i++) {
			
				device->BindTexture(IGLDevice::Texture3D, texs[i]);
				device->TexParamater(IGLDevice::Texture3D,
									 IGLDevice::TextureMagFilter,
									 IGLDevice::Linear);
				device->TexParamater(IGLDevice::Texture3D,
									 IGLDevice::TextureMinFilter,
									 IGLDevice::Linear);
				device->TexParamater(IGLDevice::Texture3D,
									 IGLDevice::TextureWrapS,
									 IGLDevice::Repeat);
				device->TexParamater(IGLDevice::Texture3D,
									 IGLDevice::TextureWrapT,
									 IGLDevice::Repeat);
				device->TexParamater(IGLDevice::Texture3D,
									 IGLDevice::TextureWrapR,
									 IGLDevice::ClampToEdge);
				device->TexImage3D(IGLDevice::Texture3D, 0,
								   IGLDevice::RGB5A1,
								   w, h, d, 0,
								   IGLDevice::BGRA,
								   IGLDevice::UnsignedShort1555Rev,
								   NULL);
				
			}
			
			std::vector<uint16_t> v;
			v.resize(w * h);
			std::fill(v.begin(), v.end(),
					  0x4210);
			
			for(int j = 0; j < 4; j++) {
				
				device->BindTexture(IGLDevice::Texture3D, texs[j]);
				for(int i = 0; i < d; i++){
					device->TexSubImage3D(IGLDevice::Texture3D,
										  0, 0, 0, i,
										  w, h, 1,
										  IGLDevice::BGRA,
										  IGLDevice::UnsignedShort1555Rev,
										  v.data());
				}
			}
			dispatch = NULL;
			
		}
		
		GLRadiosityRenderer::~GLRadiosityRenderer() {
			SPADES_MARK_FUNCTION();
			if(dispatch){
				dispatch->Join();
				delete dispatch;
			}
			device->DeleteTexture(textureFlat);
			device->DeleteTexture(textureX);
			device->DeleteTexture(textureY);
			device->DeleteTexture(textureZ);
		}
		
		GLRadiosityRenderer::Result GLRadiosityRenderer::Evaluate(IntVector3 ipos) {
			SPADES_MARK_FUNCTION_DEBUG();
			
			GLRadiosityRenderer::Result result;
			result.base = MakeVector3(0, 0, 0);
			result.x = MakeVector3(0, 0, 0);
			result.y = MakeVector3(0, 0, 0);
			result.z = MakeVector3(0, 0, 0);
			
			Vector3 pos = {ipos.x + .5f, ipos.y + .5f, ipos.z + .5f};
			
			GLMapShadowRenderer *shadowmap = renderer->mapShadowRenderer;
			uint32_t *bitmap = shadowmap->bitmap.data();
			int centerX = ipos.x;
			int centerY = ipos.y - ipos.z;
			const int yMask = h - 1;
			const int pitch = w;
			
			for(int x = -Envelope; x <= Envelope; x++) {
				uint32_t *column = bitmap + ((centerX + x) & (w - 1));
				for(int y = -Envelope; y <= Envelope; y++) {
					uint32_t pixel = column[pitch * ((centerY + y) & yMask)];
					int depth = pixel >> 24;
					
					// shadowmap pixel's world coord
					int wx = centerX + x;
					int wy = centerY + y + depth;
					int wz = depth;
					
					// if true, this is negative-y faced plane
					// if false, this is negative-z faced plane
					bool isSide = pixel & 0x80;
					
					// direction dependent process
					Vector3 center; // center of face
					Vector3 diff; // pos - center
					float diffDot; // dot(diff, normal)
					if(isSide) {
						// normal cull
						if(wy <= ipos.y)
							continue;
						
						center.x = wx + .5f;
						center.y = wy;
						center.z = wz - .5f;
						
						diff = pos - center;
						diffDot = -diff.y;
					}else{
						if(wz <= ipos.z)
							continue;
						
						center.x = wx + .5f;
						center.y = wy + .5f;
						center.z = wz;
						
						diff = pos - center;
						diffDot = -diff.z;
					}
					
					SPAssert(diffDot >= 0.f);
					
					float diffLen = diff.GetLength();
					float invDiffLen = 1.f / diffLen;
					float invDiffLenSmooth = 1.f / ((diffLen) + .4f);
					
					// fallout because of direciton
					float intensity = diffDot * invDiffLen;
					
					// 1/(r^2) distance falloff
					intensity *= invDiffLenSmooth;
					intensity *= invDiffLenSmooth;
					
					// smooth envelope cull
					float distFalloff = 1.f - diffLen * diffLen * (1.f / (Envelope * Envelope + 1));
					if(distFalloff < 0.f)
						continue;
					intensity *= distFalloff;
					
					// normalize
					Vector3 normDiff = diff * -invDiffLen;
					
					// extract shadowmap color
					float red = (pixel) & 0x3f;
					float green = (pixel >> 8) & 0x3f;
					float blue = (pixel >> 16) & 0x3f;
					
					Vector3 color = {red, green, blue};
					color *= intensity;
					
					// add to result
					result.base += color;
					result.x += color * normDiff.x;
					result.y += color * normDiff.y;
					result.z += color * normDiff.z;
					
					SPAssert(!isnan(intensity));
					SPAssert(intensity >= 0.f);
					SPAssert(red >= 0.f && red < 64.f);
					SPAssert(green >= 0.f && green < 64.f);
					SPAssert(blue >= 0.f && blue < 64.f);
				}
			}
			
			float scale = 0.1f / 64.f;
			result.base *= scale;
			result.x *= scale;
			result.y *= scale;
			result.z *= scale;
			
			return result;
		}
		
		void GLRadiosityRenderer::GameMapChanged(int x, int y, int z, client::GameMap * map){
			SPADES_MARK_FUNCTION_DEBUG();
			if(map != this->map)
				return;
			
			Invalidate(x - Envelope, y - Envelope, z - Envelope,
					   x + Envelope, y + Envelope, z + Envelope);
		}
		
		void GLRadiosityRenderer::Invalidate(int minX, int minY, int minZ,
												 int maxX, int maxY, int maxZ) {
			SPADES_MARK_FUNCTION_DEBUG();
			if(minZ < 0) minZ = 0;
			if(maxZ > d-1) maxZ = d-1;
			if(minX > maxX ||
			   minY > maxY ||
			   minZ > maxZ)
				return;
			
			// these should be floor div
			int cx1 = minX >> ChunkSizeBits;
			int cy1 = minY >> ChunkSizeBits;
			int cz1 = minZ >> ChunkSizeBits;
			int cx2 = maxX >> ChunkSizeBits;
			int cy2 = maxY >> ChunkSizeBits;
			int cz2 = maxZ >> ChunkSizeBits;
			
			for(int cx = cx1; cx <= cx2; cx++)
				for(int cy = cy1; cy <= cy2; cy++)
					for(int cz = cz1; cz <= cz2; cz++) {
						Chunk& c = GetChunkWrapped(cx, cy, cz);
						int originX = cx * ChunkSize;
						int originY = cy * ChunkSize;
						int originZ = cz * ChunkSize;
						
						int inMinX = std::max(minX - originX, 0);
						int inMinY = std::max(minY - originY, 0);
						int inMinZ = std::max(minZ - originZ, 0);
						int inMaxX = std::min(maxX - originX, ChunkSize-1);
						int inMaxY = std::min(maxY - originY, ChunkSize-1);
						int inMaxZ = std::min(maxZ - originZ, ChunkSize-1);
						
						if(!c.dirty){
							c.dirtyMinX = inMinX;
							c.dirtyMinY = inMinY;
							c.dirtyMinZ = inMinZ;
							c.dirtyMaxX = inMaxX;
							c.dirtyMaxY = inMaxY;
							c.dirtyMaxZ = inMaxZ;
							c.dirty = true;
						}else{
							c.dirtyMinX = std::min(inMinX, c.dirtyMinX);
							c.dirtyMinY = std::min(inMinY, c.dirtyMinY);
							c.dirtyMinZ = std::min(inMinZ, c.dirtyMinZ);
							c.dirtyMaxX = std::max(inMaxX, c.dirtyMaxX);
							c.dirtyMaxY = std::max(inMaxY, c.dirtyMaxY);
							c.dirtyMaxZ = std::max(inMaxZ, c.dirtyMaxZ);
						}
						
					}
		}
		
		int GLRadiosityRenderer::GetNumDirtyChunks() {
			int cnt = 0;
			for(int i = 0; i < chunks.size(); i++){
				Chunk& c = chunks[i];
				if(c.dirty) cnt++;
			}
			return cnt;
		}
		
		void GLRadiosityRenderer::Update() {
			if(GetNumDirtyChunks() > 0 &&
			   (dispatch == NULL || dispatch->done)){
				if(dispatch){
					dispatch->Join();
					delete dispatch;
				}
				dispatch = new UpdateDispatch(this);
				dispatch->Start();
			}
			
			for(int i = 0; i < chunks.size(); i++){
				Chunk& c = chunks[i];
				if(!c.transfered){
					c.transfered = true;
					device->BindTexture(IGLDevice::Texture3D, textureFlat);
					device->TexSubImage3D(IGLDevice::Texture3D,
										  0,
										  c.cx * ChunkSize,
										  c.cy * ChunkSize,
										  c.cz * ChunkSize,
										  ChunkSize,
										  ChunkSize,
										  ChunkSize,
										  IGLDevice::BGRA,
										  IGLDevice::UnsignedShort1555Rev,
										  c.dataFlat);
					
					
					device->BindTexture(IGLDevice::Texture3D, textureX);
					device->TexSubImage3D(IGLDevice::Texture3D,
										  0,
										  c.cx * ChunkSize,
										  c.cy * ChunkSize,
										  c.cz * ChunkSize,
										  ChunkSize,
										  ChunkSize,
										  ChunkSize,
										  IGLDevice::BGRA,
										  IGLDevice::UnsignedShort1555Rev,
										  c.dataX);
					
					
					device->BindTexture(IGLDevice::Texture3D, textureY);
					device->TexSubImage3D(IGLDevice::Texture3D,
										  0,
										  c.cx * ChunkSize,
										  c.cy * ChunkSize,
										  c.cz * ChunkSize,
										  ChunkSize,
										  ChunkSize,
										  ChunkSize,
										  IGLDevice::BGRA,
										  IGLDevice::UnsignedShort1555Rev,
										  c.dataY);
					
					
					device->BindTexture(IGLDevice::Texture3D, textureZ);
					device->TexSubImage3D(IGLDevice::Texture3D,
										  0,
										  c.cx * ChunkSize,
										  c.cy * ChunkSize,
										  c.cz * ChunkSize,
										  ChunkSize,
										  ChunkSize,
										  ChunkSize,
										  IGLDevice::BGRA,
										  IGLDevice::UnsignedShort1555Rev,
										  c.dataZ);
					
					
					
					
				}
			}
		}
		
		void GLRadiosityRenderer::UpdateDirtyChunks() {
			int dirtyChunkIds[256];
			int numDirtyChunks = 0;
			int nearDirtyChunks = 0;
			
			// first, check only chunks in near range
			Vector3 eyePos = renderer->GetSceneDef().viewOrigin;
			int eyeX = (int)(eyePos.x) >> ChunkSizeBits;
			int eyeY = (int)(eyePos.y) >> ChunkSizeBits;
			int eyeZ = (int)(eyePos.z) >> ChunkSizeBits;
			
			for(int i = 0; i < chunks.size(); i++){
				Chunk& c = chunks[i];
				int dx = (c.cx - eyeX) & (chunkW - 1);
				int dy = (c.cy - eyeY) & (chunkH - 1);
				int dz = (c.cz - eyeZ);
				if(dx >= 6 && dx <= chunkW - 6)
					continue;
				if(dy >= 6 && dy <= chunkW - 6)
					continue;
				if(dz >= 6 || dz <= -6)
					continue;
				if(c.dirty){
					dirtyChunkIds[numDirtyChunks++] = i;
					nearDirtyChunks++;
					if(numDirtyChunks >= 256)
						break;
				}
			}
			
			// far chunks
			if(numDirtyChunks == 0){
				for(int i = 0; i < chunks.size(); i++){
					Chunk& c = chunks[i];
					if(c.dirty){
						dirtyChunkIds[numDirtyChunks++] = i;
						if(numDirtyChunks >= 256)
							break;
					}
				}
			}
			
			// limit update count per frame
			for(int i = 0; i < 8; i++){
				if(numDirtyChunks <= 0)
					break;
				int idx = rand() % numDirtyChunks;
				Chunk& c = chunks[dirtyChunkIds[idx]];
				
				// remove from list (fast)
				if(idx < numDirtyChunks - 1){
					std::swap(dirtyChunkIds[idx], dirtyChunkIds[numDirtyChunks - 1]);
				}
				numDirtyChunks--;
				
				UpdateChunk(c.cx, c.cy, c.cz);
			}
			/*
			printf("%d (%d near) chunk update left\n",
				   GetNumDirtyChunks(), nearDirtyChunks);*/
		}
		
		static float CompressDynamicRange(float v){
			if(v >= 0.f)
				return sqrtf(v);
			else
				return -sqrtf(-v);
		}
		
		static uint16_t EncodeValue(Vector3 vec) {
			float v;
			int iv;
			unsigned int out = 0x8000;
			
			vec.x = CompressDynamicRange(vec.x);
			vec.y = CompressDynamicRange(vec.y);
			vec.z = CompressDynamicRange(vec.z);
			
			vec *= .5f; vec += .5f;
			vec *= 30.f / 31.f;
			
			v = vec.x * 31.f + .5f;
			if(v > 31.2f)v = 31.2f; if(v < 0.f) v = 0.f;
			iv = (unsigned int)v;
			if(iv > 31) iv = 31; if(iv < 0)iv = 0;
			out |= iv << 10;
			
			v = vec.y * 31.f + .5f;
			if(v > 31.2f)v = 31.2f; if(v < 0.f) v = 0.f;
			iv = (unsigned int)v;
			if(iv > 31) iv = 31; if(iv < 0)iv = 0;
			out |= iv << 5;
			
			v = vec.z * 31.f + .5f;
			if(v > 31.2f)v = 31.2f; if(v < 0.f) v = 0.f;
			iv = (unsigned int)v;
			if(iv > 31) iv = 31; if(iv < 0)iv = 0;
			out |= iv;
			
			return (uint16_t)out;
		}
		
		void GLRadiosityRenderer::UpdateChunk(int cx, int cy, int cz) {
			Chunk& c = GetChunk(cx, cy, cz);
			if(!c.dirty)
				return;
			
			int originX = cx * ChunkSize;
			int originY = cy * ChunkSize;
			int originZ = cz * ChunkSize;
			
			for(int z = c.dirtyMinZ; z <= c.dirtyMaxZ; z++)
				for(int y = c.dirtyMinY; y <= c.dirtyMaxY; y++)
					for(int x = c.dirtyMinX; x <= c.dirtyMaxX; x++)
					{
						IntVector3 pos;
						pos.x = (x + originX);
						pos.y = (y + originY);
						pos.z = (z + originZ);
						
						Result res = Evaluate(pos);
						c.dataFlat[z][y][x] = EncodeValue(res.base);
						c.dataX[z][y][x] = EncodeValue(res.x);
						c.dataY[z][y][x] = EncodeValue(res.y);
						c.dataZ[z][y][x] = EncodeValue(res.z);
					}
			
			c.dirty = false;
			c.transfered = false;
		}
	}
}
