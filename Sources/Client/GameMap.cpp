//
//  GameMap.cpp
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GameMap.h"
#include "../Core/IStream.h"
#include <vector>
#include "../Core/Exception.h"
#include <math.h>
#include "../Core/Debug.h"
#include <stdlib.h>

// silly VOXLAP function
static inline void ftol(float f, long *a)
{
    *a = (long)floor(f+0.5f);
}

namespace spades {
	namespace client {
		GameMap::GameMap():
		listener(NULL){
			SPADES_MARK_FUNCTION();
			
			
			for(int x = 0; x < DefaultWidth; x++)
				for(int y = 0; y < DefaultHeight; y++){
					solidMap[x][y] = 1; // ground only
					for(int z = 0; z < DefaultDepth; z++){
						uint32_t col = 0x00284067;
						uint32_t rnd = (uint32_t)rand() ^ ((uint32_t)rand() << 16);
						col ^= 0x070707 & rnd;
						colorMap[x][y][z] = col + (100UL * 0x1000000UL);
					}
				}
		}
		GameMap::~GameMap(){
			SPADES_MARK_FUNCTION();
			
		}
		
		bool GameMap::ClipBox(int x, int y, int z) {
			int sz;
			
			if (x < 0 || x >= 512 || y < 0 || y >= 512)
				return true;
			else if (z < 0)
				return false;
			sz = (int)z;
			if(sz == 63)
				sz=62;
			else if (sz >= 64)
				return true;
			return IsSolid((int)x, (int)y, sz);
		}
		
		bool GameMap::ClipWorld(int x, int y, int z){
			int sz;
			
			if (x < 0 || x >= 512 || y < 0 || y >= 512)
				return 0;
			if (z < 0)
				return 0;
			sz = (int)z;
			if(sz == 63)
				sz=62;
			else if (sz >= 63)
				return 1;
			else if (sz < 0)
				return 0;
			return IsSolid((int)x, (int)y, sz);
		}
		
		bool GameMap::ClipBox(float x, float y, float z){
			SPAssert(!isnan(x));
			SPAssert(!isnan(y));
			SPAssert(!isnan(z));
			return ClipBox((int)floorf(x),
						   (int)floorf(y),
						   (int)floorf(z));
		}
		
		bool GameMap::ClipWorld(float x, float y, float z){
			SPAssert(!isnan(x));
			SPAssert(!isnan(y));
			SPAssert(!isnan(z));
			return ClipWorld((int)floorf(x),
						   (int)floorf(y),
						   (int)floorf(z));
		}

