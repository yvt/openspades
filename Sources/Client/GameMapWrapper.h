//
//  GameMapWrapper.h
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <stdint.h>
#include <vector>

namespace spades {
	namespace client {
		class GameMap;
		
		struct CellPos {
			short x, y, z;
			CellPos(){}
			CellPos(int xx, int yy, int zz):
			x(xx), y(yy), z(zz){}
			
			bool operator < (const CellPos& p) const{
				if(x < p.x){
					return true;
				}else if(x > p.x){
					return false;
				}
				if(y < p.y){
					return true;
				}else if(y > p.y){
					return false;
				}
				return z < p.z;
			}
			bool operator ==(const CellPos& p) const{
				return x == p.x && y == p.y && z == p.z;
			}
			
		};
		
		/** Wraps GameMap and provides floating-block detection.*/
		class GameMapWrapper {
			friend class Client; // FIXME: for debug
		public:
			
		private:
			GameMap *map;
			
			/** Each element represents where this cell is connected from. */
			uint8_t *linkMap;
			
			enum LinkType {
				Invalid = 0, Root, 
				NegativeX, PositiveX,
				NegativeY, PositiveY,
				NegativeZ, PositiveZ
			};
			
			
			
			int width, height, depth;
			
			inline LinkType GetLink(int x, int y, int z){
				return (LinkType)linkMap[(x * height + y) * depth + z];
			}
			void SetLink(int x, int y, int z, LinkType l){
				linkMap[(x * height + y) * depth + z] = l;
			}
			
		public:
			GameMapWrapper(GameMap *);
			~GameMapWrapper();
			
			/** Addes a new block. */
			void AddBlock(int x, int y, int z,
						  uint32_t color);
			
			/** Removes the specified blocks, and returns floating blocks.
			 * This function, however, doesn't remove floating blocks. */
			std::vector<CellPos> RemoveBlocks(const std::vector<CellPos>&);
			
			void Rebuild();
		};
	}
}
