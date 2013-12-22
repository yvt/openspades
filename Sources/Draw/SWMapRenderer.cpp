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

#include "SWMapRenderer.h"
#include <Client/GameMap.h>
#include <Core/Bitmap.h>
#include <array>
#include "SWRenderer.h"
#include <Core/MiniHeap.h>
#include <Core/Settings.h>
#include <Core/ConcurrentDispatch.h>
#include <Core/Stopwatch.h>

SPADES_SETTING(r_swUndersampling, "0");
SPADES_SETTING(r_swNumThreads, "4");

namespace spades {
	namespace draw {
		
		
		
#if ENABLE_SSE // assume SSE availability (no checks!)
		static inline float fastDiv(float a, float b) {
			union {
				float tmp;
				__m128 mtmp;
			};
			tmp = b;
			mtmp = _mm_rcp_ss(mtmp);
			return a * tmp;
		}
		static inline float fastRcp(float b) {
			union {
				float tmp;
				__m128 mtmp;
			};
			tmp = b;
			mtmp = _mm_rcp_ss(mtmp);
			return tmp;
		}
		static inline float fastRSqrt(float v) {
			union {
				float tmp;
				__m128 mtmp;
			};
			tmp = v;
			mtmp = _mm_rsqrt_ss(mtmp);
			return tmp;
		}
#else
		static inline float fastDiv(float a, float b) {
			return a / b;
		}
		static inline float fastRcp(float b) {
			return 1.f / b;
		}
		static inline float fastRSqrt(float b) {
			return 1.f / sqrtf(b);
		}
#endif
		// special tan function whose value is finite.
		static inline float SpecialTan(float v) {
			static const float pi = M_PI;
			if(v <= -pi * 0.5f) {
				return -2.f;
			}else if(v < -pi * 0.25f) {
				v = -2.f - 1.f / tanf(v);
			}else if(v < pi * 0.25f) {
				v = tanf(v);
			}else if(v < pi * 0.5f){
				v = 2.f - 1.f / tanf(v);
			}else{
				return v = 2.f;
			}
			return v;
		}
		// convert from tan value to special tan value.
		static inline float ToSpecialTan(float v) {
			if(v < -1.f)
				return -2.f - fastRcp(v);
			else if(v > 1.f)
				return 2.f - fastRcp(v);
			else
				return v;
		}
		
	
		enum class Face: short {
			PosX, NegX,
			PosY, NegY,
			PosZ, NegZ
		};
		
		struct SWMapRenderer::LinePixel {
			union {
				struct {
					uint32_t combined;
				};
				struct {
					unsigned int color: 24;
					//Face face: 7;
					bool filled: 1;
				};
			};
			
			// using "operator =" makes this struct non-POD
			void Set(const LinePixel& p){
				combined = p.combined;
			}
		
			inline void Clear() {
				combined = 0;
			}
			
			inline bool IsEmpty() const {
				return combined == 0;
			}
		};
		
		// infinite length line from -z to +z
		struct SWMapRenderer::Line {
			std::vector<LinePixel> pixels;
			Vector3 horizonDir;
			float pitchTanMin;
			float pitchScale;
			int pitchTanMinI;
			int pitchScaleI;
		};
		

		SWMapRenderer::SWMapRenderer(SWRenderer *r,
									 client::GameMap *m,
									 SWFeatureLevel level):
		map(m),
		frameBuf(nullptr),
		depthBuf(nullptr),
		rleHeap(m->Width() * m->Height() * 16),
		level(level),
		w(m->Width()), h(m->Height()),
		renderer(r){
			rle.resize(w * h);
			rleLen.resize(w * h);
			
			Stopwatch sw;
			sw.Reset();
			SPLog("Building RLE map...");
			
			int idx = 0;
			for(int y = 0; y < h; y++)
				for(int x = 0; x < w; x++) {
					BuildRle(x, y, rleBuf);
					
					auto ref = rleHeap.Alloc(rleBuf.size() * sizeof(RleData));
					short *ptr = rleHeap.Dereference<short>(ref);
					std::memcpy(ptr, rleBuf.data(), rleBuf.size() * sizeof(RleData));
					
					rle[idx] = ref;
					rleLen[idx] = rleBuf.size() * sizeof(RleData);
					
					idx++;
				}
			SPLog("RLE map created in %.6f seconds", sw.GetTime());
		}
		
		SWMapRenderer::~SWMapRenderer() {
			
		}
		