		bool GameMap::CastRay(spades::Vector3 v0,
							  spades::Vector3 v1,
							  float length,
							  spades::IntVector3 &vOut) {
			SPADES_MARK_FUNCTION_DEBUG();
			
			SPAssert(!isnan(v0.x));
			SPAssert(!isnan(v0.y));
			SPAssert(!isnan(v0.z));
			SPAssert(!isnan(v1.x));
			SPAssert(!isnan(v1.y));
			SPAssert(!isnan(v1.z));
			SPAssert(!isnan(length));
			
			v1 = v0 + v1 * length;
			
			Vector3 f, g;
			IntVector3 a, c, d, p, i;
			long cnt = 0;
			
			a = v0.Floor();
			c = v1.Floor();
			
			if (c.x <  a.x) {
				d.x = -1; f.x = v0.x-a.x; g.x = (v0.x-v1.x)*1024; cnt += a.x-c.x;
			}
			else if (c.x != a.x) {
				d.x =  1; f.x = a.x+1-v0.x; g.x = (v1.x-v0.x)*1024; cnt += c.x-a.x;
			}
			else
				f.x = g.x = 0;
			if (c.y <  a.y) {
				d.y = -1; f.y = v0.y-a.y;   g.y = (v0.y-v1.y)*1024; cnt += a.y-c.y;
			}
			else if (c.y != a.y) {
				d.y =  1; f.y = a.y+1-v0.y; g.y = (v1.y-v0.y)*1024; cnt += c.y-a.y;
			}
			else
				f.y = g.y = 0;
			if (c.z <  a.z) {
				d.z = -1; f.z = v0.z-a.z;   g.z = (v0.z-v1.z)*1024; cnt += a.z-c.z;
			}
			else if (c.z != a.z) {
				d.z =  1; f.z = a.z+1-v0.z; g.z = (v1.z-v0.z)*1024; cnt += c.z-a.z;
			}
			else
				f.z = g.z = 0;
			
			Vector3 pp = MakeVector3(f.x * g.z - f.z * g.x,
									 f.y * g.z - f.z * g.y,
									 f.y * g.x - f.x * g.y);
			p = pp.Floor();
			i = g.Floor();
			
			if(cnt > (long)length)
				cnt = (long)length;
			
#if 1 
			// faster version
			uint64_t lastSolidMap = solidMap[a.x&(DefaultWidth-1)]
			[a.y&(DefaultHeight-1)];
			if(a.z < 0 && d.z < 0){
				return false;
			}else if(a.z < 0){
				while(cnt > 0 && a.z < 0){
					if (((p.x|p.y) >= 0) && (a.z != c.z)) {
						a.z += d.z; p.x -= i.x; p.y -= i.y;
					}
					else if ((p.z >= 0) && (a.x != c.x)) {
						a.x += d.x; p.x += i.z; p.z -= i.y;
					}
					else {
						a.y += d.y; p.y += i.z; p.z += i.x;
					}
					cnt--;
				}
			}else if(a.z >= 64){
				vOut = a;
				return true;
			}
			while(cnt > 0){
				if (((p.x|p.y) >= 0) && (a.z != c.z)) {
					a.z += d.z; p.x -= i.x; p.y -= i.y;
					if(a.z < 0 && d.z < 0){
						return false;
					}else if (a.z >= 64) {
						vOut = a;
						return true;
					}
				}
				else if ((p.z >= 0) && (a.x != c.x)) {
					a.x += d.x; p.x += i.z; p.z -= i.y;
					lastSolidMap = solidMap[a.x&(DefaultWidth-1)]
					[a.y&(DefaultHeight-1)];
				}
				else {
					a.y += d.y; p.y += i.z; p.z += i.x;
					lastSolidMap = solidMap[a.x&(DefaultWidth-1)]
					[a.y&(DefaultHeight-1)];
				}
				
				if ((lastSolidMap >> (uint64_t)a.z) & 1ULL) {
					vOut = a;
					return true;
				}
				cnt--;
			}
#else
			while(cnt > 0){
				if (((p.x|p.y) >= 0) && (a.z != c.z)) {
					a.z += d.z; p.x -= i.x; p.y -= i.y;
				}
				else if ((p.z >= 0) && (a.x != c.x)) {
					a.x += d.x; p.x += i.z; p.z -= i.y;
				}
				else {
					a.y += d.y; p.y += i.z; p.z += i.x;
				}
				
				if (IsSolidWrapped(a.x, a.y, a.z)) {
					vOut = a;
					return true;
				}
				cnt--;
			}
#endif
			return false;
		}
		
