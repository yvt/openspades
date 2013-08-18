//
//  VoxelModel.h
//  OpenSpades
//
//  Created by yvt on 7/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <stdint.h>
#include "../Core/Debug.h"
#include "IStream.h"
#include "Math.h"

namespace spades {
	class VoxelModel {
		Vector3 origin;
		int width, height, depth;
		uint64_t *solidBits;
		uint32_t *colors;
	public:
		VoxelModel(int width, int height, int depth);
		~VoxelModel();
		
		static VoxelModel *LoadKV6(IStream *);
		
		const uint64_t& GetSolidBitsAt(int x, int y) const {
			SPAssert(x >= 0); SPAssert(x < width);
			SPAssert(y >= 0); SPAssert(y < height);
			return solidBits[x + y * width];
		}
		uint64_t& GetSolidBitsAt(int x, int y) {
			SPAssert(x >= 0); SPAssert(x < width);
			SPAssert(y >= 0); SPAssert(y < height);
			return solidBits[x + y * width];
		}
		
		const uint32_t& GetColor(int x, int y, int z) const {
			SPAssert(x >= 0); SPAssert(x < width);
			SPAssert(y >= 0); SPAssert(y < height);
			SPAssert(z >= 0); SPAssert(z < depth);
			return colors[(x + y * width) * depth + z];
		}
		
		uint32_t& GetColor(int x, int y, int z) {
			SPAssert(x >= 0); SPAssert(x < width);
			SPAssert(y >= 0); SPAssert(y < height);
			SPAssert(z >= 0); SPAssert(z < depth);
			return colors[(x + y * width) * depth + z];
		}
		
		bool IsSolid(int x, int y, int z) const {
			if(z < 0 || z >= depth)
				return false;
			if(x < 0 || y < 0 || x >= width || y >= height)
				return false;
			uint64_t bits = GetSolidBitsAt(x, y);
			return (bits >> z) & 1;
		}
				
		void SetAir(int x, int y, int z) {
			SPAssert(z >= 0); SPAssert(z < depth);
			uint64_t mask = 1ULL << z;
			GetSolidBitsAt(x, y) &= ~mask;
		}
		
		void SetSolid(int x, int y, int z, uint32_t color) {
			uint64_t mask = 1ULL << z;
			GetSolidBitsAt(x, y) |= mask;
			GetColor(x, y, z) = color;
		}
		
		Vector3 GetOrigin() {
			return origin;
		}
		
		void SetOrigin(Vector3 v){
			origin = v;
		}
		
		int GetWidth() const { return width; }
		int GetHeight() const { return height; }
		int GetDepth() const { return depth; }
	};
}