		void SWMapRenderer::BuildRle(int x, int y, std::vector<RleData> &out) {
			out.clear();
			
			out.push_back(0); // [0] = +Z face position address
			out.push_back(0); // [0] = +X face position address
			out.push_back(0); // [0] = -X face position address
			out.push_back(0); // [0] = +Y face position address
			out.push_back(0); // [0] = -Y face position address
			
			uint64_t smap = map->GetSolidMapWrapped(x, y);
			std::array<uint64_t, 4> adjs =
			{map->GetSolidMapWrapped(x+1, y),
			 map->GetSolidMapWrapped(x-1, y),
			 map->GetSolidMapWrapped(x, y+1),
			 map->GetSolidMapWrapped(x, y-1)};
			bool old = false;
			
			for(int z = 0; z < 64; z++) {
				bool b = (smap >> z) & 1;
				if(b && !old) {
					out.push_back(static_cast<RleData>(z));
				}
				old = b;
			}
			out.push_back(-1);
			
			out[0] = static_cast<RleData>(out.size());
			
			old = true;
			for(int z = 63; z >= 0; z--) {
				bool b = (smap >> z) & 1;
				if(b && !old) {
					out.push_back(static_cast<RleData>(z));
				}
				old = b;
			}
			out.push_back(-1);
			
			for(int k = 0; k < 4; k++) {
				out[k + 1] = static_cast<RleData>(out.size());
				for(int z = 0; z < 64; z++) {
					if((smap >> z) & 1){
						if(!((adjs[k] >> z) & 1)){
							out.push_back(static_cast<RleData>(z));
						}
					}
				}
				out.push_back(-1);
			}
			
		}
		
		void SWMapRenderer::UpdateRle(int x, int y) {
			int idx = x + y * w;
			BuildRle(x, y, rleBuf);
			
			rleHeap.Free(rle[idx], rleLen[idx]);
			
			auto ref = rleHeap.Alloc(rleBuf.size() * sizeof(RleData));
			short *ptr = rleHeap.Dereference<short>(ref);
			std::memcpy(ptr, rleBuf.data(), rleBuf.size() * sizeof(RleData));
			
			rle[idx] = ref;
			rleLen[idx] = rleBuf.size() * sizeof(RleData);
		}
		
		
		
