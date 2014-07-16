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

#include <chrono>

#include "SceneRenderer.h"

#include <Client/IModel.h>
#include <bitset>
#include <unordered_map>

namespace spades { namespace editor {
	
#pragma mark - SceneRenderer
	
	SceneRenderer::SceneRenderer(Scene *s,
								 client::IRenderer *r):
	scene(s),
	renderer(r) {
		SPAssert(s);
		SPAssert(r);
		s->AddListener(this);
		for (const auto& rf: s->GetRootFrames())
			RootFrameAdded(rf);
	}
	
	SceneRenderer::~SceneRenderer() {
		scene->RemoveListener(this);
	}
	
	void SceneRenderer::AddToScene(osobj::Pose *pose) {
		// generate custom color
		using namespace std::chrono;
		auto tick = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count() & 4095;
		auto v = static_cast<float>(tick) / 4096.f * M_PI * 2.f;
		Vector3 col;
		col.x = sinf(v);
		col.y = sinf(v + M_PI * 2.f / 3.f);
		col.z = sinf(v + M_PI * 4.f / 3.f);
		col = col * .1f + .5f;
		
		for (const auto& r: frames) {
			r.second->AddToScene(r.first->matrix, pose, col);
		}
	}
	
	void SceneRenderer::RootFrameAdded(RootFrame *r) {
		auto h = MakeHandle<FrameRenderer>(r->frame, renderer);
		frames[r] = h;
	}
	
	void SceneRenderer::RootFrameRemoved(RootFrame *r) {
		frames.erase(r);
	}
	
#pragma mark - FrameRenderer
	
	FrameRenderer::FrameRenderer(osobj::Frame *f,
								 client::IRenderer *r):
	frame(f),
	renderer(r) {
		SPAssert(f);
		SPAssert(r);
		frame->AddListener(this);
		
		for (const auto& f: frame->GetChildren())
			ChildFrameAdded(frame, f);
		for (const auto& o: frame->GetObjects())
			ObjectAdded(frame, o);
	}
	
	FrameRenderer::~FrameRenderer() {
		frame->RemoveListener(this);
	}
	
	void FrameRenderer::AddToScene(const Matrix4 &m, osobj::Pose *pose,
								   const Vector3& customColor) {
		auto mm = m * (pose ? pose->GetTransform(frame) :
					   frame->GetTransform());
	
		for (const auto& o: objects)
			o.second->AddToScene(mm, customColor);
		
		for (const auto& c: children)
			c.second->AddToScene(mm, pose, customColor);
	}
	
	void FrameRenderer::ChildFrameAdded(osobj::Frame *p, osobj::Frame *ch) {
		SPAssert(p == frame);
		children[ch] = MakeHandle<FrameRenderer>(ch, renderer);
	}
	
	void FrameRenderer::ChildFrameRemoved(osobj::Frame *p, osobj::Frame *ch) {
		SPAssert(p == frame);
		children.erase(ch);
	}
	
	void FrameRenderer::ObjectAdded(osobj::Frame *p, osobj::Object *ob) {
		SPAssert(p == frame);
		Handle<ObjectRenderer> r;
		
		auto *vob = dynamic_cast<osobj::VoxelModelObject *>(ob);
		if (vob) {
			r = MakeHandle<VoxelModelObjectRenderer>(vob, renderer);
		}
		
		SPAssert(r);
		
		objects[ob] = r;
	}
	
	void FrameRenderer::ObjectRemoved(osobj::Frame *p, osobj::Object *ob) {
		SPAssert(p == frame);
		objects.erase(ob);
	}
	
#pragma mark - VoxelModelObjectRenderer
	
	VoxelModelObjectRenderer::VoxelModelObjectRenderer
	(osobj::VoxelModelObject *o,
	 client::IRenderer *renderer):
	obj(o), renderer(renderer) {
		SPAssert(o);
		SPAssert(renderer);
	}
	
	VoxelModelObjectRenderer::~VoxelModelObjectRenderer() {
		
	}
	
	void VoxelModelObjectRenderer::VoxelModelUpdated(VoxelModel *v) {
		SPAssert(v == &obj->GetModel());
		
		// model has to be regenerated
		rendererModel = nullptr;
	}
	
	void VoxelModelObjectRenderer::AddToScene(const Matrix4& m,
											  const Vector3& customColor) {
		if (!rendererModel) {
			rendererModel = ToHandle(renderer->CreateModel(&obj->GetModel()));
		}
		
		client::ModelRenderParam param;
		param.matrix = m;
		param.depthHack = false;
		param.customColor = customColor;
		
		renderer->RenderModel(rendererModel, param);
	}
	
#pragma mark - SelectionRenderer
	
