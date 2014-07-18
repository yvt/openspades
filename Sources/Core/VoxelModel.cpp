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

#include "VoxelModel.h"
#include "Exception.h"
#include <algorithm>
#include <vector>
#include <cstring>
#include "Debug.h"
#include <ScriptBindings/ScriptManager.h>
#include "FileManager.h"

namespace spades {
	VoxelModel::VoxelModel(int w, int h, int d) {
		SPADES_MARK_FUNCTION();
		
		if(w < 1 || h < 1 || d < 1 || w > 4096 || h > 4096)
			SPRaise("Invalid dimension: %dx%dx%d", w, h, d);
		
		width = w;
		height = h;
		depth = d;
		if(d > 64){
			SPRaise("Voxel model with depth > 64 is not supported.");
		}
		
		solidBits = new uint64_t[w * h];
		colors = new uint32_t[w * h * d];
		
		std::fill(solidBits, solidBits + w * h, 0);
		
		
	}
	VoxelModel::~VoxelModel() {
		SPADES_MARK_FUNCTION();
		
		delete[] solidBits;
		delete[] colors;
	}
	
	void VoxelModel::AddListener(VoxelModelListener *l) {
		auto it = std::find(listeners.begin(), listeners.end(), l);
		if (it == listeners.end()) {
			listeners.push_back(l);
		}
	}
	
	void VoxelModel::RemoveListener(VoxelModelListener *l) {
		auto it = std::find(listeners.begin(), listeners.end(), l);
		if (it != listeners.end()) {
			listeners.erase(it);
		}
	}
	
	struct KV6Block {
		uint32_t color;
		uint16_t zPos;
		uint8_t visFaces;
		uint8_t lighting;
	};
	
	struct KV6Header {
		uint32_t xsiz, ysiz, zsiz;
		float xpivot, ypivot, zpivot;
		uint32_t blklen;
	};
	static uint32_t swapColor(uint32_t col ){
		union {
			uint8_t bytes[4];
			uint32_t c;
		} u;
		u.c = col;
		std::swap(u.bytes[0], u.bytes[2]);
		return (u.c & 0xffffff);
	}
	
	void VoxelModel::HollowFill() {
		std::vector<IntVector3> stack;
		std::vector<uint8_t> flags;
		flags.resize(width * height * depth);
		std::memset(flags.data(), 0, flags.size());
		
		stack.reserve(width * height * depth);
		
		auto Flag = [&](int x, int y, int z) -> uint8_t& {
			return flags[x + width * (y + height * z)];
		};
		
		for(int x = 0; x < width; x++) for(int y = 0; y < height; y++) {
			auto m = GetSolidBitsAt(x, y);
			for(int z = 0; z < depth; z++){
				if(m & (1ULL << z)) {
					Flag(x, y, z) = 1;
				}
			}
		}
		for(int x = 0; x < width; x++) for(int y = 0; y < height; y++){
			if(!IsSolid(x, y, 0)){
				stack.push_back(IntVector3::Make(x, y, 0));
				Flag(x, y, 0) = 1;
			}
			if(!IsSolid(x, y, depth - 1)){
				stack.push_back(IntVector3::Make(x, y, depth - 1));
				Flag(x, y, depth - 1) = 1;
			}
		}
		for(int x = 0; x < width; x++) for(int z = 1; z < depth - 1; z++){
			if(!IsSolid(x, 0, z)){
				stack.push_back(IntVector3::Make(x, 0, z));
				Flag(x, 0, z) = 1;
			}
			if(!IsSolid(x, height - 1, z)){
				stack.push_back(IntVector3::Make(x, height - 1, z));
				Flag(x, height - 1, z) = 1;
			}
		}
		for(int y = 1; y < height - 1; y++) for(int z = 1; z < depth - 1; z++){
			if(!IsSolid(0, y, z)){
				stack.push_back(IntVector3::Make(0, y, z));
				Flag(0, y, z) = 1;
			}
			if(!IsSolid(width - 1, y, z)){
				stack.push_back(IntVector3::Make(width - 1, y, z));
				Flag(width - 1, y, z) = 1;
			}
		}
		
		while(!stack.empty()){
			auto v = stack.back();
			stack.pop_back();
			
			auto Visit = [&](int x, int y, int z) {
				SPAssert(x >= 0);
				SPAssert(x < width);
				SPAssert(y >= 0);
				SPAssert(y < height);
				SPAssert(z >= 0);
				SPAssert(z < depth);
				Flag(x, y, z) = 1;
				stack.push_back(IntVector3::Make(x, y, z));
			};
			
			if(v.x > 0 && !Flag(v.x - 1, v.y, v.z)) Visit(v.x - 1, v.y, v.z);
			if(v.x < width-1 && !Flag(v.x + 1, v.y, v.z)) Visit(v.x + 1, v.y, v.z);
			if(v.y > 0 && !Flag(v.x, v.y - 1, v.z)) Visit(v.x, v.y - 1, v.z);
			if(v.y < height-1 && !Flag(v.x, v.y + 1, v.z)) Visit(v.x, v.y + 1, v.z);
			if(v.z > 0 && !Flag(v.x, v.y, v.z - 1)) Visit(v.x, v.y, v.z - 1);
			if(v.z < depth-1 && !Flag(v.x, v.y, v.z + 1)) Visit(v.x, v.y, v.z + 1);
		}
		
		for(int z = 0, idx = 0; z < depth; z++)
			for(int y = 0; y < height; y++)
				for(int x = 0; x < width; x++, idx++)  {
					if(!flags[idx]) {
						SPAssert(!IsSolid(x, y, z));
						SetSolid(x, y, z, 0xddbeef);
					}
				}
		
		for(int z = 0, idx = 0; z < depth; z++)
			for(int y = 0; y < height; y++)
				for(int x = 0; x < width; x++, idx++)  {
					if(!flags[idx]) {
						SPAssert(IsSolid(x, y, z));
						SPAssert(IsSolid(x+1, y, z));
						SPAssert(IsSolid(x-1, y, z));
						SPAssert(IsSolid(x, y+1, z));
						SPAssert(IsSolid(x, y-1, z));
						SPAssert(IsSolid(x, y, z+1));
						SPAssert(IsSolid(x, y, z-1));
					}
				}
		
		
	}
	
