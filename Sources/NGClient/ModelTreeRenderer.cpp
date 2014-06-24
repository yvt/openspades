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

#include "ModelTreeRenderer.h"

namespace spades { namespace ngclient {
	
	class VoxelModelRenderer: public ObjectRenderer {
		Handle<osobj::VoxelModelObject> obj;
		Handle<client::IModel> model;
		client::IRenderer& r;
	protected:
		~VoxelModelRenderer() { }
	public:
		VoxelModelRenderer(client::IRenderer& r,
						   osobj::VoxelModelObject& o):
		obj(&o), r(r) {
			model.Set(r.CreateModel(&o.GetModel()), false);
		}
		void AddToScene(const Matrix4& m,
						const ModelTreeRenderParam& p) override {
			client::ModelRenderParam param;
			param.customColor = p.customColor;
			param.matrix = m;
			param.depthHack = p.depthHack;
			r.RenderModel(model, param);
		}
	};
	
	ModelTreeRenderer::ModelTreeRenderer
	(client::IRenderer *renderer,
	 osobj::Frame *root):
	renderer(renderer),
	root(root) {
		SPAssert(renderer);
		SPAssert(root);
		Init(*root);
	}
	
	void ModelTreeRenderer::Init(osobj::Frame &f) {
		for (const auto& o: f.GetObjects()) {
			auto& r = dynamic_cast<osobj::VoxelModelObject&>(*o);
			objectRenderers.emplace
			(o,
			 Handle<ObjectRenderer>
			 (new VoxelModelRenderer(*renderer,	r),
			  false));
		}
		for (const auto& c: f.GetChildren()) {
			Init(*c);
		}
	}
	
	ModelTreeRenderer::~ModelTreeRenderer() {
		
	}
	
	void ModelTreeRenderer::AddToScene(osobj::Pose &pose,
									   const ModelTreeRenderParam& params) {
		AddToScene(pose, params, *root,
				   Matrix4::Identity());
	}
	
	void ModelTreeRenderer::AddToScene(osobj::Pose &pose,
									   const ModelTreeRenderParam& params,
									   osobj::Frame &f,
									   const Matrix4& m) {
		auto mm = m * pose.GetTransform(&f);
		for (const auto& o: f.GetObjects()) {
			objectRenderers[o]->AddToScene(mm,
										   params);
		}
		for (const auto& c: f.GetChildren()) {
			AddToScene(pose, params, *c, mm);
		}
	}
	
} }