	SelectionRenderer::SelectionRenderer(client::IRenderer *r):
	renderer(r) {
		img = ToHandle(r->RegisterImage("Gfx/UI/EditorSelection.png"));
	}
	
	SelectionRenderer::~SelectionRenderer() { }
	
	namespace {
		enum class FaceDir {
			PosX, NegX,
			PosY, NegY,
			PosZ, NegZ
		};
		
		enum class EdgeDir {
			NegU, PosU,
			NegV, PosV
		};
		
		enum class CubeEdgeDir {
			NegXNegY, NegXPosY,
			PosXNegY, PosXPosY,
			NegZNegX, NegZPosX,
			NegZNegY, NegZPosY,
			PosZNegX, PosZPosX,
			PosZNegY, PosZPosY
		};
		
		template <bool neg, EdgeDir edge> struct FaceEdgeCWTmpInner { };
		template <> struct FaceEdgeCWTmpInner<false, EdgeDir::NegU>
		{ static const EdgeDir value = EdgeDir::NegV; };
		template <> struct FaceEdgeCWTmpInner<false, EdgeDir::PosU>
		{ static const EdgeDir value = EdgeDir::PosV; };
		template <> struct FaceEdgeCWTmpInner<false, EdgeDir::NegV>
		{ static const EdgeDir value = EdgeDir::PosU; };
		template <> struct FaceEdgeCWTmpInner<false, EdgeDir::PosV>
		{ static const EdgeDir value = EdgeDir::NegU; };
		template <> struct FaceEdgeCWTmpInner<true, EdgeDir::NegU>
		{ static const EdgeDir value = EdgeDir::PosV; };
		template <> struct FaceEdgeCWTmpInner<true, EdgeDir::PosU>
		{ static const EdgeDir value = EdgeDir::NegV; };
		template <> struct FaceEdgeCWTmpInner<true, EdgeDir::NegV>
		{ static const EdgeDir value = EdgeDir::NegU; };
		template <> struct FaceEdgeCWTmpInner<true, EdgeDir::PosV>
		{ static const EdgeDir value = EdgeDir::PosU; };
		
		// face/edge to cube
		template <FaceDir face, EdgeDir edge> struct FaceEdgeToCubeEdgeTmp { };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegX, EdgeDir::NegU>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegZNegX; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegX, EdgeDir::PosU>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosZNegX; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegX, EdgeDir::NegV>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegXNegY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegX, EdgeDir::PosV>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegXPosY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosX, EdgeDir::NegU>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosZPosX; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosX, EdgeDir::PosU>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegZPosX; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosX, EdgeDir::NegV>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosXPosY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosX, EdgeDir::PosV>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosXNegY; };
		
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegY, EdgeDir::NegU>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosZNegY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegY, EdgeDir::PosU>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegZNegY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegY, EdgeDir::NegV>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosXNegY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegY, EdgeDir::PosV>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegXNegY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosY, EdgeDir::NegU>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegZPosY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosY, EdgeDir::PosU>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosZPosY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosY, EdgeDir::NegV>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegXPosY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosY, EdgeDir::PosV>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosXPosY; };
		
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegZ, EdgeDir::NegU>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegZNegY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegZ, EdgeDir::PosU>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegZPosY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegZ, EdgeDir::NegV>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegZPosX; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::NegZ, EdgeDir::PosV>
		{ static const CubeEdgeDir value = CubeEdgeDir::NegZNegX; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosZ, EdgeDir::NegU>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosZPosY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosZ, EdgeDir::PosU>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosZNegY; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosZ, EdgeDir::NegV>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosZNegX; };
		template <> struct FaceEdgeToCubeEdgeTmp<FaceDir::PosZ, EdgeDir::PosV>
		{ static const CubeEdgeDir value = CubeEdgeDir::PosZPosX; };
		
		template <EdgeDir edge>
		CubeEdgeDir FaceEdgeToCubeEdgeInner(FaceDir face) {
			switch (face) {
				case FaceDir::NegX: return FaceEdgeToCubeEdgeTmp<FaceDir::NegX, edge>::value;
				case FaceDir::PosX: return FaceEdgeToCubeEdgeTmp<FaceDir::PosX, edge>::value;
				case FaceDir::NegY: return FaceEdgeToCubeEdgeTmp<FaceDir::NegY, edge>::value;
				case FaceDir::PosY: return FaceEdgeToCubeEdgeTmp<FaceDir::PosY, edge>::value;
				case FaceDir::NegZ: return FaceEdgeToCubeEdgeTmp<FaceDir::NegZ, edge>::value;
				case FaceDir::PosZ: return FaceEdgeToCubeEdgeTmp<FaceDir::PosZ, edge>::value;
			}
			SPAssert(false);
			return CubeEdgeDir::NegXNegY;
		}
		CubeEdgeDir FaceEdgeToCubeEdge(FaceDir face, EdgeDir edge) {
			switch (edge) {
				case EdgeDir::NegU: return FaceEdgeToCubeEdgeInner<EdgeDir::NegU>(face);
				case EdgeDir::NegV: return FaceEdgeToCubeEdgeInner<EdgeDir::NegV>(face);
				case EdgeDir::PosU: return FaceEdgeToCubeEdgeInner<EdgeDir::PosU>(face);
				case EdgeDir::PosV: return FaceEdgeToCubeEdgeInner<EdgeDir::PosV>(face);
			}
			SPAssert(false);
			return CubeEdgeDir::NegXNegY;
		}
		