		template<SWFeatureLevel flevel>
		void SWMapRenderer::BuildLine(Line& line,
									  float minPitch, float maxPitch) {
			
			// hard code for further optimization
			enum {
				w = 512, h = 512
			};
			SPAssert(map->Width() == 512);
			SPAssert(map->Height() == 512);
			
			const auto *rle = this->rle.data();
			auto& rleHeap = this->rleHeap;
			client::GameMap *map = this->map;
			
			// pitch culling
			{
				const auto& frustrum = renderer->frustrum;
				static const float pi = M_PI;
				const auto& horz = line.horizonDir;
				minPitch = -pi * 0.4999f;
				maxPitch = pi * 0.4999f;
				
				auto cull = [&minPitch, &maxPitch]() {
					minPitch = 2.f;
					maxPitch = -2.f;
				};
				auto clip = [&minPitch, &maxPitch, &horz, &cull](Vector3 plane) {
					if(plane.x == 0.f && plane.y == 0.f) {
						if(plane.z > 0.f) {
							minPitch = std::max(minPitch, 0.f);
						}else{
							maxPitch = std::min(maxPitch, 0.f);
						}
					}else if(plane.z == 0.f){
						if(Vector3::Dot(plane, horz) < 0.f) {
							cull();
						}
					}else{
						Vector3 prj = plane; prj.z = 0.f;
						prj = prj.Normalize();
						
						float zv = fabsf(plane.z);
						float cs = Vector3::Dot(prj, horz);
						
						float ang = zv * zv * (1.f - cs * cs) / (cs * cs);
						ang = -cs * sqrtf(1.f + ang);
						ang = zv / ang;
						
						// convert to tan
						ang = sqrtf(1.f - ang * ang) / ang;
						
						// convert to angle
						ang = atanf(ang);
						
						if(plane.z > 0.f) {
							minPitch = std::max(minPitch, ang - 0.01f);
						}else{
							maxPitch = std::min(maxPitch, -ang + 0.01f);
						}
					}
				};
				
				clip(frustrum[2].n);
				clip(frustrum[3].n);
				clip(frustrum[4].n);
				clip(frustrum[5].n);
				
			}
			
			float minTan = SpecialTan(minPitch);
			float maxTan = SpecialTan(maxPitch);
			
			line.pitchTanMin = minTan;
			line.pitchScale = lineResolution / (maxTan - minTan);
			line.pitchTanMinI = static_cast<int>(minTan * 65536.f);
			line.pitchScaleI = static_cast<int>(line.pitchScale * 65536.f);
			
			// TODO: pitch culling
			
			// ray direction
			float dirX = line.horizonDir.x;
			float dirY = line.horizonDir.y;
			if(dirY == 0.f) dirY = 1.e-20f;
			if(dirX == 0.f) dirX = 1.e-20f;
			float invDirX = 1.f / dirX;
			float invDirY = 1.f / dirY;
			int signX = dirX > 0.f ? 1 : -1;
			int signY = dirY > 0.f ? 1 : -1;
			
			// camera position
			float cx = sceneDef.viewOrigin.x;
			float cy = sceneDef.viewOrigin.y;
			float cz = sceneDef.viewOrigin.z;
			
			int icz = static_cast<int>(floorf(cz));
			
			// ray position
			float rx = cx, ry = cy;
			
			// ray position in integer
			int irx = static_cast<int>(floorf(rx));
			int iry = static_cast<int>(floorf(ry));
			
			float fogDist = 128.f;
			float distance = 1.e-40f; // traveled path
			
			auto& pixels = line.pixels;
			
			pixels.resize(lineResolution);
			
			const float transScale = static_cast<float>(pixels.size()) / (maxTan - minTan);
			const float transOffset = -minTan * transScale;
			
			for(size_t i = 0; i < pixels.size(); i++)
				pixels[i].Clear();
			
			// if culled out, bail out now (pixels are filled)
			if(minPitch >= maxPitch)
				return;
			
			std::array<float, 65> zval; // precompute (z - cz) * some
			for(size_t i = 0; i < zval.size(); i++)
				zval[i] = (static_cast<float>(i) - cz);
			
			float vmax = lineResolution + 0.5f;
			auto transform = [&zval, &transOffset, vmax, &transScale](float invDist, int z) {
				float p = ToSpecialTan(invDist * zval[z]) * transScale + transOffset;
				p = std::max(p, 0.f);
				p = std::min(p, vmax);
				return static_cast<int>(p);
			};
			
			uint32_t lastSolidMap1;
			uint32_t lastSolidMap2;
			{
				auto lastSolidMap = map->GetSolidMapWrapped(irx, iry);
				lastSolidMap1 = static_cast<uint32_t>(lastSolidMap);
				lastSolidMap2 = static_cast<uint32_t>(lastSolidMap >> 32);
			}
			
			int count = 1;
			
			while(distance < fogDist) {
				
				int nextIRX = irx + signX;
				int nextIRY = iry + signY;
				
				float oldDistance = distance;
				
				float timeToNextX =
				(signX > 0 ? (nextIRX - rx) :
				 (irx - rx)) * invDirX;
				
				float timeToNextY =
				(signY > 0 ? (nextIRY - ry) :
				 (iry - ry)) * invDirY;
				
				int oirx = irx, oiry = iry;
				Face wallFace;
				
				if(timeToNextX < timeToNextY) {
					// go across x plane
					irx += signX;
					rx = signX > 0 ? irx : (irx + 1);
					ry += dirY * timeToNextX;
					distance += timeToNextX;
					wallFace = signX > 0 ? Face::NegX : Face::PosX;
				}else{
					// go across y plane
					iry += signY;
					rx += dirX * timeToNextY;
					ry = signY > 0 ? iry : (iry + 1);
					distance += timeToNextY;
					wallFace = signY > 0 ? Face::NegY : Face::PosY;
				}
				
				// finalize active spans
				float invDist;// = 1.f / distance;
				float oldInvDist;
				
#if ENABLE_SSE
				{
					union {
						struct{
							float v, v2;
						};
						__m128 m;
					};
					m = _mm_rcp_ps(_mm_setr_ps(distance, oldDistance, 0.f, 0.f));
					invDist = v;
					oldInvDist = v2;
				}
#else
				invDist = 1.f / distance;
				oldInvDist = 1.f / oldDistance;
#endif
				
				
				// check for new spans
				uint32_t solidMap1, solidMap2;
				{
					auto solidMap = map->GetSolidMapWrapped(irx, iry);
					solidMap1 = static_cast<uint32_t>(solidMap);
					solidMap2 = static_cast<uint32_t>(solidMap >> 32);
				}
				auto solidMapDiff1 = solidMap1 & ~lastSolidMap1;
				auto solidMapDiff2 = solidMap2 & ~lastSolidMap2;
				
				
				auto BuildLinePixel = [map](int x, int y, int z,
											Face face) {
					LinePixel px;
#if ENABLE_SSE
					if(flevel == SWFeatureLevel::SSE2) {
						__m128i m;
						uint32_t col = map->GetColorWrapped(x, y, z);
						m = _mm_setr_epi32(col, 0,0,0);
						m = _mm_unpacklo_epi8(m, _mm_setzero_si128());
						m = _mm_shufflelo_epi16(m, 0xc6);
						
						switch(face){
							case Face::PosZ:
								m = _mm_srli_epi16(m, 1);
								break;
							case Face::PosX:
							case Face::PosY:
							case Face::NegX:
								m = _mm_adds_epi16
								(_mm_srli_epi16(m, 1), _mm_srli_epi16(m, 2));
								break;
							default:
								break;
						}
						if((col>>24)<100) {
							m = _mm_srli_epi16(m, 1);
						}
						m = _mm_packus_epi16(m, m);
						_mm_store_ss(reinterpret_cast<float *>(&px.combined),
										 _mm_castsi128_ps(m));
						px.filled = true;
					}else
#endif
						// non-optimized
					{
						uint32_t col;
						col = map->GetColorWrapped(x, y, z);
						col = (col & 0xff00) | ((col & 0xff) << 16) | ((col & 0xff0000) >> 16);
						switch(face){
							case Face::PosZ:
								col = (col & 0xfcfcfc) >> 2;
								break;
							case Face::PosX:
							case Face::PosY:
							case Face::NegX:
								col = (col & 0xfefefe) >> 1;
								break;
							default:
								break;
						}
						px.combined = col;
						px.filled = true;
					}
					return px;
				};
				
				// floor/ceiling
				{
					
					// linear code
					auto addFloor = [&](int vx, int vy, int vz) {
						int p1 = transform(invDist, vz);
						int p2 = transform(oldInvDist, vz);
						LinePixel pix = BuildLinePixel(vx, vy, vz, Face::NegZ);
						
						for(int j = p1; j < p2; j++) {
							auto& p = pixels[j];
							if(!p.IsEmpty()) continue;
							p.Set(pix);
						}
					};
					auto addCeiling = [&](int vx, int vy, int vz) {
						int p1 = transform(invDist, vz + 1);
						int p2 = transform(oldInvDist, vz + 1);
						LinePixel pix = BuildLinePixel(vx, vy, vz, Face::PosZ);
						
						for(int j = p2; j < p1; j++) {
							auto& p = pixels[j];
							if(!p.IsEmpty()) continue;
							p.Set(pix);
						}
					};
					
					// RLE scan
					auto ref = rle[(oirx & w-1) + ((oiry & h-1) * w)];
					RleData *rle = rleHeap.Dereference<RleData>(ref);
					{
						RleData *ptr = rle + 5;
						while(*ptr != -1) {
							int z = *ptr;
							if(z > icz) {
								addFloor(oirx, oiry, z);
							}
							ptr++;
						}
					}
					{
						RleData *ptr = rle + rle[0];
						while(*ptr != -1) {
							int z = *ptr;
							if(z < icz) {
								addCeiling(oirx, oiry, z);
							}
							ptr++;
						}
					}
					
				} // done: floor/ceiling
				
				// add walls
				if(false){
					// brute-force
					auto checkRange = [&](uint32_t diff, bool upper,
										   int begin, int end) {
						int shift = upper ? 32 : 0;
						for(int i = begin; i < end; i++) {
							
							if(diff & (1UL << i)) {
								int p1 = transform(invDist, i + shift);
								int p2 = transform(invDist, i + 1 + shift);
								
								LinePixel pix = BuildLinePixel(irx, iry, i+shift, wallFace);
								
								for(int j = p1; j < p2; j++) {
									auto& p = pixels[j];
									if(!p.IsEmpty()) continue;
									p.Set(pix);
								}
							} /* end if(solidMapDiff & (1ULL << i)) */
						}
					};
					
					auto recurse = [&](uint32_t diff, bool upper) {
						if(diff == 0) {
							return;
						}
						
						if(diff & 0xff) {
							checkRange(diff, upper, 0, 8);
						}
						if(diff & 0xff00) {
							checkRange(diff, upper, 8, 16);
						}
						if(diff & 0xff0000) {
							checkRange(diff, upper, 16, 24);
						}
						if(diff & 0xff000000) {
							checkRange(diff, upper, 24, 32);
						}
					};
					
					recurse(solidMapDiff1, false);
					recurse(solidMapDiff2, true);
				}else{
					// by RLE map
					auto ref = rle[(irx & w-1) + ((iry & h-1) * w)];
					RleData *rle = rleHeap.Dereference<RleData>(ref);
					rle += rle[1 + static_cast<int>(wallFace)];
					
					while(*rle != -1) {
						int z = *(rle++);
						
						int p1 = transform(invDist, z);
						int p2 = transform(invDist, z + 1);
						
						LinePixel pix = BuildLinePixel(irx, iry, z, wallFace);
						
						for(int j = p1; j < p2; j++) {
							auto& p = pixels[j];
							if(!p.IsEmpty()) continue;
							p.Set(pix);
						}
					}
					
				} // add wall - end
				
				
				lastSolidMap1 = solidMap1;
				lastSolidMap2 = solidMap2;
				
				
				// check pitch cull
				if((--count) == 0){
					if((transform(invDist, 0) >= lineResolution - 1 && icz >= 0) ||
					   transform(invDist, 63) <= 0)
						break;
					count = 2;
				}
				
				// let's go to next voxel!
			}
		}

		
		struct AtanTable {
			std::array<uint16_t, 5000> sm;
			std::array<uint16_t, 5000> lg;
			std::array<uint16_t, 5000> smN;
			std::array<uint16_t, 5000> lgN;
			
