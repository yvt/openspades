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

#include "GLOptimizedVoxelModel.h"
#include "GLRenderer.h"
#include "GLImage.h"
#include "GLProgram.h"
#include "GLProgramAttribute.h"
#include "GLProgramUniform.h"
#include "../Core/Debug.h"
#include "GLShadowShader.h"
#include "GLDynamicLightShader.h"
#include "IGLShadowMapRenderer.h"
#include "GLShadowMapShader.h"
#include "../poly2tri/poly2tri.h"
#include "../Core/Exception.h"
#include <set>
#include "../Core/Bitmap.h"
#include "../Core/BitmapAtlasGenerator.h"

namespace spades {
	namespace draw {
		void GLOptimizedVoxelModel::PreloadShaders(spades::draw::GLRenderer *renderer) {
			renderer->RegisterProgram("Shaders/OptimizedVoxelModel.program");
			renderer->RegisterProgram("Shaders/OptimizedVoxelModelDynamicLit.program");
			renderer->RegisterProgram("Shaders/OptimizedVoxelModelShadowMap.program");
			renderer->RegisterImage("Gfx/AmbientOcclusion.tga");
		}
		GLOptimizedVoxelModel::GLOptimizedVoxelModel(VoxelModel *m,
								   GLRenderer *r){
			SPADES_MARK_FUNCTION();
			
			renderer = r;
			device = r->GetGLDevice();
			
			BuildVertices(m);
			GenerateTexture();
			
			program = renderer->RegisterProgram("Shaders/OptimizedVoxelModel.program");
			dlightProgram = renderer->RegisterProgram("Shaders/OptimizedVoxelModelDynamicLit.program");
			shadowMapProgram = renderer->RegisterProgram("Shaders/OptimizedVoxelModelShadowMap.program");
			aoImage = (GLImage *)renderer->RegisterImage("Gfx/AmbientOcclusion.tga");
			
			
			buffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->BufferData(IGLDevice::ArrayBuffer,
							   vertices.size() * sizeof(Vertex),
							   vertices.data(), IGLDevice::StaticDraw);
			
			idxBuffer = device->GenBuffer();
			device->BindBuffer(IGLDevice::ArrayBuffer, idxBuffer);
			device->BufferData(IGLDevice::ArrayBuffer,
							   indices.size() * sizeof(uint32_t),
							   indices.data(), IGLDevice::StaticDraw);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			
			origin = m->GetOrigin();
			origin -= .5f; // (0,0,0) is center of voxel (0,0,0)
			
			Vector3 minPos = {0, 0, 0};
			Vector3 maxPos = {
				(float)m->GetWidth(), (float)m->GetHeight(), (float)m->GetDepth()
			};
			minPos += origin; maxPos += origin;
			Vector3 maxDiff = {
				std::max(fabsf(minPos.x), fabsf(maxPos.x)),
				std::max(fabsf(minPos.y), fabsf(maxPos.y)),
				std::max(fabsf(minPos.z), fabsf(maxPos.z))
			};
			radius = maxDiff.GetLength();
			
			boundingBox.min = minPos;
			boundingBox.max = maxPos;
			
			// clean up
			numIndices = (unsigned int)indices.size();
			std::vector<Vertex>().swap(vertices);
			std::vector<uint32_t>().swap(indices);
		}
		GLOptimizedVoxelModel::~GLOptimizedVoxelModel() {
			SPADES_MARK_FUNCTION();
			
			image->Release();
			device->DeleteBuffer(idxBuffer);
			device->DeleteBuffer(buffer);
		}
		
		void GLOptimizedVoxelModel::GenerateTexture() {
			BitmapAtlasGenerator atlasGen;
			std::map<Bitmap *, int> idx;
			std::vector<IntVector3> poss;
			for(size_t i = 0; i < bmps.size(); i++){
				idx[bmps[i]] = i;
				atlasGen.AddBitmap(bmps[i]);
			}
			
			BitmapAtlasGenerator::Result result = atlasGen.Pack();
			Handle<Bitmap> bmp = result.bitmap;
			SPAssert(result.items.size() == bmps.size());
			for(size_t i = 0; i < bmps.size(); i++){
				bmps[i]->Release();
			}
			bmps.clear();
			
			poss.resize(result.items.size());
			for(size_t i = 0; i < result.items.size(); i++){
				const BitmapAtlasGenerator::Item& item = result.items[i];
				int id = idx[item.bitmap];
				poss[id] = IntVector3::Make(item.x, item.y, 0);
			}
			
			// move vertices' texture coord
			SPAssert(bmpIndex.size() == vertices.size());
			for(size_t i = 0; i < bmpIndex.size(); i++){
				int id = (int)bmpIndex[i];
				Vertex& v = vertices[i];
				IntVector3 p = poss[id];
				v.u += p.x;
				v.v += p.y;
			}
			
			std::vector<uint16_t>().swap(bmpIndex);
			
			image = static_cast<GLImage *>(renderer->CreateImage(bmp));
		}
		