		// pos U -> neg V
		template <FaceDir> struct IsNegFaceTmp { static const bool value = false; };
		template <> struct IsNegFaceTmp<FaceDir::NegX> { static const bool value = true; };
		template <> struct IsNegFaceTmp<FaceDir::PosY> { static const bool value = true; };
		template <> struct IsNegFaceTmp<FaceDir::NegZ> { static const bool value = true; };
		
		template <FaceDir face, EdgeDir edge> struct FaceEdgeCWTmp:
		FaceEdgeCWTmpInner<IsNegFaceTmp<face>::value, edge> { };
		
		/** Finds edge at the specified direction
		 * FIXME: this looks inverted */
		template <FaceDir face, FaceDir vec> struct FaceVecToEdge { };
		template <FaceDir _> struct FaceVecToEdge<FaceDir::PosZ, _>
		{ static const EdgeDir value = EdgeDir::NegV;  };
		template <FaceDir _> struct FaceVecToEdge<FaceDir::NegZ, _>
		{ static const EdgeDir value = EdgeDir::PosV;  };
		
		template <FaceDir _> struct FaceVecToEdge<FaceDir::PosX, _>
		{ static const EdgeDir value = EdgeDir::NegU;  };
		template <FaceDir _> struct FaceVecToEdge<FaceDir::NegX, _>
		{ static const EdgeDir value = EdgeDir::PosU;  };
		
		template <> struct FaceVecToEdge<FaceDir::PosY, FaceDir::PosX>
		{ static const EdgeDir value = EdgeDir::NegU;  };
		template <> struct FaceVecToEdge<FaceDir::PosY, FaceDir::NegX>
		{ static const EdgeDir value = EdgeDir::NegU;  };
		template <> struct FaceVecToEdge<FaceDir::PosY, FaceDir::PosZ>
		{ static const EdgeDir value = EdgeDir::NegV;  };
		template <> struct FaceVecToEdge<FaceDir::PosY, FaceDir::NegZ>
		{ static const EdgeDir value = EdgeDir::NegV;  };
		template <> struct FaceVecToEdge<FaceDir::NegY, FaceDir::PosX>
		{ static const EdgeDir value = EdgeDir::PosU;  };
		template <> struct FaceVecToEdge<FaceDir::NegY, FaceDir::NegX>
		{ static const EdgeDir value = EdgeDir::PosU;  };
		template <> struct FaceVecToEdge<FaceDir::NegY, FaceDir::PosZ>
		{ static const EdgeDir value = EdgeDir::PosV;  };
		template <> struct FaceVecToEdge<FaceDir::NegY, FaceDir::NegZ>
		{ static const EdgeDir value = EdgeDir::PosV;  };
		
		/** Returns face that connects to the specified edge */
		template <FaceDir, EdgeDir> struct FaceUVFaceTmp { };
		template <> struct FaceUVFaceTmp<FaceDir::PosX, EdgeDir::NegU>
		{ static const FaceDir value = FaceDir::PosZ; };
		template <> struct FaceUVFaceTmp<FaceDir::PosX, EdgeDir::PosU>
		{ static const FaceDir value = FaceDir::NegZ; };
		template <> struct FaceUVFaceTmp<FaceDir::PosX, EdgeDir::NegV>
		{ static const FaceDir value = FaceDir::NegY; };
		template <> struct FaceUVFaceTmp<FaceDir::PosX, EdgeDir::PosV>
		{ static const FaceDir value = FaceDir::PosY; };
		template <> struct FaceUVFaceTmp<FaceDir::NegX, EdgeDir::NegU>
		{ static const FaceDir value = FaceDir::NegZ; };
		template <> struct FaceUVFaceTmp<FaceDir::NegX, EdgeDir::PosU>
		{ static const FaceDir value = FaceDir::PosZ; };
		template <> struct FaceUVFaceTmp<FaceDir::NegX, EdgeDir::NegV>
		{ static const FaceDir value = FaceDir::PosY; };
		template <> struct FaceUVFaceTmp<FaceDir::NegX, EdgeDir::PosV>
		{ static const FaceDir value = FaceDir::NegY; };
		
