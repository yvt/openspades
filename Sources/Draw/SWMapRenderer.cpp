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

SPADES_SETTING(r_swUndersampling, "0");

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
					short vx;
					short vy;
					short vz;
					Face face;
				};
				struct {
					uint64_t align;
				};
			};
			
			// using "operator =" makes this struct non-POD
			void Set(const LinePixel& p){
				align = p.align;
			}
		
			inline void Clear() {
				vx = -1;
			}
			
			inline bool IsEmpty() const {
				return vx == -1;
			}
		};
		
		// infinite length line from -z to +z
		struct SWMapRenderer::Line {
			std::vector<LinePixel> pixels;
			Vector3 horizonDir;
			float pitchTanMin;
			float pitchScale;
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
		}
		
		SWMapRenderer::~SWMapRenderer() {
			
		}
		
		void SWMapRenderer::BuildRle(int x, int y, std::vector<RleData> &out) {
			out.clear();
			
			out.push_back(0); // [0] = +Z face position address
			
			uint64_t smap = map->GetSolidMapWrapped(x, y);
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
							minPitch = std::max(minPitch, ang);
						}else{
							maxPitch = std::min(maxPitch, -ang);
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
				
				// floor/ceiling
				{
					
					// linear code
					auto addFloor = [&](short vx, short vy, short vz) {
						int p1 = transform(invDist, vz);
						int p2 = transform(oldInvDist, vz);
						LinePixel pix;
						pix.vx = oirx; pix.vy = oiry; pix.vz = vz;
						pix.face = Face::NegZ;
						
						for(int j = p1; j < p2; j++) {
							auto& p = pixels[j];
							if(!p.IsEmpty()) continue;
							p.Set(pix);
						}
					};
					auto addCeiling = [&](short vx, short vy, short vz) {
						int p1 = transform(invDist, vz + 1);
						int p2 = transform(oldInvDist, vz + 1);
						LinePixel pix;
						pix.vx = oirx; pix.vy = oiry; pix.vz = vz;
						pix.face = Face::PosZ;
						
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
						RleData *ptr = rle + 1;
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
				{
					auto checkRange = [&](uint32_t diff, bool upper,
										   int begin, int end) {
						int shift = upper ? 32 : 0;
						for(int i = begin; i < end; i++) {
							
							if(diff & (1UL << i)) {
								int p1 = transform(invDist, i + shift);
								int p2 = transform(invDist, i + 1 + shift);
								
								LinePixel pix;
								pix.vx = irx; pix.vy = iry; pix.vz = i + shift;
								pix.face = wallFace;
								
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
				}
				
				
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
			std::array<float, 5000> sm;
			std::array<float, 5000> lg;
			std::array<float, 5000> smN;
			std::array<float, 5000> lgN;
			
			AtanTable() {
				for(int i = 0; i < 5000; i++) {
					sm[i] = atanf(i / 4096.f);
					lg[i] = atanf(1.f / ((i + .5f) / 4096.f));
					smN[i] = -atanf(i / 4096.f);
					lgN[i] = -atanf(1.f / ((i + .5f) / 4096.f));
				}
			}
		};
		static AtanTable atanTable;
		static inline float fastATan(float v){
			if(v < 0.f) {
				if(v > -1.f) {
					v *= -4096.f;
					int idx = static_cast<int>(v);
					//v -= idx;
					float ret = atanTable.smN[idx];
					return ret;
				}else{
					v = fastDiv(-4096.f, v);
					int idx = static_cast<int>(v);
					//v -= idx;
					float ret = atanTable.lgN[idx];
					return ret;
				}
			}else{
				if(v < 1.f) {
					v *= 4096.f;
					int idx = static_cast<int>(v);
					//v -= idx;
					float ret = atanTable.sm[idx];
					return ret;
					//ret += (atanTable.sm[idx + 1] - ret) * v;
					//return ret;
				}else{
					v = fastDiv(4096.f, v);
					int idx = static_cast<int>(v);
					//v -= idx;
					float ret = atanTable.lg[idx];
					return ret;
					//ret += (atanTable.lg[idx + 1] - ret) * v;
					//return ret;
				}
			}
		}
		
		static inline float fastATan2(float y, float x) {
			static const float pi = M_PI;
			if(x == 0.f) {
				return y > 0.f ? (pi * 0.5f) : (-pi * 0.5f);
			}else if(x > 0.f) {
				return fastATan(fastDiv(y, x));
			}else{
				return fastATan(fastDiv(y, x)) + pi;
			}
		}
		
		template<SWFeatureLevel flevel, int under>
		void SWMapRenderer::RenderFinal(float yawMin, float yawMax,
										unsigned int numLines) {
			float fovX = tanf(sceneDef.fovX * 0.5f);
			float fovY = tanf(sceneDef.fovY * 0.5f);
			Vector3 front = sceneDef.viewAxis[2];
			Vector3 right = sceneDef.viewAxis[0];
			Vector3 down = sceneDef.viewAxis[1];
			
			int fw = frameBuf->GetWidth();
			int fh = frameBuf->GetHeight();
			uint32_t *fb = frameBuf->GetPixels();
			Vector3 v1 = front - right * fovX + down * fovY;
			Vector3 deltaDown = -down * (fovY * 2.f / static_cast<float>(fh));
			Vector3 deltaRight = right * (fovX * 2.f / static_cast<float>(fw) * under);
			
			static const float pi = M_PI;
			float yawScale = 65536.f / (pi * 2.f);
			int yawScale2 = static_cast<int>(pi * 2.f / (yawMax - yawMin) * 65536.f);
			auto& lineList = this->lines;
			
			enum {
				blockSize = 16,
				hBlock = blockSize / under
			};
			
			Vector3 deltaDownLarge = deltaDown * blockSize;
			Vector3 deltaRightLarge = deltaRight * hBlock;
			
			for(int fx = 0; fx < fw; fx+=blockSize){
				Vector3 v2 = v1;
				for(int fy = 0; fy < fh; fy+=blockSize){
					Vector3 v3 = v2;
					uint32_t *fb2 = fb + fx + fy * fw;
					for(int x = 0; x < blockSize; x+=under) {
						Vector3 v4 = v3;
						uint32_t *fb3 = fb2 + x;
						for(int y = 0; y < blockSize; y++) {
							Vector3 vv = v4;
							
							// solve yaw
							unsigned int yawIndex;
							{
								float x = vv.x, y = vv.y;
								float yaw;
								yaw = fastATan2(y, x);
								yaw -= yawMin;
								yawIndex = static_cast<unsigned int>
								(static_cast<int>(yaw * yawScale) & 0xffff);
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
							
							// solve color
							uint32_t col;
							if(pix.IsEmpty()) {
								col = 0xff7f7f7f;
							}else{
								col = map->GetColorWrapped(pix.vx,pix.vy,pix.vz);
								col = (col & 0xff00) | ((col & 0xff) << 16) | ((col & 0xff0000) >> 16);
								switch(pix.face){
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
							}
							
							for(int k = 0; k < under; k++){
								fb3[k] = col;
							}
							
							fb3 += fw;
							
							v4 += deltaDown;
						} // y
						v3 += deltaRight;
					} // x
					v2 += deltaDownLarge;
				} // fy
				v1 += deltaRightLarge;
			} // fx
			
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
			
			//size_t spans = 0;
			for(size_t i = 0; i < numLines; i++) {
				BuildLine<flevel>(lines[i],  pitchMin, pitchMax);
			}
			
			
			int under = r_swUndersampling;
			if(under <= 1){
				RenderFinal<flevel, 1>(yawMin, yawMax,
									static_cast<unsigned int>(numLines));
			}else if(under <= 2){
				RenderFinal<flevel, 2>(yawMin, yawMax,
									   static_cast<unsigned int>(numLines));
			}else{
				RenderFinal<flevel, 4>(yawMin, yawMax,
									   static_cast<unsigned int>(numLines));
			}
			
			
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
