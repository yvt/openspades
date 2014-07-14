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
	
} }