		uint8_t GLOptimizedVoxelModel::calcAOID(VoxelModel *m,
									   int x, int y, int z,
									   int ux, int uy, int uz,
									   int vx, int vy, int vz){
			int v = 0;
			if(m->IsSolid(x - ux, y - uy, z - uz))
				v |= 1;
			if(m->IsSolid(x + ux, y + uy, z + uz))
				v |= 1 << 1;
			if(m->IsSolid(x - vx, y - vy, z - vz))
				v |= 1 << 2;
			if(m->IsSolid(x + vx, y + vy, z + vz))
				v |= 1 << 3;
			if(m->IsSolid(x - ux + vx, y - uy + vy, z - uz + vz))
				v |= 1 << 4;
			if(m->IsSolid(x - ux - vx, y - uy - vy, z - uz - vz))
				v |= 1 << 5;
			if(m->IsSolid(x + ux + vx, y + uy + vy, z + uz + vz))
				v |= 1 << 6;
			if(m->IsSolid(x + ux - vx, y + uy - vy, z + uz - vz))
				v |= 1 << 7;
			return (uint8_t)v;
		}
		
		template <class C> static void FreeClear( C & cntr ) {
			for ( typename C::iterator it = cntr.begin();
				 it != cntr.end(); ++it ) {
				delete * it;
			}
			cntr.clear();
		}
		
		class GLOptimizedVoxelModel::SliceGenerator {
			
		public:
			//0 - air
			//1 - unprocessed solid
			//2 - "inside" the processed area, but not connected
			//3 - found holes
			uint8_t *slice;
			int usize, vsize;
			int minU, maxU, minV, maxV;
			
			struct Point {
				int x;
				int y;
				int sx, sy;
				Point(){}
				Point(int x, int y):x(x),y(y){}
				Point(int x, int y, int sx, int sy):x(x),y(y),sx(x),sy(y){}
			};
			
			// poly2tri point pool
			std::vector<p2t::Point> gPoints;
			typedef p2t::Point *PointIndex;
			
			
			uint8_t& Slice(int u, int v){
				SPAssert(u >= 0); SPAssert(v >= 0);
				SPAssert(u < usize); SPAssert(v < vsize);
				return slice[u * vsize + v];
			}
			uint8_t GetSlice(int u, int v){
				if(u < 0 || v < 0 || u >= usize || v >= vsize)
					return 0;
				return Slice(u, v);
			}
			
			PointIndex EmitPoint(int x, int y){
				gPoints.push_back(p2t::Point(x, y));
				return (PointIndex)(gPoints.size() - 1);
			}
			
			PointIndex EmitPoint(const Point& p){
				// exact coordinate causes error on p2t library...
				gPoints.push_back(p2t::Point(p.x + p.sx * 0.001 + (GetRandom() - GetRandom()) * 0.0005,
											 p.y + p.sy * 0.001 + (GetRandom() - GetRandom()) * 0.0005));
				return (PointIndex)(gPoints.size() - 1);
			}
			
			void ResolvePoints(std::vector<PointIndex>& points){
				SPADES_MARK_FUNCTION_DEBUG();
				for(size_t i = 0; i < points.size(); i++){
					points[i] = &(gPoints[(size_t)points[i]]);
				}
			}
			
			class PolygonFillIterator {
				int top, height;
				struct Span {
					std::set<int> xs;
				};
				Span *spans;
				int curX, curY;
				std::set<int>::iterator nextIt;
			public:
				PolygonFillIterator(const std::vector<Point>& points) {
					SPADES_MARK_FUNCTION_DEBUG();
					int minY = -1, maxY = -1;
					for(size_t i = 0; i < points.size(); i++){
						const Point& p = points[i];
						if(minY == -1 || p.y < minY) minY = p.y;
						if(maxY == -1 || p.y > maxY) maxY = p.y;
					}
					top = minY;
					height = maxY - minY;
					if(height == 0){
						spans = NULL;
					}else{
						spans = new Span[height];
						Point last = points[points.size() - 1];
						for(size_t i = 0; i < points.size(); i++){
							Point p = points[i];
							if(p.y != last.y){
								SPAssert(p.x == last.x);
								minY = std::min(p.y, last.y);
								maxY = std::max(p.y, last.y);
								for(int y = minY; y < maxY; y++){
									SPAssert(spans[y-top].xs.find(p.x) ==
											 spans[y-top].xs.end());
									SPAssert(y - top < height);
									spans[y - top].xs.insert(p.x);
								}
							}
							last = p;
						}
					}
					curY = -1;
				}
				~PolygonFillIterator() {
					if(spans)
						delete[] spans;
				}
				