			// [0, 2pi] -> [0, 65536]
			static uint16_t ToFixed(float v) {
				v /= (M_PI * 2.f);
				v *= 65536.f;
				int i = static_cast<int>(v);
				return static_cast<uint16_t>(i & 65535);
			}
			
			AtanTable() {
				for(int i = 0; i < 5000; i++) {
					sm[i] = ToFixed(atanf(i / 4096.f));
					lg[i] = ToFixed(atanf(1.f / ((i + .5f) / 4096.f)));
					smN[i] = ToFixed(-atanf(i / 4096.f));
					lgN[i] = ToFixed(-atanf(1.f / ((i + .5f) / 4096.f)));
				}
			}
		};
		static AtanTable atanTable;
		static inline uint16_t fastATan(float v){
			if(v < 0.f) {
				if(v > -1.f) {
					v *= -4096.f;
					int idx = static_cast<int>(v);
					//v -= idx;
					auto ret = atanTable.smN[idx];
					return ret;
				}else{
					v = fastDiv(-4096.f, v);
					int idx = static_cast<int>(v);
					//v -= idx;
					auto ret = atanTable.lgN[idx];
					return ret;
				}
			}else{
				if(v < 1.f) {
					v *= 4096.f;
					int idx = static_cast<int>(v);
					//v -= idx;
					auto ret = atanTable.sm[idx];
					return ret;
					//ret += (atanTable.sm[idx + 1] - ret) * v;
					//return ret;
				}else{
					v = fastDiv(4096.f, v);
					int idx = static_cast<int>(v);
					//v -= idx;
					auto ret = atanTable.lg[idx];
					return ret;
					//ret += (atanTable.lg[idx + 1] - ret) * v;
					//return ret;
				}
			}
		}
		