		template <> struct FaceUVFaceTmp<FaceDir::PosY, EdgeDir::NegU>
		{ static const FaceDir value = FaceDir::NegZ; };
		template <> struct FaceUVFaceTmp<FaceDir::PosY, EdgeDir::PosU>
		{ static const FaceDir value = FaceDir::PosZ; };
		template <> struct FaceUVFaceTmp<FaceDir::PosY, EdgeDir::NegV>
		{ static const FaceDir value = FaceDir::PosX; };
		template <> struct FaceUVFaceTmp<FaceDir::PosY, EdgeDir::PosV>
		{ static const FaceDir value = FaceDir::NegX; };
		template <> struct FaceUVFaceTmp<FaceDir::NegY, EdgeDir::NegU>
		{ static const FaceDir value = FaceDir::PosZ; };
		template <> struct FaceUVFaceTmp<FaceDir::NegY, EdgeDir::PosU>
		{ static const FaceDir value = FaceDir::NegZ; };
		template <> struct FaceUVFaceTmp<FaceDir::NegY, EdgeDir::NegV>
		{ static const FaceDir value = FaceDir::NegX; };
		template <> struct FaceUVFaceTmp<FaceDir::NegY, EdgeDir::PosV>
		{ static const FaceDir value = FaceDir::PosX; };
		
		template <> struct FaceUVFaceTmp<FaceDir::PosZ, EdgeDir::NegU>
		{ static const FaceDir value = FaceDir::PosY; };
		template <> struct FaceUVFaceTmp<FaceDir::PosZ, EdgeDir::PosU>
		{ static const FaceDir value = FaceDir::NegY; };
		template <> struct FaceUVFaceTmp<FaceDir::PosZ, EdgeDir::NegV>
		{ static const FaceDir value = FaceDir::NegX; };
		template <> struct FaceUVFaceTmp<FaceDir::PosZ, EdgeDir::PosV>
		{ static const FaceDir value = FaceDir::PosX; };
		template <> struct FaceUVFaceTmp<FaceDir::NegZ, EdgeDir::NegU>
		{ static const FaceDir value = FaceDir::NegY; };
		template <> struct FaceUVFaceTmp<FaceDir::NegZ, EdgeDir::PosU>
		{ static const FaceDir value = FaceDir::PosY; };
		template <> struct FaceUVFaceTmp<FaceDir::NegZ, EdgeDir::NegV>
		{ static const FaceDir value = FaceDir::PosX; };
		template <> struct FaceUVFaceTmp<FaceDir::NegZ, EdgeDir::PosV>
		{ static const FaceDir value = FaceDir::NegX; };
		
		/** Flips the face */
		template <FaceDir> struct FlipFaceDir { };
		template <> struct FlipFaceDir<FaceDir::PosX> { static const FaceDir value = FaceDir::NegX; };
		template <> struct FlipFaceDir<FaceDir::NegX> { static const FaceDir value = FaceDir::PosX; };
		template <> struct FlipFaceDir<FaceDir::PosY> { static const FaceDir value = FaceDir::NegY; };
		template <> struct FlipFaceDir<FaceDir::NegY> { static const FaceDir value = FaceDir::PosY; };
		template <> struct FlipFaceDir<FaceDir::PosZ> { static const FaceDir value = FaceDir::NegZ; };
		template <> struct FlipFaceDir<FaceDir::NegZ> { static const FaceDir value = FaceDir::PosZ; };
		
		/** Retruns the normal of the face */
		template <FaceDir> struct FaceNormalTmp { };
		template <> struct FaceNormalTmp<FaceDir::PosX> { static IntVector3 value() { return IntVector3(1, 0, 0); } };
		template <> struct FaceNormalTmp<FaceDir::NegX> { static IntVector3 value() { return IntVector3(-1, 0, 0); } };
		template <> struct FaceNormalTmp<FaceDir::PosY> { static IntVector3 value() { return IntVector3(0, 1, 0); } };
		template <> struct FaceNormalTmp<FaceDir::NegY> { static IntVector3 value() { return IntVector3(0, -1, 0); } };
		template <> struct FaceNormalTmp<FaceDir::PosZ> { static IntVector3 value() { return IntVector3(0, 0, 1); } };
		template <> struct FaceNormalTmp<FaceDir::NegZ> { static IntVector3 value() { return IntVector3(0, 0, -1); } };
		