				bool Next(Point& outPoint) {
					if(curY >= height)
						return false;
					SPADES_MARK_FUNCTION_DEBUG();
					
					// check current span
					if(curY != -1){
						Span& span = spans[curY];
						if(nextIt != span.xs.end()){
							// emit
							outPoint.x = curX;
							outPoint.y = curY + top;
							curX++;
							SPAssert(curX <= *nextIt);
							if(curX == *nextIt){
								nextIt++;
								// move to next solid
								if(nextIt == span.xs.end()){
									// span end
								}else{
									// next solid
									curX = *nextIt;
									nextIt++;
									SPAssert(nextIt != span.xs.end());
								}
							}
							return true;
						}
					}
					
					// move to next span
					curY++;
					while(curY < height){
						if(!spans[curY].xs.empty()){
							break;
						}
						curY++;
					}
					
					if(curY >= height){
						return false;
					}
					
					Span& span = spans[curY];
					SPAssert(!span.xs.empty());
					nextIt = span.xs.begin();
					outPoint.x = *nextIt;
					outPoint.y = curY + top;
					
					curX = outPoint.x + 1;
					nextIt++;
					SPAssert(curX <= *nextIt);
					if(curX == *nextIt){
						nextIt++;
						// move to next solid
						if(nextIt == span.xs.end()){
							// span end
						}else{
							// next solid
							curX = *nextIt;
							nextIt++;
							SPAssert(nextIt != span.xs.end());
						}
					}
					
					return true;
				}
			};
			
			// given a top-side point, makes path
			std::vector<Point> MakePolyline(int su, int sv, uint8_t inner) {
				std::vector<Point> poly;
				SPADES_MARK_FUNCTION_DEBUG();
				SPAssert(GetSlice(su, sv) == inner);
				SPAssert(GetSlice(su, sv - 1) != inner);
				
				// go as backward as possible
				while(GetSlice(su, sv) == inner){
					su--;
				}
				su++;
				
				// we support only the case (su, sv) is a corner
				SPAssert(GetSlice(su, sv - 1) != inner);
				
				int u = su, v = sv;
				int du = 1, dv = 0;
				bool corner = true;
				bool first = true;
				// coord. system: u = right, v = down
				if(u == 3 && v == 3){
					first = true;
				}
				
				while(u != su || v != sv || du != 1 || dv != 0 || first) {
					int outU, outV; // vector to outside (polygon normal)
					if(dv == 0) { outU = 0; outV = -du; }
					else { outU = dv; outV = 0; }
					
					SPAssert(GetSlice(u, v) == inner);
					SPAssert(GetSlice(u + outU, v + outV) != inner);
					
					if(corner){
						if(du == 1)
							poly.push_back(Point(u, v,1,1));
						else if(du == -1)
							poly.push_back(Point(u+1, v+1, -1,-1));
						else if(dv == 1)
							poly.push_back(Point(u+1, v, -1,1));
						else if(dv == -1)
							poly.push_back(Point(u, v+1, 1,-1));
						else
							SPAssert(false);
						corner = false;
					}
					
					// go forward
					u += du; v += dv;
					
					if(GetSlice(u, v) != inner){
						// go back, and turn right
						u -= du; v -= dv;
						int odu = du, odv = dv;
						du = -odv; dv = odu;
						corner = true;
					}else if(GetSlice(u + outU, v + outV) == inner){
						// turn left, and go forward
						int odu = du, odv = dv;
						du = odv; dv = -odu;
						u += du; v += dv;
						corner = true;
					}
				
					first = false;
				}
				
				SPAssert(corner);
				SPAssert(!first);
				SPAssert(poly.size() >= 4);
				
				return poly;
			}
			
			/** start scanning area from (su, sv) */
			p2t::CDT *ProcessArea(int su, int sv) {
				SPADES_MARK_FUNCTION_DEBUG();
				gPoints.clear();
				
				std::vector<Point> exteria;
				std::vector<std::vector<Point> > holes;
				exteria = MakePolyline(su, sv, 1);
				
				{
					// find holes
					PolygonFillIterator filler(exteria);
					Point pt;
					while(filler.Next(pt)){
						int type = GetSlice(pt.x, pt.y);
						if(type == 0){
							// hole
							std::vector<Point> hole;
							hole = MakePolyline(pt.x, pt.y, 0);
							holes.push_back(hole);
							
							// exclude areas inside the hole
							PolygonFillIterator filler2(hole);
							while(filler2.Next(pt)){
								type = GetSlice(pt.x, pt.y);
								if(type == 1){
									Slice(pt.x, pt.y) = 2;
								}else if(type == 0){
									Slice(pt.x, pt.y) = 3;
								}else{
									SPAssert(false);
								}
							}
						}
					}
				}
				
				{
					// remove this area
					PolygonFillIterator filler(exteria);
					Point pt;
					while(filler.Next(pt)){
						int type = GetSlice(pt.x, pt.y);
						if(type == 0){
							SPAssert(false);
						}else if(type == 1){ // this area
							Slice(pt.x, pt.y) = 0;
						}else if(type == 2){ // not this area, but inside
							Slice(pt.x, pt.y) = 1;
						}else if(type == 3){ // hole of this area
							Slice(pt.x, pt.y) = 0;
						}else{
							SPAssert(false);
						}
					}
				}
				
				std::vector<std::vector<PointIndex> > pts2;
				p2t::CDT *cdt;
				{
					std::vector<PointIndex> poly;
					for(size_t i = 0; i < exteria.size(); i++){
						Point pt = exteria[i];
						poly.push_back(EmitPoint(pt));
					}
					pts2.push_back(poly);
				}
				for(size_t j = 0; j < holes.size(); j++){
					std::vector<PointIndex> poly;
					const std::vector<Point>& ipoly = holes[j];
					for(size_t i = 0; i < ipoly.size(); i++){
						Point pt = ipoly[i];
						poly.push_back(EmitPoint(pt));
					}
					pts2.push_back(poly);
				}
				
				for(size_t i = 0; i < pts2.size(); i++){
					ResolvePoints(pts2[i]);
				}
				
				cdt = new p2t::CDT(pts2[0]);
				
				for(size_t i = 1; i < pts2.size(); i++)
					cdt->AddHole(pts2[i]);
				
				cdt->Triangulate();
				
				return cdt;
			}
			
		};
		