		static inline uint16_t fastATan2(float y, float x) {
			if(x == 0.f) {
				return y > 0.f ? 16384 : -16384;
				//y > 0.f ? (pi * 0.5f) : (-pi * 0.5f);
			}else if(x > 0.f) {
				return fastATan(fastDiv(y, x));
			}else{
				return fastATan(fastDiv(y, x)) + 32768;
			}
		}
		
		template<SWFeatureLevel flevel, int under>
		void SWMapRenderer::RenderFinal(float yawMin, float yawMax,
										unsigned int numLines,
										unsigned int threadId,
										unsigned int numThreads) {
			float fovX = tanf(sceneDef.fovX * 0.5f);
			float fovY = tanf(sceneDef.fovY * 0.5f);
			Vector3 front = sceneDef.viewAxis[2];
			Vector3 right = sceneDef.viewAxis[0];
			Vector3 down = sceneDef.viewAxis[1];
			
			unsigned int fw = frameBuf->GetWidth();
			unsigned int fh = frameBuf->GetHeight();
			uint32_t *fb = frameBuf->GetPixels();
			Vector3 v1 = front - right * fovX + down * fovY;
			Vector3 deltaDown = -down * (fovY * 2.f / static_cast<float>(fh));
			Vector3 deltaRight = right * (fovX * 2.f / static_cast<float>(fw) * under);
			
			static const float pi = M_PI;
			float yawScale = 65536.f / (pi * 2.f);
			int yawScale2 = static_cast<int>(pi * 2.f / (yawMax - yawMin) * 65536.f);
			int yawMin2 = static_cast<int>(yawMin * yawScale);
			auto& lineList = this->lines;
			
			enum {
				blockSize = 8,
				hBlock = blockSize / under
			};
			
			Vector3 deltaDownLarge = deltaDown * blockSize;
			Vector3 deltaRightLarge = deltaRight * hBlock;
			
			unsigned int startX = threadId * fw / numThreads;
			unsigned int endX = (threadId + 1) * fw / numThreads;
			
			startX = (startX / blockSize) * blockSize;
			endX = (endX / blockSize) * blockSize;
			
			v1 += deltaRight * static_cast<float>(startX / under);
			
			for(unsigned int fx = startX; fx < endX; fx+=blockSize){
				Vector3 v2 = v1;
				for(unsigned int fy = 0; fy < fh; fy+=blockSize){
					
					if(v2.z > 0.96f || v2.z < -0.96f) {
						// near to pole. cannot be approximated by piecewise
						goto SlowBlockPath;
					}
					
				FastBlockPath:
					{
						
						// Use bi-linear interpolation for faster yaw/pitch
						// computation.
						
						auto calcYawindex = [yawScale2, numLines, yawMin2](Vector3 v) {
							int yawIndex;
							{
								float x = v.x, y = v.y;
								int yaw;
								yaw = fastATan2(y, x);
								yaw -= yawMin2;
								yawIndex = static_cast<int>
								(yaw & 0xffff);
							}
							yawIndex <<= 8;
							return yawIndex;
						};
						auto calcPitch = [] (Vector3 vv) {
							float pitch;
							pitch = vv.z * fastRSqrt(vv.x*vv.x+vv.y*vv.y);
							pitch = ToSpecialTan(pitch);
							return static_cast<int>(pitch * (65536.f * 8192.f));
						};
						int yawIndex1 = calcYawindex(v2);
						int pitch1 = calcPitch(v2);
						int yawIndex2 = calcYawindex(v2 + deltaRightLarge);
						int pitch2 = calcPitch(v2 + deltaRightLarge);
						int yawIndex3 = calcYawindex(v2 + deltaDownLarge);
						int pitch3 = calcPitch(v2 + deltaDownLarge);
						int yawIndex4 = calcYawindex(v2 + deltaRightLarge + deltaDownLarge);
						int pitch4 = calcPitch(v2 + deltaRightLarge + deltaDownLarge);
						
						// note: `<<8>>8` is phase unwrapping
						int yawDiff1 = ((yawIndex2 - yawIndex1)<<8>>8) / hBlock;
						int yawDiff2 = ((yawIndex4 - yawIndex3)<<8>>8) / hBlock;
						int pitchDiff1 = (pitch2 - pitch1) / hBlock;
						int pitchDiff2 = (pitch4 - pitch3) / hBlock;
						
						int yawIndexA = yawIndex1;
						int yawIndexB = yawIndex3;
						int pitchA = pitch1;
						int pitchB = pitch3;
						
						uint32_t *fb2 = fb + fx + fy * fw;
						for(unsigned int x = 0; x < blockSize; x+=under) {
							uint32_t *fb3 = fb2 + x;
							int yawIndexC = yawIndexA;
							int yawDelta = ((yawIndexB - yawIndexA)<<8>>8) / blockSize;
							int pitchC = pitchA;
							int pitchDelta = (pitchB - pitchA) / blockSize;
							
							for(unsigned int y = 0; y < blockSize; y++) {
								unsigned int yawIndex = static_cast<unsigned int>(yawIndexC<<8>>16);
								yawIndex = (yawIndex * yawScale2) >> 16;
								yawIndex = (yawIndex * numLines) >> 16;
								auto& line = lineList[yawIndex];
								
								// solve pitch
								int pitchIndex;
								
								{
									pitchIndex = pitchC >> 13;
									pitchIndex -= line.pitchTanMinI;
									pitchIndex = static_cast<int>
									((static_cast<int64_t>(pitchIndex) *
									  static_cast<int64_t>(line.pitchScaleI)) >> 32);
									//pitch = (pitch - line.pitchTanMin) * line.pitchScale;
									//pitchIndex = static_cast<int>(pitch);
									pitchIndex = std::max(pitchIndex, 0);
									pitchIndex = std::min(pitchIndex, lineResolution - 1);
								}
								
								auto& pix = line.pixels[pitchIndex];
								
								// write color.
								// NOTE: combined contains both color and other information,
								// though this isn't a problem as long as the color comes
								// in the LSB's
#if ENABLE_SSE
								if(flevel == SWFeatureLevel::SSE2) {
									__m128i m;
									
									if(under == 1) {
										_mm_stream_si32(reinterpret_cast<int *>(fb3),
														static_cast<int>(pix.combined));
									}else if(under == 2){
										m = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float *>(&pix)));
										m = _mm_shuffle_epi32(m, 0x00);
										_mm_store_sd(reinterpret_cast<double *>(fb3),
													 _mm_castsi128_pd(m));
									}else if(under == 4){
										m = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float *>(&pix)));
										m = _mm_shuffle_epi32(m, 0x00);
										_mm_stream_si128(reinterpret_cast<__m128i *>(fb3),
														 (m));
									}
									
								}else
#endif
									// non-optimized
								{
									uint32_t col = pix.combined;
									
									for(int k = 0; k < under; k++){
										fb3[k] = col;
									}
								}
								
								
								fb3 += fw;
								
								yawIndexC += yawDelta;
								pitchC += pitchDelta;
							}
							
							yawIndexA += yawDiff1;
							yawIndexB += yawDiff2;
							pitchA += pitchDiff1;
							pitchB += pitchDiff2;
						}
						
					}
					goto Converge;
					