		/** Retruns the position of the face */
		template <FaceDir> struct FacePosTmp { };
		template <> struct FacePosTmp<FaceDir::PosX> { static IntVector3 value() { return IntVector3(1, 0, 0); } };
		template <> struct FacePosTmp<FaceDir::NegX> { static IntVector3 value() { return IntVector3(0, 0, 0); } };
		template <> struct FacePosTmp<FaceDir::PosY> { static IntVector3 value() { return IntVector3(0, 1, 0); } };
		template <> struct FacePosTmp<FaceDir::NegY> { static IntVector3 value() { return IntVector3(0, 0, 0); } };
		template <> struct FacePosTmp<FaceDir::PosZ> { static IntVector3 value() { return IntVector3(0, 0, 1); } };
		template <> struct FacePosTmp<FaceDir::NegZ> { static IntVector3 value() { return IntVector3(0, 0, 0); } };
		
		/** Returns location of edge relative to the center of the face */
		template <FaceDir face, EdgeDir edge> struct FaceEdgePosTmp:
		public FlipFaceDir<FaceUVFaceTmp<face, edge>::value> {};
		
		/** Returns start point of edge */
		template <FaceDir face, EdgeDir edge> struct FaceEdgeStartPosTmp {
			static IntVector3 value() {
				auto v = FacePosTmp<face>::value();
				v += FacePosTmp<FaceUVFaceTmp<face, edge>::value>::value();
				v += FacePosTmp<FaceEdgePosTmp<face, FaceEdgeCWTmp<face, edge>::value>::value>::value();
				return v;
			}
		};;
		
		static inline IntVector3 FaceNormal(FaceDir face) {
			switch (face) {
				case FaceDir::PosX: return FaceNormalTmp<FaceDir::PosX>::value();
				case FaceDir::NegX: return FaceNormalTmp<FaceDir::NegX>::value();
				case FaceDir::PosY: return FaceNormalTmp<FaceDir::PosY>::value();
				case FaceDir::NegY: return FaceNormalTmp<FaceDir::NegY>::value();
				case FaceDir::PosZ: return FaceNormalTmp<FaceDir::PosZ>::value();
				case FaceDir::NegZ: return FaceNormalTmp<FaceDir::NegZ>::value();
			}
			return IntVector3(0, 0, 0);
		}
		