		static IntVector3 ExactPoint(const p2t::Point *pt){
			return IntVector3::Make(lround(pt->x), lround(pt->y),0);
		}
		
		static int64_t DoubledTriangleArea(IntVector3 v1, IntVector3 v2, IntVector3 v3) {
			int64_t x1 = v1.x, y1 = v1.y;
			int64_t x2 = v2.x, y2 = v2.y;
			int64_t x3 = v3.x, y3 = v3.y;
			return (x1 - x3) * (y2 - y1) -
			(x1 - x2) * (y3 - y1);
		}
		
		void GLOptimizedVoxelModel::EmitSlice(uint8_t *slice,
											  int usize, int vsize,
											  int sx, int sy, int sz,
											  int ux, int uy, int uz,
											  int vx, int vy, int vz,
											  int mx, int my, int mz,
											  bool flip,
											  VoxelModel *model) {
			SPADES_MARK_FUNCTION();
			int minU = -1, minV = -1, maxU = -1, maxV = -1;
			
			for(int u = 0; u < usize; u++){
				for(int v = 0; v < vsize; v++){
					if(slice[u * vsize + v]){
						if(minU == -1 || u < minU) minU = u;
						if(maxU == -1 || u > maxU) maxU = u;
						if(minV == -1 || v < minV) minV = v;
						if(maxV == -1 || v > maxV) maxV = v;
					}
				}
			}
			
			if(minU == -1){
				// no face
				return;
			}
			
			int nx, ny, nz;
			nx = uy * vz - uz * vy;
			ny = uz * vx - ux * vz;
			nz = ux * vy - uy * vx;
			if(!flip){
				nx = -nx;
				ny = -ny;
				nz = -nz;
			}
			
			SliceGenerator generator;
			generator.slice = slice;
			generator.usize = usize;
			generator.vsize = vsize;
			generator.minU = minU;
			generator.maxU = maxU;
			generator.minV = minV;
			generator.maxV = maxV;
			
			int tu = minU - 1, tv = minV - 1;
			int bw = (maxU - minU) + 3, bh = (maxV - minV) + 3;
			int bId = bmps.size();
			Bitmap *bmp = new Bitmap(bw, bh);
			bmps.push_back(bmp);
			{
				uint32_t *pixels = bmp->GetPixels();
				IntVector3 p1 = {mx, my, mz};
				
				IntVector3 uu = {ux, uy, uz};
				IntVector3 vv = {vx, vy, vz};
				IntVector3 nn = {nx, ny, nz};
				
				p1 += uu * tu; p1 += vv * tv;
				
				for(int y = 0; y < bh; y++){
					IntVector3 p2 = p1;
					for(int x = 0; x < bw; x++){
						IntVector3 p3 = p2;
						int u = x + tu, v = y + tv;
						if(u < 0 || v < 0 || u >= usize || v >= vsize ||
						   !slice[u*vsize+v]){
							if((v >= 0 && v < vsize) && u > 0 && slice[(u-1)*vsize+(v)]){
								u--;
								p3 -= uu;
							}else if((v >= 0 && v < vsize) && u < usize-1 && slice[(u+1)*vsize+(v)]){
								u++;
								p3 += uu;
							}else if((u >= 0 && u < usize) && v > 0 && slice[(u)*vsize+(v-1)]){
								v--;
								p3 -= vv;
							}else if((u >= 0 && u < usize) && v < vsize-1 && slice[(u)*vsize+(v+1)]){
								v++;
								p3 += vv;
							}else if(u > 0 && v > 0 && slice[(u-1)*vsize+(v-1)]){
								u--; v--;
								p3 -= uu; p3 -= vv;
							}else if(u > 0 && v < vsize-1 && slice[(u-1)*vsize+(v+1)]){
								u--; v++;
								p3 -= uu; p3 += vv;
							}else if(u < usize-1 && v > 0 && slice[(u+1)*vsize+(v-1)]){
								u++; v--;
								p3 += uu; p3 -= vv;
							}else if(u < usize-1 && v < vsize-1 && slice[(u+1)*vsize+(v+1)]){
								u++; v++;
								p3 += uu; p3 += vv;
							}else{
								*(pixels++) = 0x00ff00ff;
								p2 += uu;
								continue;
							}
						}
						
						SPAssert(model->IsSolid(p3.x, p3.y, p3.z));
						uint32_t col = model->GetColor(p3.x, p3.y, p3.z);
						
						col &= 0xffffff;
						
						// add AOID
						p3 += nn;
						SPAssert(!model->IsSolid(p3.x, p3.y, p3.z));
						uint8_t aoId = calcAOID(model,
												p3.x, p3.y, p3.z,
												ux, uy, uz,
												vx, vy, vz);
						col |= ((uint8_t)aoId) << 24;
						
						*(pixels++) = col;
						
						p2 += uu;
					}
					
					p1 += vv;
				}
			}
			
			// TODO: optimize scan range
			for(int v = 0; v < vsize; v++){
				for(int u = 0; u < usize; u++){
					if(slice[u * vsize + v]){
						p2t::CDT *cdt = generator.ProcessArea(u, v);
						std::vector<p2t::Triangle *> triangles = cdt->GetTriangles();
						
						for(size_t i = 0; i < triangles.size(); i++){
							p2t::Triangle& tri = *triangles[i];
							uint32_t idx = (uint32_t)vertices.size();
							IntVector3 pt1 = ExactPoint(tri.GetPoint(0));
							IntVector3 pt2 = ExactPoint(tri.GetPoint(1));
							IntVector3 pt3 = ExactPoint(tri.GetPoint(2));
							
							// degenerate triangle
							if(DoubledTriangleArea(pt1, pt2, pt3) == 0)
								continue;
							
							Vertex vtx;
							vtx.nx = nx; vtx.ny = ny; vtx.nz = nz;
							
							vtx.x = sx + (int)pt1.x * ux + (int)pt1.y * vx;
							vtx.y = sy + (int)pt1.x * uy + (int)pt1.y * vy;
							vtx.z = sz + (int)pt1.x * uz + (int)pt1.y * vz;
							vtx.u = (int)pt1.x - tu;
							vtx.v = (int)pt1.y - tv;
							vertices.push_back(vtx);
							
							vtx.x = sx + (int)pt2.x * ux + (int)pt2.y * vx;
							vtx.y = sy + (int)pt2.x * uy + (int)pt2.y * vy;
							vtx.z = sz + (int)pt2.x * uz + (int)pt2.y * vz;
							vtx.u = (int)pt2.x - tu;
							vtx.v = (int)pt2.y - tv;
							vertices.push_back(vtx);
							
							vtx.x = sx + (int)pt3.x * ux + (int)pt3.y * vx;
							vtx.y = sy + (int)pt3.x * uy + (int)pt3.y * vy;
							vtx.z = sz + (int)pt3.x * uz + (int)pt3.y * vz;
							vtx.u = (int)pt3.x - tu;
							vtx.v = (int)pt3.y - tv;
							vertices.push_back(vtx);
							
							if(flip){
								indices.push_back(idx+2);
								indices.push_back(idx+1);
								indices.push_back(idx);
								
							}else{
								indices.push_back(idx);
								indices.push_back(idx+1);
								indices.push_back(idx+2);
							}
							bmpIndex.push_back(bId);
							bmpIndex.push_back(bId);
							bmpIndex.push_back(bId);
						}
						
						delete cdt;
					}
				}
			}
			
		}
		