		GameMap::RayCastResult GameMap::CastRay2(spades::Vector3 v0,
												 spades::Vector3 dir,
												 int maxSteps) {
			SPADES_MARK_FUNCTION_DEBUG();
			GameMap::RayCastResult result;
			
			SPAssert(!isnan(v0.x));
			SPAssert(!isnan(v0.y));
			SPAssert(!isnan(v0.z));
			SPAssert(!isnan(dir.x));
			SPAssert(!isnan(dir.y));
			SPAssert(!isnan(dir.z));
			
			dir = dir.Normalize();
			
			spades::IntVector3 iv = v0.Floor();
			spades::Vector3 fv;
			if(IsSolidWrapped(iv.x, iv.y, iv.z)) {
				result.hit = true;
				result.startSolid = true;
				result.hitPos = v0;
				result.hitBlock = iv;
				result.normal = IntVector3::Make(0,0,0);
				return result;
			}
			
			if(dir.x > 0.f){
				fv.x = (float)(iv.x + 1) - v0.x;
			}else{
				fv.x = v0.x - (float)iv.x;
			}
			if(dir.y > 0.f){
				fv.y = (float)(iv.y + 1) - v0.y;
			}else{
				fv.y = v0.y - (float)iv.y;
			}
			if(dir.z > 0.f){
				fv.z = (float)(iv.z + 1) - v0.z;
			}else{
				fv.z = v0.z - (float)iv.z;
			}
			
			float invX = dir.x;
			float invY = dir.y;
			float invZ = dir.z;
			
			if(invX != 0.f) invX = 1.f / fabsf(invX);
			if(invY != 0.f) invY = 1.f / fabsf(invY);
			if(invZ != 0.f) invZ = 1.f / fabsf(invZ);
			
			for(int i = 0; i < maxSteps; i++){
				IntVector3 nextBlock;
				int hasNextBlock = 0;
				float nextBlockTime;
				
				if(invX != 0.f){
					nextBlock = iv;
					if(dir.x > 0.f) nextBlock.x++;
					else nextBlock.x--;
					nextBlockTime = fv.x * invX;
					hasNextBlock = 1;
				}
				if(invY != 0.f){
					float t = fv.y * invY;
					if(!hasNextBlock || t < nextBlockTime){
						nextBlock = iv;
						if(dir.y > 0.f) nextBlock.y++;
						else nextBlock.y--;
						nextBlockTime = t;
						hasNextBlock = 2;
					}
				}
				if(invZ != 0.f){
					float t = fv.z * invZ;
					if(!hasNextBlock || t < nextBlockTime){
						nextBlock = iv;
						if(dir.z > 0.f) nextBlock.z++;
						else nextBlock.z--;
						nextBlockTime = t;
						hasNextBlock = 3;
					}
				}
				SPAssert(hasNextBlock != 0); // must hit a plane
				SPAssert(hasNextBlock == 1 || // x-plane
					   hasNextBlock == 2 || // y-plane
					   hasNextBlock == 3);  // z-plane
				
				if(hasNextBlock == 1){
					fv.x = 1.f;
				}else{
					fv.x -= fabsf(dir.x) * nextBlockTime;
				}
				if(hasNextBlock == 2){
					fv.y = 1.f;
				}else{
					fv.y -= fabsf(dir.y) * nextBlockTime;
				}
				if(hasNextBlock == 3){
					fv.z = 1.f;
				}else{
					fv.z -= fabsf(dir.z) * nextBlockTime;
				}
				
				if(IsSolidWrapped(nextBlock.x,
								  nextBlock.y,
								  nextBlock.z)){
					// hit.
					Vector3 hitPos;
					if(dir.x > 0.f){
						hitPos.x = (float)(nextBlock.x+1)-fv.x;
					}else{
						hitPos.x = (float)nextBlock.x+fv.x;
					}
					if(dir.y > 0.f){
						hitPos.y = (float)(nextBlock.y+1)-fv.y;
					}else{
						hitPos.y = (float)nextBlock.y+fv.y;
					}
					if(dir.z > 0.f){
						hitPos.z = (float)(nextBlock.z+1)-fv.z;
					}else{
						hitPos.z = (float)nextBlock.z+fv.z;
					}
					
					result.hit = true;
					result.startSolid = false;
					result.hitPos = hitPos;
					result.hitBlock = nextBlock;
					result.normal = iv - nextBlock;
					return result;
				}else{
					iv = nextBlock;
				}
			}
			
			result.hit = false;
			result.startSolid = false;
			result.hitPos = v0;
			return result;
		}
		
		static uint32_t swapColor(uint32_t col ){
			union {
				uint8_t bytes[4];
				uint32_t c;
			} u;
			u.c = col;
			std::swap(u.bytes[0], u.bytes[2]);
			return (u.c & 0xffffff) | (100UL * 0x1000000);
		}
		
		GameMap *GameMap::Load(spades::IStream *stream) {
			SPADES_MARK_FUNCTION();
			
			std::string bytes = stream->ReadAllBytes();
			size_t len = bytes.size();
			size_t pos = 0;
			
			GameMap *map = new GameMap();
			try{
				for(int y = 0; y < 512; y++){
					for(int x = 0; x < 512; x++){
						map->solidMap[x][y] = 0xffffffffffffffffULL;
						
						if(pos + 2 >= len){
							SPRaise("File truncated");
						}
						
						int z = 0;
						for(;;){
							int i;
							uint32_t *color;
							int number_4byte_chunks = bytes[pos];
							int top_color_start = bytes[pos + 1];
							int top_color_end = bytes[pos + 2];
							int bottom_color_start;
							int bottom_color_end;
							int len_top;
							int len_bottom;
							
							for(i = z; i < top_color_start; i++)
								map->Set(x, y, i, false, 0);
							
							if(pos + 4 + top_color_end - top_color_start + 3 >= len){
								SPRaise("File truncated");
							}
							
							color = (uint32_t *)(bytes.data() + pos + 4);
							for(z = top_color_start; z <= top_color_end; z++)
								map->Set(x, y, z, true, swapColor(*(color++)));
							
							len_bottom = top_color_end - top_color_start + 1;
							
							if(number_4byte_chunks == 0){
								pos += 4 * (len_bottom + 1);
								break;
							}
							
							len_top = (number_4byte_chunks - 1) - len_bottom;
							
							pos += (int)bytes[pos] * 4;
							
							if(pos + 3 >= len){
								SPRaise("File truncated");
							}
							
							bottom_color_end = bytes[pos + 3];
							bottom_color_start = bottom_color_end - len_top;
							
							for(z = bottom_color_start; z < bottom_color_end; z++){
								map->Set(x, y, z, true, swapColor(*(color++)));
							}
						}
						
					}
				}
				
				return map;
			}catch(...){
				delete map;
				throw;
			}
		}
	}
}
