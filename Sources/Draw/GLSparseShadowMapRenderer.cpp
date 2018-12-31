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

#include "GLSparseShadowMapRenderer.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/Settings.h>
#include "GLModel.h"
#include "GLModelRenderer.h"
#include "GLProfiler.h"
#include "GLRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"

namespace spades {
	namespace draw {

		GLSparseShadowMapRenderer::GLSparseShadowMapRenderer(GLRenderer *r)
		    : IGLShadowMapRenderer(r), device(r->GetGLDevice()), settings(r->GetSettings()) {
			SPADES_MARK_FUNCTION();

			textureSize = settings.r_shadowMapSize;
			if ((int)textureSize > 4096) {
				SPLog("r_shadowMapSize is too large; changed to 4096");
				settings.r_shadowMapSize = textureSize = 4096;
			}

			for (minLod = 0; (Tiles << minLod) < textureSize; minLod++)
				;
			// minLod = std::max(0, minLod - 2);
			minLod = 0;
			maxLod = 15;

			colorTexture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D, colorTexture);
			device->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGB, textureSize, textureSize, 0,
			                   IGLDevice::RGB, IGLDevice::UnsignedByte, NULL);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
			                     IGLDevice::ClampToEdge);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
			                     IGLDevice::ClampToEdge);

			texture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D, texture);
			device->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::DepthComponent24, textureSize,
			                   textureSize, 0, IGLDevice::DepthComponent, IGLDevice::UnsignedInt,
			                   NULL);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Linear);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
			                     IGLDevice::ClampToEdge);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
			                     IGLDevice::ClampToEdge);

			pagetableTexture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D, pagetableTexture);
			device->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGBA8, Tiles, Tiles, 0,
			                   IGLDevice::BGRA, IGLDevice::UnsignedByte, NULL);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                     IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                     IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS,
			                     IGLDevice::ClampToEdge);
			device->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT,
			                     IGLDevice::ClampToEdge);

			framebuffer = device->GenFramebuffer();
			device->BindFramebuffer(IGLDevice::Framebuffer, framebuffer);
			device->FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::ColorAttachment0,
			                             IGLDevice::Texture2D, colorTexture, 0);
			device->FramebufferTexture2D(IGLDevice::Framebuffer, IGLDevice::DepthAttachment,
			                             IGLDevice::Texture2D, texture, 0);

			device->BindFramebuffer(IGLDevice::Framebuffer, 0);
		}

		GLSparseShadowMapRenderer::~GLSparseShadowMapRenderer() {
			SPADES_MARK_FUNCTION();

			device->DeleteTexture(texture);
			device->DeleteTexture(pagetableTexture);
			device->DeleteFramebuffer(framebuffer);
			device->DeleteTexture(colorTexture);
		}