		void GLOptimizedVoxelModel::BuildVertices(spades::VoxelModel *model) {
			SPADES_MARK_FUNCTION();
			
			SPAssert(vertices.empty());
			SPAssert(indices.empty());
			
			int w = model->GetWidth();
			int h = model->GetHeight();
			int d = model->GetDepth();
			
			std::vector<uint8_t> slice;
			
			// x-slice
			slice.resize(h * d);
			std::fill(slice.begin(), slice.end(), 0);
			for(int x = 0; x < w; x++){
				for(int y = 0; y < h; y++){
					for(int z = 0; z < d; z++){
						uint8_t& s = slice[y * d + z];
						if(x == 0)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) &&
								 !model->IsSolid(x - 1, y, z))
							? 1 : 0;
					}
				}
				EmitSlice(slice.data(), h, d,
						  x, 0, 0,
						  0, 1, 0,
						  0, 0, 1,
						  x, 0, 0,
						  false,
						  model);
				
				for(int y = 0; y < h; y++){
					for(int z = 0; z < d; z++){
						uint8_t& s = slice[y * d + z];
						if(x == w - 1)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) &&
								 !model->IsSolid(x + 1, y, z))
							? 1 : 0;
					}
				}
				EmitSlice(slice.data(), h, d,
						  x + 1, 0, 0,
						  0, 1, 0,
						  0, 0, 1,
						  x, 0, 0,
						  true,
						  model);
			}
			
			// y-slice
			slice.resize(w * d);
			std::fill(slice.begin(), slice.end(), 0);
			for(int y = 0; y < h; y++){
				for(int x = 0; x < w; x++){
					for(int z = 0; z < d; z++){
						uint8_t& s = slice[x * d + z];
						if(y == 0)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) &&
								 !model->IsSolid(x, y - 1, z))
							? 1 : 0;
					}
				}
				EmitSlice(slice.data(), w, d,
						  0, y, 0,
						  1, 0, 0,
						  0, 0, 1,
						  0, y, 0,
						  true,
						  model);
				
				for(int x = 0; x < w; x++){
					for(int z = 0; z < d; z++){
						uint8_t& s = slice[x * d + z];
						if(y == h - 1)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) &&
								 !model->IsSolid(x, y + 1, z))
							? 1 : 0;
					}
				}
				EmitSlice(slice.data(), w, d,
						  0, y + 1, 0,
						  1, 0, 0,
						  0, 0, 1,
						  0, y, 0,
						  false,
						  model);
			}
			
			// z-slice
			slice.resize(w * h);
			std::fill(slice.begin(), slice.end(), 0);
			for(int z = 0; z < d; z++){
				for(int x = 0; x < w; x++){
					for(int y = 0; y < h; y++){
						uint8_t& s = slice[x * h + y];
						if(z == 0)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) &&
								 !model->IsSolid(x, y, z - 1))
							? 1 : 0;
					}
				}
				EmitSlice(slice.data(), w, h,
						  0, 0, z,
						  1, 0, 0,
						  0, 1, 0,
						  0, 0, z,
						  false,
						  model);
				
				for(int x = 0; x < w; x++){
					for(int y = 0; y < h; y++){
						uint8_t& s = slice[x * h + y];
						if(z == d - 1)
							s = model->IsSolid(x, y, z) ? 1 : 0;
						else
							s = (model->IsSolid(x, y, z) &&
								 !model->IsSolid(x, y, z + 1))
							? 1 : 0;
					}
				}
				EmitSlice(slice.data(), w, h,
						  0, 0, z + 1,
						  1, 0, 0,
						  0, 1, 0,
						  0, 0, z,
						  true,
						  model);
			}
			
			printf("%d vertices emit\n", (int)indices.size());
		}
		
		void GLOptimizedVoxelModel::RenderShadowMapPass(std::vector<client::ModelRenderParam> params) {
			SPADES_MARK_FUNCTION();
			
			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);
			
			shadowMapProgram->Use();
			
			static GLShadowMapShader shadowMapShader;
			shadowMapShader(renderer, shadowMapProgram, 0);
			
			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(shadowMapProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);
			
			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");
			
			positionAttribute(shadowMapProgram);
			normalAttribute(shadowMapProgram);
			
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(),
										4, IGLDevice::UnsignedByte,
										false, sizeof(Vertex),
										(void *)0);
			if(normalAttribute() != -1){
				device->VertexAttribPointer(normalAttribute(),
											3, IGLDevice::Byte,
											false, sizeof(Vertex),
											(void *)8);
			}
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			
			device->EnableVertexAttribArray(positionAttribute(), true);
			if(normalAttribute() != -1)
				device->EnableVertexAttribArray(normalAttribute(), true);
			
			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);
			
			for(size_t i = 0; i < params.size(); i++){
				const client::ModelRenderParam& param = params[i];
				
				// frustrum cull
				float rad = radius;
				rad *= param.matrix.GetAxis(0).GetLength();
				
				if(param.depthHack)
					continue;
				
				if(!renderer->GetShadowMapRenderer()->SphereCull(param.matrix.GetOrigin(),
																 rad)){
					continue;
				}
				
				Matrix4 modelMatrix = param.matrix;
				
				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(shadowMapProgram);
				modelMatrixU.SetValue(modelMatrix);
				
				modelMatrix.m[12] = 0.f;
				modelMatrix.m[13] = 0.f;
				modelMatrix.m[14] = 0.f;
				static GLProgramUniform modelNormalMatrix("modelNormalMatrix");
				modelNormalMatrix(shadowMapProgram);
				modelNormalMatrix.SetValue(modelMatrix);
				
				device->DrawElements(IGLDevice::Triangles,
									 numIndices,
									 IGLDevice::UnsignedInt,
									 (void *)0);
				
			}
			
			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
			
			
			device->EnableVertexAttribArray(positionAttribute(), false);
			if(normalAttribute() != -1)
				device->EnableVertexAttribArray(normalAttribute(), false);
			
			device->ActiveTexture(0);
			device->BindTexture(IGLDevice::Texture2D, 0);
		}
		
		void GLOptimizedVoxelModel::RenderSunlightPass(std::vector<client::ModelRenderParam> params) {
			SPADES_MARK_FUNCTION();
			
			bool mirror = renderer->IsRenderingMirror();
			
			device->ActiveTexture(0);
			aoImage->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureMinFilter,
								 IGLDevice::Linear);
			
			device->ActiveTexture(1);
			image->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureMinFilter,
								 IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureMagFilter,
								 IGLDevice::Nearest);
			
			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);
			
			program->Use();
			
			static GLShadowShader shadowShader;
			shadowShader(renderer, program, 2);
			
			static GLProgramUniform fogDistance("fogDistance");
			fogDistance(program);
			fogDistance.SetValue(renderer->GetFogDistance());
			
			static GLProgramUniform fogColor("fogColor");
			fogColor(program);
			Vector3 fogCol = renderer->GetFogColorForSolidPass();
			fogCol *= fogCol; // linearize
			fogColor.SetValue(fogCol.x, fogCol.y, fogCol.z);
			
			static GLProgramUniform aoUniform("ambientOcclusionTexture");
			aoUniform(program);
			aoUniform.SetValue(0);
			
			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(program);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);
			
			static GLProgramUniform texScale("texScale");
			texScale(program);
			texScale.SetValue(1.f / image->GetWidth(),
							  1.f / image->GetHeight());
			
			static GLProgramUniform modelTexture("modelTexture");
			modelTexture(program);
			modelTexture.SetValue(1);
			
			static GLProgramUniform sunLightDirection("sunLightDirection");
			sunLightDirection(program);
			Vector3 sunPos = MakeVector3(0, -1, -1);
			sunPos = sunPos.Normalize();
			sunLightDirection.SetValue(sunPos.x, sunPos.y, sunPos.z);
			
			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute textureCoordAttribute("textureCoordAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");
			
			positionAttribute(program);
			textureCoordAttribute(program);
			normalAttribute(program);
			
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(),
										4, IGLDevice::UnsignedByte,
										false, sizeof(Vertex),
										(void *)0);
			device->VertexAttribPointer(textureCoordAttribute(),
										2, IGLDevice::UnsignedShort,
										false, sizeof(Vertex),
										(void *)4);
			device->VertexAttribPointer(normalAttribute(),
										3, IGLDevice::Byte,
										false, sizeof(Vertex),
										(void *)8);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(textureCoordAttribute(), true);
			device->EnableVertexAttribArray(normalAttribute(), true);
			
			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);
			
			for(size_t i = 0; i < params.size(); i++){
				const client::ModelRenderParam& param = params[i];
				
				if(mirror && param.depthHack)
					continue;
				
				// frustrum cull
				float rad = radius;
				rad *= param.matrix.GetAxis(0).GetLength();
				if(!renderer->SphereFrustrumCull(param.matrix.GetOrigin(),
												 rad)){
					continue;
				}
				
				
				static GLProgramUniform customColor("customColor");
				customColor(program);
				customColor.SetValue(param.customColor.x, param.customColor.y, param.customColor.z);
				
				
				Matrix4 modelMatrix = param.matrix;
				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(program);
				projectionViewModelMatrix.SetValue(renderer->GetProjectionViewMatrix() * modelMatrix);
				
				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				viewModelMatrix(program);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * modelMatrix);
				
				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(program);
				modelMatrixU.SetValue(modelMatrix);
				
				modelMatrix.m[12] = 0.f;
				modelMatrix.m[13] = 0.f;
				modelMatrix.m[14] = 0.f;
				static GLProgramUniform modelNormalMatrix("modelNormalMatrix");
				modelNormalMatrix(program);
				modelNormalMatrix.SetValue(modelMatrix);
				
				if(param.depthHack){
					device->DepthRange(0.f, 0.1f);
				}
				
				device->DrawElements(IGLDevice::Triangles,
									 numIndices,
									 IGLDevice::UnsignedInt,
									 (void *)0);
				if(param.depthHack){
					device->DepthRange(0.f, 1.f);
				}
				
			}
			
			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
			
			
			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(textureCoordAttribute(), false);
			device->EnableVertexAttribArray(normalAttribute(), false);
			
			device->ActiveTexture(1);
			device->BindTexture(IGLDevice::Texture2D, 0);
			device->ActiveTexture(0);
			device->BindTexture(IGLDevice::Texture2D, 0);
		}
		
		void GLOptimizedVoxelModel::RenderDynamicLightPass(std::vector<client::ModelRenderParam> params, std::vector<GLDynamicLight> lights) {
			SPADES_MARK_FUNCTION();
			
			bool mirror = renderer->IsRenderingMirror();
			
			device->ActiveTexture(0);
			aoImage->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureMinFilter,
								 IGLDevice::Linear);
			
			device->ActiveTexture(1);
			image->Bind(IGLDevice::Texture2D);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureMinFilter,
								 IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D,
								 IGLDevice::TextureMagFilter,
								 IGLDevice::Nearest);
			
			device->Enable(IGLDevice::CullFace, true);
			device->Enable(IGLDevice::DepthTest, true);
						
			dlightProgram->Use();
			
			static GLDynamicLightShader dlightShader;
			
			static GLProgramUniform fogDistance("fogDistance");
			fogDistance(dlightProgram);
			fogDistance.SetValue(renderer->GetFogDistance());
			
			static GLProgramUniform modelOrigin("modelOrigin");
			modelOrigin(dlightProgram);
			modelOrigin.SetValue(origin.x, origin.y, origin.z);
			
			
			static GLProgramUniform texScale("texScale");
			texScale(dlightProgram);
			texScale.SetValue(1.f / image->GetWidth(),
							  1.f / image->GetHeight());
			
			static GLProgramUniform modelTexture("modelTexture");
			modelTexture(dlightProgram);
			modelTexture.SetValue(1);
			
			// setup attributes
			static GLProgramAttribute positionAttribute("positionAttribute");
			static GLProgramAttribute textureCoordAttribute("textureCoordAttribute");
			static GLProgramAttribute normalAttribute("normalAttribute");
			
			positionAttribute(dlightProgram);
			textureCoordAttribute(dlightProgram);
			normalAttribute(dlightProgram);
			
			device->BindBuffer(IGLDevice::ArrayBuffer, buffer);
			device->VertexAttribPointer(positionAttribute(),
										4, IGLDevice::UnsignedByte,
										false, sizeof(Vertex),
										(void *)0);
			device->VertexAttribPointer(textureCoordAttribute(),
										2, IGLDevice::UnsignedShort,
										false, sizeof(Vertex),
										(void *)4);
			device->VertexAttribPointer(normalAttribute(),
										3, IGLDevice::Byte,
										false, sizeof(Vertex),
										(void *)8);
			device->BindBuffer(IGLDevice::ArrayBuffer, 0);
			
			device->EnableVertexAttribArray(positionAttribute(), true);
			device->EnableVertexAttribArray(textureCoordAttribute(), true);
			device->EnableVertexAttribArray(normalAttribute(), true);
			
			device->BindBuffer(IGLDevice::ElementArrayBuffer, idxBuffer);
			
			for(size_t i = 0; i < params.size(); i++){
				const client::ModelRenderParam& param = params[i];
				
				if(mirror && param.depthHack)
					continue;
				
				// frustrum cull
				float rad = radius;
				rad *= param.matrix.GetAxis(0).GetLength();
				if(!renderer->SphereFrustrumCull(param.matrix.GetOrigin(),
												 rad)){
					continue;
				}
				
				
				static GLProgramUniform customColor("customColor");
				customColor(dlightProgram);
				customColor.SetValue(param.customColor.x, param.customColor.y, param.customColor.z);
				
				
				Matrix4 modelMatrix = param.matrix;
				static GLProgramUniform projectionViewModelMatrix("projectionViewModelMatrix");
				projectionViewModelMatrix(dlightProgram);
				projectionViewModelMatrix.SetValue(renderer->GetProjectionViewMatrix() * modelMatrix);
				
				static GLProgramUniform viewModelMatrix("viewModelMatrix");
				viewModelMatrix(dlightProgram);
				viewModelMatrix.SetValue(renderer->GetViewMatrix() * modelMatrix);
				
				static GLProgramUniform modelMatrixU("modelMatrix");
				modelMatrixU(dlightProgram);
				modelMatrixU.SetValue(modelMatrix);
				
				modelMatrix.m[12] = 0.f;
				modelMatrix.m[13] = 0.f;
				modelMatrix.m[14] = 0.f;
				static GLProgramUniform modelNormalMatrix("modelNormalMatrix");
				modelNormalMatrix(dlightProgram);
				modelNormalMatrix.SetValue(modelMatrix);
				
				if(param.depthHack){
					device->DepthRange(0.f, 0.1f);
				}
				for(size_t i = 0; i < lights.size(); i++){
					if(!GLDynamicLightShader::SphereCull(lights[i],
														 param.matrix.GetOrigin(), rad))
						continue;
					
					dlightShader(renderer, dlightProgram, lights[i], 2);
					
					device->DrawElements(IGLDevice::Triangles,
										 numIndices,
										 IGLDevice::UnsignedInt,
										 (void *)0);
				}
				if(param.depthHack){
					device->DepthRange(0.f, 1.f);
				}
				
			}
			
			device->BindBuffer(IGLDevice::ElementArrayBuffer, 0);
			
			
			device->EnableVertexAttribArray(positionAttribute(), false);
			device->EnableVertexAttribArray(textureCoordAttribute(), false);
			device->EnableVertexAttribArray(normalAttribute(), false);
			
			device->ActiveTexture(0);
		}
		
	}
}