		template <FaceDir f, EdgeDir e> struct FaceUVTmp {
			static IntVector3 value() { return FaceNormalTmp<FaceUVFaceTmp<f, e>::value>::value(); }
		};
		
	}
	
	class SelectionRenderer::EdgeDetector {
		
		VoxelModel& model;
		IntVector3 const camera;
		std::vector<bool> visited;
		std::vector<IntVector3> edgeBuffer;
		
		bool IsFaceVisible(const IntVector3& p,
						   FaceDir f) {
			switch (f) {
				case FaceDir::PosX: return camera.x > p.x;
				case FaceDir::NegX: return camera.x < p.x;
				case FaceDir::PosY: return camera.y > p.y;
				case FaceDir::NegY: return camera.y < p.y;
				case FaceDir::PosZ: return camera.z > p.z;
				case FaceDir::NegZ: return camera.z < p.z;
				default:
					SPAssert(false);
					return false;
			}
		}
		
		auto HasVisited(const IntVector3& p,
						FaceDir f, EdgeDir e) -> decltype(visited)::reference {
			auto edge = FaceEdgeToCubeEdge(f, e);
			return visited[(((p.x * model.GetHeight()) + p.y)
							* model.GetDepth() + p.z) * 12 +
						   static_cast<unsigned int>(edge)];
		}
		
		IntVector3 currentVoxel;
		FaceDir currentFace;
		EdgeDir currentEdge;
		
		template <FaceDir face, EdgeDir edge>
		void AddPointInner() {
			auto v = FaceEdgeStartPosTmp<face, edge>::value();
			v += currentVoxel;
			if (!edgeBuffer.empty() &&
				v == edgeBuffer.back())
				return;
			edgeBuffer.push_back(v);
			
			
			SPAssert(model.IsSolid(currentVoxel.x, currentVoxel.y, currentVoxel.z));
			SPAssert({
				auto norm = FaceNormalTmp<face>::value();
				auto vfront = currentVoxel + norm; !model.IsSolid(vfront.x, vfront.y, vfront.z);
			});
			
		}
		template <FaceDir face>
		void AddPointInner2() {
			switch (currentEdge) {
				case EdgeDir::NegU: AddPointInner<face, EdgeDir::NegU>(); break;
				case EdgeDir::PosU: AddPointInner<face, EdgeDir::PosU>(); break;
				case EdgeDir::NegV: AddPointInner<face, EdgeDir::NegV>(); break;
				case EdgeDir::PosV: AddPointInner<face, EdgeDir::PosV>(); break;
			}
		}
		void AddPoint() {
			switch (currentFace) {
				case FaceDir::NegX: AddPointInner2<FaceDir::NegX>(); break;
				case FaceDir::PosX: AddPointInner2<FaceDir::PosX>(); break;
				case FaceDir::NegY: AddPointInner2<FaceDir::NegY>(); break;
				case FaceDir::PosY: AddPointInner2<FaceDir::PosY>(); break;
				case FaceDir::NegZ: AddPointInner2<FaceDir::NegZ>(); break;
				case FaceDir::PosZ: AddPointInner2<FaceDir::PosZ>(); break;
			}
		}
		
		template <FaceDir face, EdgeDir edge>
		void CheckFaceEdge() {
			auto side = FaceUVTmp<face, edge>::value();
			auto norm = FaceNormalTmp<face>::value();
			auto v2 = currentVoxel + side;
			auto v1 = v2 + norm;
			
			SPAssert(model.IsSolid(currentVoxel.x, currentVoxel.y, currentVoxel.z));
			SPAssert({ auto vfront = currentVoxel + norm; !model.IsSolid(vfront.x, vfront.y, vfront.z); });
			
			if (model.IsSolid(v1.x, v1.y, v1.z)) {
				if (IsFaceVisible(v1, FlipFaceDir<FaceUVFaceTmp<face, edge>::value>::value)) {
					currentVoxel = v1;
					using nextFace = FlipFaceDir<FaceUVFaceTmp<face, edge>::value>;
					using nextEdge = FaceVecToEdge<FlipFaceDir<face>::value, nextFace::value>;
					currentFace = nextFace::value;
					currentEdge = nextEdge::value;
					return AddPoint();
				}
			} else if (!model.IsSolid(v2.x, v2.y, v2.z)) {
				if (IsFaceVisible(currentVoxel, FaceUVFaceTmp<face, edge>::value)) {
					using nextFace = FaceUVFaceTmp<face, edge>;
					using nextEdge = FaceVecToEdge<face, nextFace::value>;
					currentFace = nextFace::value;
					currentEdge = nextEdge::value;
					return AddPoint();
				}
			} else {
				// turn left
				currentVoxel = v2;
				currentEdge = FaceEdgeCWTmp<FlipFaceDir<face>::value, edge>::value;
				return AddPoint();
			}
			
			// just turn right
			HasVisited(currentVoxel, currentFace, currentEdge) = true;
			currentEdge = FaceEdgeCWTmp<face, edge>::value;
			return AddPoint();
		}
		template <FaceDir face>
		void CheckFace() {
			switch (currentEdge) {
				case EdgeDir::NegU: CheckFaceEdge<face, EdgeDir::NegU>(); break;
				case EdgeDir::PosU: CheckFaceEdge<face, EdgeDir::PosU>(); break;
				case EdgeDir::NegV: CheckFaceEdge<face, EdgeDir::NegV>(); break;
				case EdgeDir::PosV: CheckFaceEdge<face, EdgeDir::PosV>(); break;
			}
		}
		
		void EdgeScan(const IntVector3& startVoxel,
					  FaceDir startFace, EdgeDir startEdge) {
			edgeBuffer.clear();
			if (HasVisited(startVoxel, startFace, startEdge)) return;
			
			currentVoxel = startVoxel;
			currentFace = startFace;
			currentEdge = startEdge;
			AddPoint();
			
			int steps = 30000;
			
			while ((steps--) > 0 && edgeBuffer.size() < 20000) {
				//SPAssert(steps > 0);
				switch (currentFace) {
					case FaceDir::NegX: CheckFace<FaceDir::NegX>(); break;
					case FaceDir::PosX: CheckFace<FaceDir::PosX>(); break;
					case FaceDir::NegY: CheckFace<FaceDir::NegY>(); break;
					case FaceDir::PosY: CheckFace<FaceDir::PosY>(); break;
					case FaceDir::NegZ: CheckFace<FaceDir::NegZ>(); break;
					case FaceDir::PosZ: CheckFace<FaceDir::PosZ>(); break;
				}
				
				HasVisited(currentVoxel, currentFace, currentEdge) = true;
				
				if (currentVoxel == startVoxel &&
					FaceEdgeToCubeEdge(startFace, startEdge) ==
					FaceEdgeToCubeEdge(currentFace, currentEdge)) {
					// loop ended
					AddPoint();
					break;
				}
				if (currentVoxel == startVoxel) {
					//steps --;
				}
			}
			AddPoint();
		}
		
		template <class F, FaceDir face, EdgeDir edge>
		inline void RootFaceEdge(const IntVector3& voxel,
								 F callback) {
			auto side = FaceUVTmp<face, edge>::value();
			auto norm = FaceNormalTmp<face>::value();
			auto v2 = voxel + side;
			auto v1 = v2 + norm;
			
			// start scanning at seams
			if (model.IsSolid(v1.x, v1.y, v1.z)) {
				if (!IsFaceVisible(v1, FlipFaceDir<FaceUVFaceTmp<face, edge>::value>::value)) {
					EdgeScan(voxel, face, edge);
					if(!edgeBuffer.empty()) callback(edgeBuffer);
				}
			} else if (!model.IsSolid(v2.x, v2.y, v2.z)) {
				if (!IsFaceVisible(IntVector3(voxel.x, voxel.y, voxel.z), FaceUVFaceTmp<face, edge>::value)) {
					EdgeScan(voxel, face, edge);
					if(!edgeBuffer.empty()) callback(edgeBuffer);
				}
			}
		}
		
		template <class F, FaceDir face>
		inline void RootFace(const IntVector3& voxel,
					  F callback) {
			RootFaceEdge<F, face, EdgeDir::NegU>(voxel, callback);
			RootFaceEdge<F, face, EdgeDir::PosU>(voxel, callback);
			RootFaceEdge<F, face, EdgeDir::NegV>(voxel, callback);
			RootFaceEdge<F, face, EdgeDir::PosV>(voxel, callback);
		}
		
		template <class F>
		void Root(const IntVector3& voxel,
				  FaceDir face,
				  F callback) {
			if (!IsFaceVisible(voxel, face)) return;
			
			switch (face) {
				case FaceDir::PosX: RootFace<F, FaceDir::PosX>(voxel, callback); break;
				case FaceDir::NegX: RootFace<F, FaceDir::NegX>(voxel, callback); break;
				case FaceDir::PosY: RootFace<F, FaceDir::PosY>(voxel, callback); break;
				case FaceDir::NegY: RootFace<F, FaceDir::NegY>(voxel, callback); break;
				case FaceDir::PosZ: RootFace<F, FaceDir::PosZ>(voxel, callback); break;
				case FaceDir::NegZ: RootFace<F, FaceDir::NegZ>(voxel, callback); break;
				default: SPAssert(false);
			}
			
		}
		
		template <class F>
		void ForEachSetBit(uint64_t i, F callback) {
			int j = 0;
			while (i) {
				if (i & 1) {
					callback(j);
				}
				i >>= 1; ++j;
			}
		}
		
	public:
		
		EdgeDetector(VoxelModel& model,
					 const Vector3& camOrigin):
		model(model),
		camera(camOrigin.Floor()) {
			visited.resize(model.GetWidth() *
						   model.GetHeight() *
						   model.GetDepth() * 12);
		}
		
		template <class F>
		void Scan(F callback) {
			for (int x = 0; x < model.GetWidth(); ++x) {
				for (int y = 0; y < model.GetHeight(); ++y) {
					auto mp = model.GetSolidBitsAt(x, y);
					auto m1 = x == 0 ? 0 : model.GetSolidBitsAt(x - 1, y);
					auto m2 = x == model.GetWidth() - 1 ? 0 : model.GetSolidBitsAt(x + 1, y);
					auto m3 = y == 0 ? 0 : model.GetSolidBitsAt(x, y - 1);
					auto m4 = y == model.GetHeight() - 1 ? 0 : model.GetSolidBitsAt(x, y + 1);
					
					ForEachSetBit(mp & ~(mp << 1), [&](int z) {
						Root(IntVector3(x, y, z), FaceDir::NegZ, callback);
					});
					ForEachSetBit(mp & ~(mp >> 1), [&](int z) {
						Root(IntVector3(x, y, z), FaceDir::PosZ, callback);
					});
					ForEachSetBit(mp & ~m1, [&](int z) {
						Root(IntVector3(x, y, z), FaceDir::NegX, callback);
					});
					ForEachSetBit(mp & ~m2, [&](int z) {
						Root(IntVector3(x, y, z), FaceDir::PosX, callback);
					});
					ForEachSetBit(mp & ~m3, [&](int z) {
						Root(IntVector3(x, y, z), FaceDir::NegY, callback);
					});
					ForEachSetBit(mp & ~m4, [&](int z) {
						Root(IntVector3(x, y, z), FaceDir::PosY, callback);
					});
				}
			}
		}
		
	};
	
	void SelectionRenderer::SetSceneDefiniton
	(const client::SceneDefinition &def) {
		sceneDef = def;
		
		viewScale.x = renderer->ScreenWidth() * .5f / tanf(def.fovX * .5f);
		viewScale.y = -renderer->ScreenHeight() * .5f  / tanf(def.fovY * .5f);
		viewScale.z = 0.f;
		
		viewOffset.x = renderer->ScreenWidth() * .5f;
		viewOffset.y = renderer->ScreenHeight() * .5f;
		viewOffset.z = 0.f;
		
	}
	
	void SelectionRenderer::RenderSelection(VoxelModel &vm,
											const Matrix4 &mm) {
		auto m = mm * Matrix4::Translate(vm.GetOrigin() - .5f);
		auto invM = m.InversedFast();
		auto localCamOrigin = invM * sceneDef.viewOrigin;
		EdgeDetector d(vm, localCamOrigin.GetXYZ());
		
		std::vector<Vector3> vertsf;
		std::vector<Vector3> vertsc;
		std::vector<Vector2> sides;
		
		
		
		d.Scan([&](std::vector<IntVector3>& verts) {
			if (verts.front() == verts.back()) verts.pop_back();
			if (verts.size() <= 1) return;
			
			// Z-plane clip
			vertsf.clear();
			vertsc.clear();
			for (const auto& v: verts) {
				Vector3 vv(v.x, v.y, v.z);
				vertsf.push_back((m * vv).GetXYZ());
			}
			{
				auto origin = sceneDef.viewOrigin;
				auto dir = sceneDef.viewAxis[2];
				auto offs = Vector3::Dot(origin, dir);
				const float zNear = .01f;
				auto dist = [=](const Vector3& v) {
					return Vector3::Dot(v, dir) - offs - zNear;
				};
				std::size_t firstIndex = 0;
				while (firstIndex < vertsf.size()) {
					if (dist(vertsf[firstIndex]) >= 0.f) {
						break;
					}
					++firstIndex;
				}
				if (firstIndex == vertsf.size()) {
					// culled
					return;
				}
				
				std::size_t i = firstIndex;
				bool lastVisible = true;
				do {
					std::size_t next = i + 1;
					if (next == vertsf.size()) next = 0;
					auto nextDist = dist(vertsf[next]);
					if (lastVisible) {
						if (nextDist >= 0.f) {
							vertsc.push_back(vertsf[next]);
						} else {
							auto last = vertsf[i];
							auto lastDist = dist(last);
							auto per = lastDist / (lastDist - nextDist);
							vertsc.push_back(Mix(last, vertsf[next], per));
							lastVisible = false;
						}
					} else if (nextDist >= 0.f) {
						auto last = vertsf[i];
						auto lastDist = dist(last);
						auto per = lastDist / (lastDist - nextDist);
						vertsc.push_back(Mix(last, vertsf[next], per));
						vertsc.push_back(vertsf[next]);
						lastVisible = true;
					}
					i = next;
				} while (i != firstIndex);
			}
			
			// transform
			for (auto& v: vertsc) {
				auto vv = v - sceneDef.viewOrigin;
				v.x = Vector3::Dot(vv, sceneDef.viewAxis[0]);
				v.y = Vector3::Dot(vv, sceneDef.viewAxis[1]);
				v.z = Vector3::Dot(vv, sceneDef.viewAxis[2]);
				v *= 1.f / v.z;
				v = v * viewScale + viewOffset;
			}
			
			// render
			sides.resize(vertsc.size());
			for (std::size_t i = 0; i < vertsc.size(); ++i) {
				auto vv1 = vertsc[i == 0 ? vertsc.size() - 1 : i - 1];
				auto vv2 = vertsc[i];
				Vector2 v1(vv1.x, vv1.y), v2(vv2.x, vv2.y);
				auto dir = v2 - v1;
				if (dir.GetPoweredLength() < .00000001f) continue;
				dir = dir.Normalize();
				Vector2 side(-dir.y, dir.x);
				const float len = 16.f;
				sides[i] = side * len;
			}
			for (std::size_t i = 0; i < vertsc.size(); ++i) {
				auto vv1 = vertsc[i == 0 ? vertsc.size() - 1 : i - 1];
				auto vv2 = vertsc[i];
				Vector2 v1(vv1.x, vv1.y), v2(vv2.x, vv2.y);
				const auto& side = sides[i];
				renderer->DrawImage(img,
									v1 + side, v2 + side, v1,
									AABB2(32, 16, 0, 16));
				const auto& prevSide = sides[i == 0 ? vertsc.size() - 1 : i - 1];
				if (Vector2::Dot(v2 - v1, prevSide) <= 0.f) {
					Vector2 center(32, 32);
					if (Vector2::Dot(side, prevSide) > 0.f) {
						renderer->DrawImage(img,
											v1, v1 + side, v1 + prevSide,
											center, center + side, center + prevSide);
					} else {
						Vector2 midSide(-side.y, side.x);
						renderer->DrawImage(img,
											v1, v1 + side, v1 + midSide,
											center, center + side, center + midSide);
						renderer->DrawImage(img,
											v1, v1 + midSide, v1 + prevSide,
											center, center + midSide, center + prevSide);
					}
				}
			}
			
		});
	}
	
} }