#define Segment GLSSMRSegment

		struct Segment {
			float low, high;
			bool empty;
			Segment() : empty(true) {}
			Segment(float l, float h) : low(std::min(l, h)), high(std::max(l, h)), empty(false) {}
			void operator+=(const Segment &seg) {
				if (seg.empty)
					return;
				if (empty) {
					low = seg.low;
					high = seg.high;
					empty = false;
				} else {
					low = std::min(low, seg.low);
					high = std::max(high, seg.high);
				}
			}
			void operator+=(float v) {
				if (empty) {
					low = high = v;
				} else {
					if (v < low)
						low = v;
					if (v > high)
						high = v;
				}
			}
		};

		static Vector3 FrustrumCoord(const client::SceneDefinition &def, float x, float y,
		                             float z) {
			x *= z;
			y *= z;
			return def.viewOrigin + def.viewAxis[0] * x + def.viewAxis[1] * y + def.viewAxis[2] * z;
		}

		static Segment ZRange(Vector3 base, Vector3 dir, Plane3 plane1, Plane3 plane2) {
			return Segment(plane1.GetDistanceTo(base) / Vector3::Dot(dir, plane1.n),
			               plane2.GetDistanceTo(base) / Vector3::Dot(dir, plane2.n));
		}

		void GLSparseShadowMapRenderer::BuildMatrix(float near, float far) {
			// TODO: variable light direction?
			Vector3 lightDir = MakeVector3(0, -1, -1).Normalize();
			// set better up dir?
			Vector3 up = MakeVector3(0, 0, 1);
			Vector3 side = Vector3::Cross(up, lightDir).Normalize();
			up = Vector3::Cross(lightDir, side).Normalize();

			// build frustrum
			client::SceneDefinition def = GetRenderer()->GetSceneDef();
			Vector3 frustrum[8];
			float tanX = tanf(def.fovX * .5f);
			float tanY = tanf(def.fovY * .5f);

			frustrum[0] = FrustrumCoord(def, tanX, tanY, near);
			frustrum[1] = FrustrumCoord(def, tanX, -tanY, near);
			frustrum[2] = FrustrumCoord(def, -tanX, tanY, near);
			frustrum[3] = FrustrumCoord(def, -tanX, -tanY, near);
			frustrum[4] = FrustrumCoord(def, tanX, tanY, far);
			frustrum[5] = FrustrumCoord(def, tanX, -tanY, far);
			frustrum[6] = FrustrumCoord(def, -tanX, tanY, far);
			frustrum[7] = FrustrumCoord(def, -tanX, -tanY, far);

			// compute frustrum's x,y boundary
			float minX, maxX, minY, maxY;
			minX = maxX = Vector3::Dot(frustrum[0], side);
			minY = maxY = Vector3::Dot(frustrum[0], up);
			for (int i = 1; i < 8; i++) {
				float x = Vector3::Dot(frustrum[i], side);
				float y = Vector3::Dot(frustrum[i], up);
				if (x < minX)
					minX = x;
				if (x > maxX)
					maxX = x;
				if (y < minY)
					minY = y;
				if (y > maxY)
					maxY = y;
			}

			// compute frustrum's z boundary
			Segment seg;
			Plane3 plane1(0, 0, 1, -4.f);
			Plane3 plane2(0, 0, 1, 64.f);
			seg += ZRange(side * minX + up * minY, lightDir, plane1, plane2);
			seg += ZRange(side * minX + up * maxY, lightDir, plane1, plane2);
			seg += ZRange(side * maxX + up * minY, lightDir, plane1, plane2);
			seg += ZRange(side * maxX + up * maxY, lightDir, plane1, plane2);

			for (int i = 1; i < 8; i++) {
				seg += Vector3::Dot(frustrum[i], lightDir);
			}

			// build frustrum obb
			Vector3 origin = side * minX + up * minY + lightDir * seg.low;
			Vector3 axis1 = side * (maxX - minX);
			Vector3 axis2 = up * (maxY - minY);
			Vector3 axis3 = lightDir * (seg.high - seg.low);

			obb = OBB3(Matrix4::FromAxis(axis1, axis2, axis3, origin));
			vpWidth = 2.f / axis1.GetLength();
			vpHeight = 2.f / axis2.GetLength();

			// convert to projectionview matrix
			matrix = obb.m.InversedFast();

			matrix = Matrix4::Scale(2.f) * matrix;
			matrix = Matrix4::Translate(-1, -1, -1) * matrix;

			// scale a little big for padding
			matrix = Matrix4::Scale(.98f) * matrix;
			//
			matrix = Matrix4::Scale(1, 1, -1) * matrix;

// make sure frustrums in range
#ifndef NDEBUG
			for (int i = 0; i < 8; i++) {
				Vector4 v = matrix * frustrum[i];
				SPAssert(v.x >= -1.f);
				SPAssert(v.y >= -1.f);
				// SPAssert(v.z >= -1.f);
				SPAssert(v.x < 1.f);
				SPAssert(v.y < 1.f);
				// SPAssert(v.z < 1.f);
			}
#endif
		}

		void GLSparseShadowMapRenderer::Render() {
			SPADES_MARK_FUNCTION();

			IGLDevice::Integer lastFb = device->GetInteger(IGLDevice::FramebufferBinding);

			// client::SceneDefinition def = GetRenderer()->GetSceneDef();

			float nearDist = 0.f;
			float farDist = 150.f;

			BuildMatrix(nearDist, farDist);

			device->BindFramebuffer(IGLDevice::Framebuffer, framebuffer);
			device->Viewport(0, 0, textureSize, textureSize);
			device->ClearDepth(1.f);
			device->Clear(IGLDevice::DepthBufferBit);

			RenderShadowMapPass();

			device->BindFramebuffer(IGLDevice::Framebuffer, lastFb);

			device->Viewport(0, 0, device->ScreenWidth(), device->ScreenHeight());
		}

