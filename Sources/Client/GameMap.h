/*
 Copyright (c) 2013 yvt
 based on code of pysnip (c) Mathias Kaerlev 2011-2012.
 
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

#include <stdint.h>
#include "../Core/Debug.h"
#include "../Core/Math.h"

#include "IGameMapListener.h"
#include <Core/RefCountedObject.h>
#include <list>
#include <Core/Mutex.h>
#include <Core/AutoLocker.h>
#include <functional>
#include <memory>

namespace spades{
	class IStream;
	namespace client {
		struct NGMapOptions {
			int quality;
			NGMapOptions():
			quality(50) {}
			void Validate() const;
		};
		
		class GameMap: public RefCountedObject {
		protected:
			~GameMap();
		public:
			// fixed for now
			enum {
				ChunkSizeBits = 5,
				ChunkSize = 1 << ChunkSizeBits,
				DefaultDepth = 64 // should be <= 64
			};
			struct Chunk {
				uint64_t solidMap[ChunkSize][ChunkSize];
				uint32_t colorMap[ChunkSize][ChunkSize][DefaultDepth];
			};
			GameMap(int width = 512, int height = 512, int depth = 64);
			
			/** Copy-on-write copy constructor. */
			GameMap(const GameMap&);
			
			static GameMap *Load(IStream *, int width = 512, int height = 512);
			static GameMap *LoadNGMap(IStream *,
									  std::function<void(float)> progressListener);
			static GameMap *LoadNGMap(IStream *);
			
			void Save(IStream *);
			void SaveNGMap(IStream *, const NGMapOptions& options);
			
			int Width() { return width; }
			int Height() { return height; }
			int Depth() { return DefaultDepth; }
			inline bool IsSolid(int x, int y, int z) {
				SPAssert(x >= 0); SPAssert(x < Width());
				SPAssert(y >= 0); SPAssert(y < Height());
				SPAssert(z >= 0); SPAssert(z < Depth());
				return ((GetSolidMapUnsafe(x, y) >> (uint64_t)z) & 1ULL) != 0;
			}
			
			/** @return 0xHHBBGGRR where HH is health (up to 100) */
			inline uint32_t GetColor(int x, int y, int z){
				SPAssert(x >= 0); SPAssert(x < Width());
				SPAssert(y >= 0); SPAssert(y < Height());
				SPAssert(z >= 0); SPAssert(z < Depth());
				auto& c = chunks[(x >> ChunkSizeBits) +
								 (y >> ChunkSizeBits) * chunkCols];
				return c->colorMap[x&(ChunkSize-1)][y&(ChunkSize-1)][z];
			}
			
			inline uint64_t GetSolidMapUnsafe(int x, int y) {
				auto& c = chunks[(x >> ChunkSizeBits) +
								 (y >> ChunkSizeBits) * chunkCols];
				
				return c->solidMap[x&(ChunkSize-1)][y&(ChunkSize-1)];
			}
			inline uint64_t GetSolidMapWrapped(int x, int y) {
				return GetSolidMapUnsafe(x & (Width() - 1), y & (Height() - 1));
			}
			
			inline void SetSolidMapUnsafe(int x, int y, uint64_t map) {
				SPAssert(x >= 0); SPAssert(x < Width());
				SPAssert(y >= 0); SPAssert(y < Height());
				auto& c = chunks[(x >> ChunkSizeBits) +
								 (y >> ChunkSizeBits) * chunkCols];
				x &= ChunkSize - 1;
				y &= ChunkSize - 1;
				
				if (!c.unique()) {
					// copy-on-write
					c.reset(new Chunk(*c));
				}
				
				c->solidMap[x][y] = map;
			}
			
			inline bool IsSolidWrapped(int x, int y, int z){
				if(z < 0)
					return false;
				if(z >= Depth())
					return true;
				return ((GetSolidMapWrapped(x, y) >> (uint64_t)z) & 1ULL) != 0;
			}
			
			inline uint32_t GetColorWrapped(int x, int y, int z){
				auto& c = chunks[((x & Width() - 1) >> ChunkSizeBits) +
								 ((y & Height() - 1) >> ChunkSizeBits) * chunkCols];
				return c->colorMap[x & (ChunkSize - 1)]
								  [y & (ChunkSize - 1)]
							      [z & (Depth() - 1)];
			}
			
			inline void Set(int x, int y, int z, bool solid, uint32_t color, bool unsafe = false){
				SPAssert(x >= 0); SPAssert(x < Width());
				SPAssert(y >= 0); SPAssert(y < Height());
				SPAssert(z >= 0); SPAssert(z < Depth());
				auto& c = chunks[(x >> ChunkSizeBits) +
								 (y >> ChunkSizeBits) * chunkCols];
				
				int cx = x & ChunkSize - 1;
				int cy = y & ChunkSize - 1;
				
				if (!c.unique()) {
					// copy-on-write
					c.reset(new Chunk(*c));
				}
				
				uint64_t mask = 1ULL << z;
				uint64_t value = c->solidMap[cx][cy];
				bool changed = false;
				if((value & mask) != (solid ? mask : 0ULL)){
					changed = true;
					value &= ~mask;
					if(solid)
						value |= mask;
					c->solidMap[cx][cy] = value;
				}
				if(solid){
					if(color != c->colorMap[cx][cy][z]){
						changed = true;
						c->colorMap[cx][cy][z] = color;
					}
				}
				if(!unsafe) {
					if(changed){
						if(listener)
							listener->GameMapChanged(x, y, z, this);
						{
							AutoLocker guard(&listenersMutex);
							for(auto*l:listeners) {
								l->GameMapChanged(x, y, z, this);
							}
						}
					}
				}
			}
			
			void SetListener(IGameMapListener *l) {
				listener = l;
			}
			IGameMapListener *GetListener() {
				return listener;
			}
			void AddListener(IGameMapListener *);
			void RemoveListener(IGameMapListener *);
			
			bool ClipBox(int x, int y, int z);
			bool ClipWorld(int x, int y, int z);
			
			bool ClipBox(float x, float y, float z);
			bool ClipWorld(float x, float y, float z);
			
			// vanila compat
			bool CastRay(Vector3 v0, Vector3 v1,
						 float length, IntVector3& vOut);
			
			// accurate and slow ray casting
			struct RayCastResult {
				bool hit;
				bool startSolid;
				Vector3 hitPos;
				IntVector3 hitBlock;
				IntVector3 normal;
			};
			RayCastResult CastRay2(Vector3 v0, Vector3 dir,
								   int maxSteps);
		private:
			std::vector<std::shared_ptr<Chunk>> chunks;
			IGameMapListener *listener;
			std::list<IGameMapListener *> listeners;
			Mutex listenersMutex;
			
			int width, height;
			int chunkCols, chunkRows;
			
			bool IsSurface(int x, int y, int z);
			
			// helper for ngmap encoder/decoder
			void ComputeNeedsColor(int x, int y, int z, uint8_t needscolor[8][8]);
			
			class NGMapDecoder;
			struct ColorBlockEmitter;
		};
	}
}