	VoxelModel *VoxelModel::LoadKV6(spades::IStream *stream) {
		SPADES_MARK_FUNCTION();
		
		if(stream->Read(4) != "Kvxl"){
			SPRaise("Invalid magic");
		}
		
		KV6Header header;
		if(stream->Read(&header, sizeof(header)) < sizeof(header)){
			SPRaise("File truncated: failed to read header");
		}
		
		std::vector<KV6Block> blkdata;
		blkdata.resize(header.blklen);
		
		if(stream->Read(blkdata.data(), sizeof(KV6Block) * header.blklen) <
		   sizeof(KV6Block) * header.blklen){
			SPRaise("File truncated: failed to read blocks");
		}
		
		std::vector<uint32_t> xoffset;
		xoffset.resize(header.xsiz);
		
		if(stream->Read(xoffset.data(), sizeof(uint32_t) * header.xsiz) <
		   sizeof(uint32_t) * header.xsiz){
			SPRaise("File truncated: failed to read xoffset");
		}
		
		std::vector<uint16_t> xyoffset;
		xyoffset.resize(header.xsiz * header.ysiz);
		
		if(stream->Read(xyoffset.data(), sizeof(uint16_t) * header.xsiz * header.ysiz) <
		   sizeof(uint16_t) * header.xsiz * header.ysiz){
			SPRaise("File truncated: failed to read xyoffset");
		}
		
		// validate: zpos < depth
		for(size_t i = 0; i < blkdata.size(); i++)
			if(blkdata[i].zPos >= header.zsiz)
				SPRaise("File corrupted: blkData[i].zPos >= header.zsiz");
		
		// validate sum(xyoffset) = blkLen
		{
			uint64_t ttl = 0;
			for(size_t i = 0; i < xyoffset.size(); i++){
				ttl += (uint32_t)xyoffset[i];
			}
			if(ttl != (uint64_t)blkdata.size())
				SPRaise("File corrupted: sum(xyoffset) != blkdata.size()");
		}
		
		VoxelModel *model = new VoxelModel(header.xsiz, header.ysiz, header.zsiz);
		model->SetOrigin(MakeVector3(-header.xpivot,
									 -header.ypivot,
									 -header.zpivot));
		try{
			int pos = 0;
			for(int x = 0; x < (int)header.xsiz; x++)
				for(int y = 0; y < (int)header.ysiz; y++){
					int spanBlocks = (int)xyoffset[x *header.ysiz + y];
					int lastZ = -1;
					//printf("pos: %d, (%d, %d): %d\n",
					//	   pos, x, y, spanBlocks);
					while(spanBlocks--){
						const KV6Block& b = blkdata[pos];
						//printf("%d, %d, %d: %d, %d\n", x, y, b.zPos,
						//	   b.visFaces, b.lighting);
						if(model->IsSolid(x, y, b.zPos)){
							SPRaise("Duplicate voxel (%d, %d, %d)",
									x, y, b.zPos);
						}
						if(b.zPos <= lastZ){
							SPRaise("Not Z-sorted");
						}
						lastZ = b.zPos;
						model->SetSolid(x, y, b.zPos,
										swapColor(b.color));
						pos++;
					}
				}
			
			SPAssert(pos == blkdata.size());
			model->HollowFill();
			return model;
		}catch(...){
			delete model;
			throw;
		}
	}
	