#pragma mark - Sparse Processor

		static const size_t NoGroup = (size_t)-1;
		static const size_t NoInstance = (size_t)-1;
		static const size_t NoNode = (size_t)-1;
		static const size_t GroupNodeFlag = NoNode ^ (NoNode >> 1);

		struct GLSparseShadowMapRenderer::Internal {
			GLSparseShadowMapRenderer *renderer;
			Vector3 cameraShadowCoord;

			typedef int LodUnit;

			struct Instance {
				GLModel *model;
				const client::ModelRenderParam *param;
				IntVector3 tile1, tile2; // int aabb2
				size_t prev, next;       // instance in the same group
			};

			/** All Instances. Sorted by model somehow. */
			std::vector<Instance> allInstances;

			struct Group {
				IntVector3 tile1, tile2; // int aabb2
				size_t firstInstance, lastInstance;
				LodUnit lod;
				bool valid;

				// shadow map texture coordinate
				int rawX, rawY, rawW, rawH;
			};

			std::vector<Group> groups;

			// packing node
			struct Node {
				size_t child1, child2;
				int pos;
				bool horizontal; // true: x=pos, false: y=pos
			};

			std::vector<Node> nodes;
			int lodBias;

			int mapSize;

			size_t groupMap[Tiles][Tiles];

			static IntVector3 ShadowMapToTileCoord(Vector3 vec) {
				vec.x = (vec.x * (float)(Tiles / 2)) + (float)(Tiles / 2);
				vec.y = (vec.y * (float)(Tiles / 2)) + (float)(Tiles / 2);

				IntVector3 v;
				v.x = (int)floorf(vec.x);
				v.y = (int)floorf(vec.y);
				;
				return v;
			}

			static Vector3 TileToShadowMapCoord(IntVector3 tile) {
				Vector3 v;
				v.x = (float)tile.x * (2.f / (float)Tiles) - 1.f;
				v.y = (float)tile.y * (2.f / (float)Tiles) - 1.f;
				return v;
			}
			LodUnit ComputeLod(Vector3 shadowCoord) {
				float dx = shadowCoord.x - cameraShadowCoord.x;
				float dy = shadowCoord.y - cameraShadowCoord.y;
				float sq = dx * dx + dy * dy;
				float lod = 1.f / (sqrtf(sq) + .01f);
				int iv = (int)(lod * 200.f);
				if (iv < 1)
					iv = 1;
				if (iv > 65536)
					iv = 65535;

				int ld = 0;
				while (iv > 0) {
					ld++;
					iv >>= 1;
				}
				return ld;
			}

			void FillGroupMap(IntVector3 tile1, IntVector3 tile2, size_t val) {
				for (int x = tile1.x; x < tile2.x; x++)
					for (int y = tile1.y; y < tile2.y; y++)
						groupMap[x][y] = val;
			}

			/** Combines `dest` and `src`. Group `src` will be
			 * no longer valid. */
			void CombineGroup(size_t dest, size_t src) {
				Group &g1 = groups[dest];
				Group &g2 = groups[src];
				FillGroupMap(g2.tile1, g2.tile2, dest);

				// extend the area
				g1.tile1.x = std::min(g1.tile1.x, g2.tile1.x);
				g1.tile1.y = std::min(g1.tile1.y, g2.tile1.y);
				g1.tile2.x = std::max(g1.tile2.x, g2.tile2.x);
				g1.tile2.y = std::max(g1.tile2.y, g2.tile2.y);

				g1.lod = std::max(g1.lod, g2.lod);

				// combine the instance list
				// [g1] + [g2] -> [g1, g2]
				Instance &destLast = allInstances[g1.lastInstance];
				Instance &srcFirst = allInstances[g2.firstInstance];
				SPAssert(destLast.next == NoInstance);
				SPAssert(srcFirst.prev == NoInstance);
				destLast.next = g2.firstInstance;
				srcFirst.prev = g1.lastInstance;
				g1.lastInstance = g2.lastInstance;

				// make sure the area is filled with the group `dest`
				IntVector3 tile1 = g1.tile1, tile2 = g1.tile2;
				for (int x = tile1.x; x < tile2.x; x++)
					for (int y = tile1.y; y < tile2.y; y++) {
						size_t g = groupMap[x][y];
						SPAssert(g != src);
						if (g != NoGroup && g != dest) {
							CombineGroup(dest, g);
							SPAssert(groupMap[x][y] == dest);
						} else {
							groupMap[x][y] = dest;
						}
					}

				g2.valid = false;
			}

			/** Finds an group in the specified range (tile1, tile2).
			 * If one was not found, creates a new group.
			 * If more than one was found, combines all groups. */
			size_t RegisterGroup(IntVector3 tile1, IntVector3 tile2) {
				size_t foundGroup = NoGroup;
				for (int x = tile1.x; x < tile2.x; x++)
					for (int y = tile1.y; y < tile2.y; y++) {
						size_t g = groupMap[x][y];
						if (g == NoGroup)
							continue;
						if (g == foundGroup)
							continue;
						if (foundGroup == NoGroup) {
							foundGroup = g;
							break;
						}
						// another group found.
						// this should be combined with `foundGroup`.
						CombineGroup(foundGroup, g);
						SPAssert(groupMap[x][y] == foundGroup);
					}

				if (foundGroup == NoGroup) {
					// new group here.
					Group g;
					g.tile1 = tile1;
					g.tile2 = tile2;
					g.firstInstance = NoInstance;
					g.lastInstance = NoInstance;
					g.lod = -100; // lod is an int
					g.valid = true;
					groups.push_back(g);

					size_t id = groups.size() - 1;
					FillGroupMap(tile1, tile2, id);
					return id;
				} else {
					// instance is added to an existing group.
					bool extended = false;
					Group &g = groups[foundGroup];
					if (tile1.x < g.tile1.x) {
						extended = true;
						g.tile1.x = tile1.x;
					}
					if (tile1.y < g.tile1.y) {
						extended = true;
						g.tile1.y = tile1.y;
					}
					if (tile2.x > g.tile2.x) {
						extended = true;
						g.tile2.x = tile2.x;
					}
					if (tile2.y > g.tile2.y) {
						extended = true;
						g.tile2.y = tile2.y;
					}

					if (extended) {
						SPAssert(foundGroup != NoGroup);
						for (int x = g.tile1.x; x < g.tile2.x; x++)
							for (int y = g.tile1.y; y < g.tile2.y; y++) {
								size_t g = groupMap[x][y];
								if (g == NoGroup) {
									groupMap[x][y] = foundGroup;
									continue;
								}
								if (g == foundGroup)
									continue;
								// another group found.
								// this should be combined with `foundGroup`.
								CombineGroup(foundGroup, g);
								SPAssert(groupMap[x][y] == foundGroup);
							}
					}

					// don't add instance here

					return foundGroup;
				}
			}

			Internal(GLSparseShadowMapRenderer *r) : renderer(r) {

				GLProfiler::Context profiler(r->GetRenderer()->GetGLProfiler(), "Sparse Page Table Generation");

				cameraShadowCoord = r->GetRenderer()->GetSceneDef().viewOrigin;
				cameraShadowCoord = (r->matrix * cameraShadowCoord).GetXYZ();

				// clear group maps
				for (size_t x = 0; x < Tiles; x++)
					for (size_t y = 0; y < Tiles; y++)
						groupMap[x][y] = NoGroup;

				const std::vector<GLModelRenderer::RenderModel> &rmodels =
				  renderer->GetRenderer()->GetModelRenderer()->models;
				allInstances.reserve(256);
				groups.reserve(64);
				nodes.reserve(256);

				for (std::vector<GLModelRenderer::RenderModel>::const_iterator it = rmodels.begin();
				     it != rmodels.end(); it++) {
					const GLModelRenderer::RenderModel &rmodel = *it;
					Instance inst;

					inst.model = rmodel.model;
					OBB3 modelBounds = inst.model->GetBoundingBox();
					for (size_t i = 0; i < rmodel.params.size(); i++) {
						inst.param = &(rmodel.params[i]);

						if (inst.param->depthHack)
							continue;

						OBB3 instWorldBoundsOBB = inst.param->matrix * modelBounds;
						// w should be 1, so this should wor
						OBB3 instBoundsOBB = r->matrix * instWorldBoundsOBB;
						AABB3 instBounds = instBoundsOBB.GetBoundingAABB();

						// frustrum(?) cull
						if (instBounds.max.x < -1.f || instBounds.max.y < -1.f ||
						    instBounds.min.x > 1.f || instBounds.min.y > 1.f)
							continue;

						inst.tile1 = ShadowMapToTileCoord(instBounds.min);
						inst.tile2 = ShadowMapToTileCoord(instBounds.max);
						inst.tile2.x++;
						inst.tile2.y++;

						if (inst.tile1.x < 0)
							inst.tile1.x = 0;
						if (inst.tile1.y < 0)
							inst.tile1.y = 0;
						if (inst.tile2.x > Tiles)
							inst.tile2.x = Tiles;
						if (inst.tile2.y > Tiles)
							inst.tile2.y = Tiles;

						if (inst.tile1.x >= inst.tile2.x)
							continue;
						if (inst.tile1.y >= inst.tile2.y)
							continue;

						size_t instId = allInstances.size();
						size_t grp = RegisterGroup(inst.tile1, inst.tile2);
						Group &g = groups[grp];
						SPAssert(g.valid);
						if (g.lastInstance == NoInstance) {
							// this is new group.
							g.firstInstance = instId;
							g.lastInstance = instId;
							inst.prev = NoInstance;
							inst.next = NoInstance;
						} else {
							// adding this instance to the group
							Instance &oldLast = allInstances[g.lastInstance];
							SPAssert(oldLast.next == NoInstance);
							oldLast.next = instId;
							inst.prev = g.lastInstance;
							inst.next = NoInstance;
							g.lastInstance = instId;
						}

						g.lod = std::max(g.lod, ComputeLod(instBoundsOBB.m.GetOrigin()));

						allInstances.push_back(inst);
					}
				}

				mapSize = r->settings.r_shadowMapSize;
			}

			bool AddGroupToNode(size_t &nodeRef, int nx, int ny, int nw, int nh, size_t gId) {
				Group &g = groups[gId];
				if (nodeRef == NoNode) {
					// empty node, try putting here
					int w = g.tile2.x - g.tile1.x;
					int h = g.tile2.y - g.tile1.y;
					int lod = g.lod;
					int minLod = renderer->minLod;
					int maxLod = renderer->maxLod;
					lod += lodBias;
					if (lod < minLod)
						lod = minLod;
					if (lod > maxLod)
						lod = maxLod;
					w <<= lod;
					h <<= lod;
					w += 2;
					h += 2; // safety margin
					if (w > nw || h > nh) {
						return false;
					}

					g.rawX = nx + 1;
					g.rawY = ny + 1;
					g.rawW = w - 1;
					g.rawH = h - 1;

					// how fits
					if (w == nw && h == nh) {
						// completely fits
						nodeRef = gId | GroupNodeFlag;
					} else if (w == nw) {
						Node node;
						node.child1 = gId | GroupNodeFlag;
						node.child2 = NoNode;
						node.horizontal = false;
						node.pos = ny + h;
						nodeRef = nodes.size();
						nodes.push_back(node);
					} else if (h == nh) {
						Node node;
						node.child1 = gId | GroupNodeFlag;
						node.child2 = NoNode;
						node.horizontal = true;
						node.pos = nx + w;
						nodeRef = nodes.size();
						nodes.push_back(node);
					} else {
						Node node;
						node.child1 = gId | GroupNodeFlag;
						node.child2 = NoNode;
						node.horizontal = true;
						node.pos = nx + w;
						size_t subnode = nodes.size();
						nodes.push_back(node);

						node.child1 = subnode;
						node.child2 = NoNode;
						node.horizontal = false;
						node.pos = ny + h;
						nodeRef = nodes.size();
						nodes.push_back(node);
					}

					return true;
				} else {
					if (nodeRef & GroupNodeFlag)
						return false;
					Node &node = nodes[nodeRef];
					if (node.horizontal) {
						if (AddGroupToNode(node.child1, nx, ny, node.pos - nx, nh, gId))
							return true;
						if (AddGroupToNode(node.child2, node.pos, ny, nx + nw - node.pos, nh, gId))
							return true;
					} else {
						if (AddGroupToNode(node.child1, nx, ny, nw, node.pos - ny, gId))
							return true;
						if (AddGroupToNode(node.child2, nx, node.pos, nw, ny + nh - node.pos, gId))
							return true;
					}
					return false;
				}
			}

			bool TryPack() {
				size_t rootNode = NoNode;
				nodes.clear();
				for (size_t i = 0; i < groups.size(); i++) {
					if (!AddGroupToNode(rootNode, 0, 0, mapSize, mapSize, i))
						return false;
				}
				return true;
			}

			void Pack() {
				if (groups.empty())
					return;

				GLProfiler::Context profiler(renderer->GetRenderer()->GetGLProfiler(), "Pack [%d group(s)]", (int)groups.size());

				lodBias = 100;
				if (TryPack()) {
					// try to make it more detail
					do {
						lodBias++;
					} while (TryPack() && lodBias < 140);
					lodBias--;
					TryPack();
				} else {
					// try to succeed packing by making it more rough
					do {
						lodBias--;
					} while (!TryPack());
				}
			}
		};

		struct GLSparseShadowMapRenderer::ModelRenderer {
			std::vector<client::ModelRenderParam> params;
			GLModel *lastModel;

			ModelRenderer() {
				params.resize(64);
				lastModel = NULL;
			}

			void Flush() {
				if (lastModel) {
					lastModel->RenderShadowMapPass(params);
					params.clear();
					lastModel = NULL;
				}
			}

			void RenderModel(GLModel *m, const client::ModelRenderParam &param) {
				if (m != lastModel)
					Flush();
				lastModel = m;
				params.push_back(param);
			}
		};

		void GLSparseShadowMapRenderer::RenderShadowMapPass() {
			Internal itnl(this);
			itnl.Pack();

			{
				GLProfiler::Context profiler(GetRenderer()->GetGLProfiler(), "Page Table Generation");
				for (int x = 0; x < Tiles; x++) {
					for (int y = 0; y < Tiles; y++) {
						size_t val = itnl.groupMap[x][y];
						uint32_t out;

						if (val == NoGroup) {
							out = 0xffffffff;
						} else {
							Internal::Group &g = itnl.groups[val];
							int lod = g.lod;
							lod += itnl.lodBias;
							if (lod < minLod)
								lod = minLod;
							if (lod > maxLod)
								lod = maxLod;
							int ix = x - g.tile1.x;
							int iy = y - g.tile1.y;
							ix <<= lod;
							iy <<= lod;
							ix += g.rawX;
							iy += g.rawY;

							unsigned int rr, gg, bb, aa;
							rr = (ix & 255);
							gg = (iy & 255);
							bb = (ix >> 8) | ((iy >> 8) << 4);
							aa = lod; // 1 << (lod - minLod);
							out = bb | (gg << 8) | (rr << 16) | (aa << 24);
						}

						pagetable[y][x] = out;
					}
				}
			}

			{
				GLProfiler::Context profiler(GetRenderer()->GetGLProfiler(), "Page Table Upload");
				device->BindTexture(IGLDevice::Texture2D, pagetableTexture);
				device->TexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, Tiles, Tiles, IGLDevice::BGRA,
				                      IGLDevice::UnsignedByte, pagetable);
			}

			Matrix4 baseMatrix = matrix;
			{
				GLProfiler::Context profiler(GetRenderer()->GetGLProfiler(), "Shadow Maps [%d group(s)]", (int)itnl.groups.size());
				ModelRenderer mrend;
				for (size_t i = 0; i < itnl.groups.size(); i++) {
					Internal::Group &g = itnl.groups[i];
					device->Viewport(g.rawX, g.rawY, g.rawW, g.rawH);

					Vector3 dest1 = Internal::TileToShadowMapCoord(g.tile1);
					Vector3 dest2 = Internal::TileToShadowMapCoord(g.tile2);

					Matrix4 mat;
					mat = Matrix4::Translate((dest1.x + dest2.x) * -.5f, (dest1.y + dest2.y) * -.5f,
					                         0.f);
					mat =
					  Matrix4::Scale(2.f / (dest2.x - dest1.x), 2.f / (dest2.y - dest1.y), 1.f) *
					  mat;
					mat = mat * baseMatrix;

					matrix = mat;

					for (size_t mId = g.firstInstance; mId != NoInstance;
					     mId = itnl.allInstances[mId].next) {
						Internal::Instance &inst = itnl.allInstances[mId];
						mrend.RenderModel(inst.model, *inst.param);

						Vector3 v =
						  inst.model->GetBoundingBox().min + inst.model->GetBoundingBox().max;
						v *= .5f;
						v = (inst.param->matrix * v).GetXYZ();
						{
							v = (baseMatrix * v).GetXYZ();
							SPAssert(v.x >= -1.2f);
							SPAssert(v.y >= -1.2f);
							SPAssert(v.x <= 1.2f);
							SPAssert(v.y <= 1.2f);
						}
					}
					mrend.Flush();
				}
			}

			matrix = baseMatrix;
		}

		bool GLSparseShadowMapRenderer::Cull(const spades::AABB3 &) {

			// TODO: who uses this?
			SPNotImplemented();
			return true;
		}

		bool GLSparseShadowMapRenderer::SphereCull(const spades::Vector3 &center, float rad) {
			return true; // for models, this is already done
			             /*
			             Vector4 vw = matrix * center;
			             float xx = fabsf(vw.x);
			             float yy = fabsf(vw.y);
			             float rx = rad * vpWidth;
			             float ry = rad * vpHeight;

			             return xx < (1.f + rx) && yy < (1.f + ry);*/
		}
	}
}