				SlowBlockPath:
					{
						Vector3 v3 = v2;
						uint32_t *fb2 = fb + fx + fy * fw;
						for(unsigned int x = 0; x < blockSize; x+=under) {
							Vector3 v4 = v3;
							uint32_t *fb3 = fb2 + x;
							for(unsigned int y = 0; y < blockSize; y++) {
								Vector3 vv = v4;
								
								// solve yaw
								unsigned int yawIndex;
								{
									float x = vv.x, y = vv.y;
									int yaw;
									yaw = fastATan2(y, x);
									yaw -= yawMin2;
									yawIndex = static_cast<unsigned int>
									(yaw & 0xffff);
								}
								yawIndex = (yawIndex * yawScale2) >> 16;
								yawIndex = (yawIndex * numLines) >> 16;
								
								auto& line = lineList[yawIndex];
								
								// solve pitch
								int pitchIndex;
								
								{
									float pitch;
									pitch = vv.z * fastRSqrt(vv.x*vv.x+vv.y*vv.y);
									pitch = ToSpecialTan(pitch);
									pitch = (pitch - line.pitchTanMin) * line.pitchScale;
									pitchIndex = static_cast<int>(pitch);
									pitchIndex = std::max(pitchIndex, 0);
									pitchIndex = std::min(pitchIndex, lineResolution - 1);
								}
								
								auto& pix = line.pixels[pitchIndex];
								
								// write color.
								// NOTE: combined contains both color and other information,
								// though this isn't a problem as long as the color comes
								// in the LSB's
	#if ENABLE_SSE
								if(flevel == SWFeatureLevel::SSE2) {
									__m128i m;
									
									if(under == 1) {
										_mm_stream_si32(reinterpret_cast<int *>(fb3),
														static_cast<int>(pix.combined));
									}else if(under == 2){
										m = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float *>(&pix)));
										m = _mm_shuffle_epi32(m, 0x00);
										_mm_store_sd(reinterpret_cast<double *>(fb3),
													 _mm_castsi128_pd(m));
									}else if(under == 4){
										m = _mm_castps_si128(_mm_load_ss(reinterpret_cast<const float *>(&pix)));
										m = _mm_shuffle_epi32(m, 0x00);
										_mm_stream_si128(reinterpret_cast<__m128i *>(fb3),
													 (m));
									}
									
								}else
	#endif
									// non-optimized
								{
									uint32_t col = pix.combined;
									
									for(int k = 0; k < under; k++){
										fb3[k] = col;
									}
								}
								
								
								fb3 += fw;
								
								v4 += deltaDown;
							} // y
							v3 += deltaRight;
						} // x
						
					} // end SlowBlockPath
					
				Converge:
					
					v2 += deltaDownLarge;
				} // fy
				v1 += deltaRightLarge;
			} // fx
			
		}
		template <class F>
		static void InvokeParallel(F f, unsigned int numThreads) {
			SPAssert(numThreads <= 32);
			std::array<std::unique_ptr<ConcurrentDispatch>, 32> disp;
			for(auto i = 1U; i < numThreads; i++) {
				auto ff = [i, &f]() {
					f(i);
				};
				disp[i] = std::unique_ptr<ConcurrentDispatch>
				(static_cast<ConcurrentDispatch *>(new FunctionDispatch<decltype(ff)>(ff)));
				disp[i]->Start();
			}
			f(0);
			for(auto i = 1U; i < numThreads; i++) {
				disp[i]->Join();
			}
		}
		
		template<SWFeatureLevel flevel>
		void SWMapRenderer::RenderInner(const client::SceneDefinition &def,
								   Bitmap *frame, float *depthBuffer) {
			
			sceneDef = def;
			frameBuf = frame;
			depthBuf = depthBuffer;
			
			// calculate line density.
			float yawMin, yawMax;
			float pitchMin, pitchMax;
			size_t numLines;
			{
				float fovX = tanf(def.fovX * 0.5f);
				float fovY = tanf(def.fovY * 0.5f);
				float fovDiag = sqrtf(fovX * fovX + fovY * fovY);
				float fovDiagAng = atanf(fovDiag);
				float pitch = asinf(def.viewAxis[2].z);
				static const float pi = M_PI;
				
				//pitch = 0.f;
				
				if(fabsf(pitch) >= pi * 0.49f - fovDiagAng) {
					// pole is visible
					yawMin = 0.f;
					yawMax = pi * 2.f;
				}else{
					float yaw = atan2l(def.viewAxis[2].y, def.viewAxis[2].x);
					// TODO: incorrect!
					yawMin = yaw - pi * .5f; //fovDiagAng;
					yawMax = yaw + pi * .5f; //fovDiagAng;
				}
				
				pitchMin = pitch - fovDiagAng;
				pitchMax = pitch + fovDiagAng;
				if(pitchMin < -pi * 0.5f) {
					pitchMax = std::max(pitchMax, -pi-pitchMin);
					pitchMin = -pi * 0.5f;
				}
				if(pitchMax > pi * 0.5f) {
					pitchMin = std::min(pitchMin, pi - pitchMax);
					pitchMax = pi * 0.5f;
				}
				
				// pitch of PI/2 will make tan(x) infinite
				pitchMin = std::max(pitchMin, -pi * 0.4999f);
				pitchMax = std::min(pitchMax, pi * 0.4999f);
				
				float interval = static_cast<float>(frame->GetHeight());
				interval = fovY * 2.f / interval;
				lineResolution = static_cast<int>((pitchMax - pitchMin) / interval * 1.5f);
				lineResolution = frame->GetHeight();
				if(pitchMin > 0.f) {
					//interval /= cosf(pitchMin);
				}else if(pitchMax < 0.f){
					//interval /= cosf(pitchMax);
				}
				
				numLines = static_cast<size_t>((yawMax - yawMin) / interval);
				
				int under = r_swUndersampling;
				under = std::max(std::min(under, 4), 1);
				numLines /= under;
				
				if(numLines < 8) numLines = 8;
				if(numLines > 65536) {
					SPRaise("Too many lines emit: %d", static_cast<int>(numLines));
				}
				lines.resize(std::max(numLines, lines.size()));
				
				SPLog("numlines: %d, each %f deg, and %d res",
					  static_cast<int>(numLines),
					  interval * 180.f / pi,
					  static_cast<int>(lineResolution));
			}
			
			// calculate vector for each lines
			{
				float scl = (yawMax - yawMin) / numLines;
				Vector3 horiz = Vector3::Make(cosf(yawMin), sinf(yawMin), 0.f);
				float c = cosf(scl);
				float s = sinf(scl);
				for(size_t i = 0; i < numLines; i++) {
					Line& l = lines[i];
					l.horizonDir = horiz;
					
					float x = horiz.x * c - horiz.y * s;
					float y = horiz.x * s + horiz.y * c;
					horiz.x = x;
					horiz.y = y;
				}
			}
			
			unsigned int numThreads = static_cast<unsigned int>((int)r_swNumThreads);
			numThreads = std::max(numThreads, 1U);
			numThreads = std::min(numThreads, 32U);
			
			{
				unsigned int nlines = static_cast<unsigned int>(numLines);
				InvokeParallel([&](unsigned int th) {
					unsigned int start = th * nlines / numThreads;
					unsigned int end = (th+1) * nlines / numThreads;
					
					for(size_t i = start; i < end; i++) {
						BuildLine<flevel>(lines[i],  pitchMin, pitchMax);
					}
				}, numThreads);
			}
			
			int under = r_swUndersampling;
			
			InvokeParallel([&](unsigned int th) {
				
				if(under <= 1){
					RenderFinal<flevel, 1>(yawMin, yawMax,
										   static_cast<unsigned int>(numLines),
										   th, numThreads);
				}else if(under <= 2){
					RenderFinal<flevel, 2>(yawMin, yawMax,
										   static_cast<unsigned int>(numLines),
										   th, numThreads);
				}else{
					RenderFinal<flevel, 4>(yawMin, yawMax,
										   static_cast<unsigned int>(numLines),
										   th, numThreads);
				}
			}, numThreads);
			
			
			
			frameBuf = nullptr;
			depthBuf = nullptr;
		}
		
		void SWMapRenderer::Render(const client::SceneDefinition &def,
								   Bitmap *frame, float *depthBuffer) {
			if(!frame) SPInvalidArgument("frame");
			if(!depthBuffer) SPInvalidArgument("depthBuffer");
			
#if ENABLE_SSE2
			if(static_cast<int>(level) >= static_cast<int>(SWFeatureLevel::SSE2)) {
				RenderInner<SWFeatureLevel::SSE2>(def, frame, depthBuffer);
				return;
			}
#endif
			
			RenderInner<SWFeatureLevel::None>(def, frame, depthBuffer);
			
		}
	}
}