	auto VoxelModel::CastRay(Vector3 v0, Vector3 dir)
	-> RayCastResult {
		RayCastResult result;
		result.hit = false;
		
		SPAssert(dir.x != 0.f || dir.y != 0.f || dir.z != 0.f);
		
		// filter out invalid value
		if (!isfinite(v0.x) || !isfinite(v0.y) || !isfinite(v0.z)) {
			return result;
		}
		if (!isfinite(dir.x) || !isfinite(dir.y) || !isfinite(dir.z)) {
			return result;
		}
		
		Vector3 orig = GetOrigin() - .5f;
		
		// transform ray into local space
		v0 -= orig;
		
		// find intersection
		const float fw = width, fh = height, fd = depth;
		if (v0.y < 0.f && dir.y <= 0.f) return result;
		if (v0.z < 0.f && dir.z <= 0.f) return result;
		if (v0.y >= fh && dir.y >= 0.f) return result;
		if (v0.z >= fd && dir.z >= 0.f) return result;
		
		if (v0.x >= 0.f && v0.y >= 0.f && v0.z >= 0.f &&
			v0.x < fw && v0.y < fh && v0.z < fd) {
			// start point is in boundary
			if (IsSolid(static_cast<int>(v0.x),
						static_cast<int>(v0.y),
						static_cast<int>(v0.z))) {
				result.hit = true;
				result.startSolid = true;
				result.hitBlock = v0.Floor();
				result.hitPos = v0 + orig;
				result.normal = IntVector3(0, 0, 0);
				return result;
			}
		} else {
			// start point is outside
			if (v0.x < 0.f) {
				if (dir.x <= 0.f) {
					return result;
				}
				float per = -v0.x / dir.x;
				v0 += dir * per; v0.x = 0.f;
				
				if (v0.y >= 0.f && v0.z >= 0.f &&
					v0.y < fh && v0.z < fd &&
					IsSolid(static_cast<int>(0),
							static_cast<int>(v0.y),
							static_cast<int>(v0.z)))
				{
					result.hit = true;
					result.startSolid = false;
					result.hitBlock = IntVector3
					(0,
					 static_cast<int>(v0.y),
					 static_cast<int>(v0.z));
					result.hitPos = v0 + orig;
					result.normal = IntVector3(-1, 0, 0);
					return result;
				}
			} else if (v0.x > fw) {
				if (dir.x >= 0.f) {
					return result;
				}
				float per = (fw - v0.x) / dir.x;
				v0 += dir * per; v0.x = fw;
				
				if (v0.y >= 0.f && v0.z >= 0.f &&
					v0.y < fh && v0.z < fd &&
					IsSolid(static_cast<int>(width - 1),
							static_cast<int>(v0.y),
							static_cast<int>(v0.z)))
				{
					result.hit = true;
					result.startSolid = false;
					result.hitBlock = IntVector3
					(width - 1,
					 static_cast<int>(v0.y),
					 static_cast<int>(v0.z));
					result.hitPos = v0 + orig;
					result.normal = IntVector3(1, 0, 0);
					return result;
				}
			}
			
			if (v0.y < 0.f) {
				if (dir.y <= 0.f) {
					return result;
				}
				float per = -v0.y / dir.y;
				v0 += dir * per; v0.y = 0.f;
				
				if (v0.x >= 0.f && v0.z >= 0.f &&
					v0.x < fw && v0.z < fd &&
					IsSolid(static_cast<int>(v0.x),
							0,
							static_cast<int>(v0.z)))
				{
					result.hit = true;
					result.startSolid = false;
					result.hitBlock = IntVector3
					(static_cast<int>(v0.x),
					 0,
					 static_cast<int>(v0.z));
					result.hitPos = v0 + orig;
					result.normal = IntVector3(0, -1, 0);
					return result;
				}
			} else if (v0.y > fh) {
				if (dir.y >= 0.f) {
					return result;
				}
				float per = (fh - v0.y) / dir.y;
				v0 += dir * per; v0.y = fh;
				
				if (v0.x >= 0.f && v0.z >= 0.f &&
					v0.x < fw && v0.z < fd &&
					IsSolid(static_cast<int>(v0.x),
							height - 1,
							static_cast<int>(v0.z)))
				{
					result.hit = true;
					result.startSolid = false;
					result.hitBlock = IntVector3
					(static_cast<int>(v0.x),
					 height - 1,
					 static_cast<int>(v0.z));
					result.hitPos = v0 + orig;
					result.normal = IntVector3(0, 1, 0);
					return result;
				}
			}
			if (v0.z < 0.f) {
				if (dir.z <= 0.f) {
					return result;
				}
				float per = -v0.z / dir.z;
				v0 += dir * per; v0.y = 0.f;
				
				if (v0.x >= 0.f && v0.y >= 0.f &&
					v0.x < fw && v0.y < fh &&
					IsSolid(static_cast<int>(v0.x),
							static_cast<int>(v0.y),
							0))
				{
					result.hit = true;
					result.startSolid = false;
					result.hitBlock = IntVector3
					(static_cast<int>(v0.x),
					 static_cast<int>(v0.y),
					 0);
					result.hitPos = v0 + orig;
					result.normal = IntVector3(0, 0, -1);
					return result;
				}
			} else if (v0.z > fd) {
				if (dir.z >= 0.f) {
					return result;
				}
				float per = (fd - v0.z) / dir.z;
				v0 += dir * per; v0.z = fd;
				
				if (v0.x >= 0.f && v0.y >= 0.f &&
					v0.x < fw && v0.y < fh &&
					IsSolid(static_cast<int>(v0.x),
							static_cast<int>(v0.y),
							depth - 1))
				{
					result.hit = true;
					result.startSolid = false;
					result.hitBlock = IntVector3
					(static_cast<int>(v0.x),
					 static_cast<int>(v0.y),
					 depth - 1);
					result.hitPos = v0 + orig;
					result.normal = IntVector3(0, 0, 1);
					return result;
				}
			}
			if (v0.x < 0.f || v0.y < 0.f ||
				v0.x > fw || v0.y > fh) {
				// doesn' intersect
				return result;
			}
			
		}
		
		SPAssert(v0.x >= 0.f);
		SPAssert(v0.y >= 0.f);
		SPAssert(v0.z >= 0.f);
		SPAssert(v0.x <= fw);
		SPAssert(v0.y <= fh);
		SPAssert(v0.z <= fd);
		
		// check special pattern
		if (dir.x == 0.f && dir.y == 0.f) {
			SPAssert(v0.x < fw);
			SPAssert(v0.y < fh);
			auto mp = GetSolidBitsAt(static_cast<int>(v0.x),
									 static_cast<int>(v0.y));
			int z = static_cast<int>(v0.z);
			if (dir.z > 0.f) {
				++z;
				if (z >= 64) return result;
				mp >>= z;
				while (mp) {
					if (mp & 1) {
						result.hit = true;
						result.startSolid = false;
						v0.z = z;
						result.hitBlock = IntVector3
						(static_cast<int>(v0.x),
						 static_cast<int>(v0.y),
						 z);
						result.hitPos = v0 + orig;
						result.normal = IntVector3(0, 0, -1);
						return result;
					}
					mp >>= 1; ++z;
				}
			} else {
				--z;
				if (z < 0) return result;
				z = std::min(z, 63);
				mp = mp << (63 - z);
				while (mp) {
					if (mp & 0x8000000000000000ULL) {
						result.hit = true;
						result.startSolid = false;
						v0.z = z + 1;
						result.hitBlock = IntVector3
						(static_cast<int>(v0.x),
						 static_cast<int>(v0.y),
						 z);
						result.hitPos = v0 + orig;
						result.normal = IntVector3(0, 0, 1);
						return result;
					}
					mp = mp << 1; --z;
				}
			}
			return result;
		}
		
		const int signX = dir.x > 0.f ? 1 : dir.x < 0.f ? -1 : 0;
		const int signY = dir.y > 0.f ? 1 : dir.y < 0.f ? -1 : 0;
		const int signZ = dir.z > 0.f ? 1 : dir.z < 0.f ? -1 : 0;
		
		SPAssert(signX != 0 || signY != 0);
		
		int ix = signX >= 0 ?
		static_cast<int>(v0.x) :
		std::max(static_cast<int>(ceilf(v0.x)) - 1, 0);
		int iy = signY >= 0 ?
		static_cast<int>(v0.y) :
		std::max(static_cast<int>(ceilf(v0.y)) - 1, 0);
		int iz = signZ >= 0 ?
		static_cast<int>(v0.z) :
		std::max(static_cast<int>(ceilf(v0.z)) - 1, 0);
		
		float fx = signX >= 0 ?
		(ix + 1) - v0.x : v0.x - ix;
		float fy = signX >= 0 ?
		(iy + 1) - v0.y : v0.y - iy;
		
		float zz = v0.z;
		
		const float invDirX = dir.x != 0.f ? 1.f / dir.x : 0.f;
		const float invDirY = dir.y != 0.f ? 1.f / dir.y : 0.f;
		const float invDirZ = dir.z != 0.f ? 1.f / dir.z : 0.f;
		
		const float invDirXAbs = fabsf(invDirX);
		const float invDirYAbs = fabsf(invDirY);
		
		auto getCurrentPos = [&]() {
			Vector3 v;
			v.z = zz;
			v.x = signX >= 0 ?
			(ix + 1) - fx : ix + fx;
			v.y = signY >= 0 ?
			(iy + 1) - fy : iy + fy;
			return v;
		};
		
		while (ix >= 0 && iy >= 0 && ix < width && iy < height) {
			int nextX = ix, nextY = iy;
			float newFx = fx, newFy = fy;
			float nextZf; int nextZ;
			float dt;
			int side = 0;
			if (signX == 0) {
				nextX = ix; nextY = iy + signY;
				dt = fy * invDirYAbs;
				newFy = 1.f;
			} else if (signY == 0) {
				nextX = ix + signX; nextY = iy;
				dt = fx * invDirXAbs;
				newFx = 1.f;
				side = 1;
			} else {
				float dtX = fx * invDirXAbs;
				float dtY = fy * invDirYAbs;
				if (dtX < dtY) {
					dt = dtX;
					nextX = ix + signX; nextY = iy;
					newFx = 1.f;
					newFy -= dt * fabsf(dir.y);
				} else {
					dt = dtY;
					nextX = ix; nextY = iy + signY;
					newFx -= dt * fabsf(dir.x);
					newFy = 1.f;
					side = 1;
				}
			}
			
			if (signZ == 0) {
				nextZ = iz; nextZf = zz;
			} else {
				nextZf = zz + dt * dir.z;
				nextZ = signZ >= 0 ?
				static_cast<int>(v0.z) :
				static_cast<int>(ceilf(v0.z)) - 1;
			}
			
			// move in the current (x, y)
			if (nextZ != iz) {
				auto map = GetSolidBitsAt(ix, iy);
				if (nextZ < iz) {
					while (iz > nextZ) {
						--iz;
						if (iz < 0) return result;
						if (map & (1ULL << iz)) {
							result.hit = true;
							result.startSolid = false;
							result.hitBlock = IntVector3
							(ix, iy, iz);
							fx -= fabsf(dir.x) * ((iz + 1) - zz) * invDirZ;
							fy -= fabsf(dir.y) * ((iz + 1) - zz) * invDirZ;
							result.hitPos = getCurrentPos();
							result.hitPos.z = iz + 1;
							result.hitPos += orig;
							result.normal = IntVector3(0, 0, 1);
							return result;
						}
					}
				} else {
					while (iz < nextZ) {
						++iz;
						if (iz >= depth) return result;
						if (map & (1ULL << iz)) {
							result.hit = true;
							result.startSolid = false;
							result.hitBlock = IntVector3
							(ix, iy, iz);
							fx -= fabsf(dir.x) * (iz - zz) * invDirZ;
							fy -= fabsf(dir.y) * (iz - zz) * invDirZ;
							result.hitPos = getCurrentPos();
							result.hitPos.z = iz;
							result.hitPos += orig;
							result.normal = IntVector3(0, 0, -1);
							return result;
						}
					}
				}
			}
			
			// move to next (x, y)
			ix = nextX; iy = nextY; iz = nextZ;
			fx = newFx; fy = newFy; zz = nextZf;
			if (ix >= 0 && iy >= 0 && ix < width && iy < height
				&& iz >= 0 && iz < depth) {
				auto map = GetSolidBitsAt(ix, iy);
				if (map & (1ULL << iz)) {
					result.hit = true;
					result.startSolid = false;
					result.hitBlock = IntVector3
					(ix, iy, iz);
					result.hitPos = getCurrentPos();
					result.hitPos += orig;
					result.normal = IntVector3
					(side == 0 ? -signX : 0,
					 side != 0 ? -signY : 0, 0);
					return result;
				}
			}
			
		}
		
		
		return result;
	}
		
}
