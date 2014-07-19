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

#include "ObjectMode.h"
#include "UIElement.h"
#include "Editor.h"
#include "MainView.h"
#include "Scene.h"
#include "Commands.h"
#include <unordered_set>
#include <Core/Strings.h>

namespace spades { namespace editor {
	
#pragma mark - ObjectSelectionMode
	
	ObjectSelectionMode::ObjectSelectionMode() { }
	ObjectSelectionMode::~ObjectSelectionMode() { }
	
	void ObjectSelectionMode::Enter(Editor *e) {
		EditorMode::Enter(e);
	}
	void ObjectSelectionMode::Leave(Editor *e) {
		EditorMode::Leave(e);
	}
	
	class ObjectSelectionView: public ModeView {
		Editor& editor;
	protected:
		void OnMouseDown(MouseButton mb, const Vector2& v) override {
			if (mb == MouseButton::Left) {
				auto dir = editor.Unproject(v);
				auto *sc = editor.GetScene();
				if (!sc) return;
				
				auto ret = sc->CastRay(editor.GetLastSceneDefinition().viewOrigin,
									   dir, editor.GetPose());
				
				if (ret) {
					auto& r = *ret;
					auto shift = GetManager().GetKeyModifierState(KeyModifier::Shift);
					if (shift && editor.IsSelected(r.frame)) {
						editor.Deselect(r.frame);
					} else {
						editor.Select(r.frame,
									  shift);
					}
				}
				
				return;
			}
			ModeView::OnMouseDown(mb, v);
		}
		void OnKeyDown(const std::string& key) override {
			if (key == "G" &&
				!editor.GetSelectedFrames().empty()) {
				editor.SetMode(MakeHandle<TranslateFrameMode>());
			} else if (key == "R" &&
					   !editor.GetSelectedFrames().empty()) {
				editor.SetMode(MakeHandle<RotateFrameMode>());
			}
			ModeView::OnKeyDown(key);
		}
	public:
		ObjectSelectionView(UIManager *m, Editor *e):
		ModeView(m, e), editor(*e) {
		}
		
		
	};
	
	
	UIElement *ObjectSelectionMode::CreateView
	(UIManager *m, Editor *e) {
		return new ObjectSelectionView(m, e);
	}
	
	
#pragma mark - FrameTransformAbstractMode
	
	class UpdateFrameTransformCommand: public Command {
		Editor *editor;
		Handle<osobj::Frame> frame;
		Matrix4 oldTransform;
		Matrix4 newTransform;
		Matrix4 oldWorkingPose;
	protected:
		~UpdateFrameTransformCommand() {}
	public:
		UpdateFrameTransformCommand
		(osobj::Frame *frame,
		 Editor *editor,
		 const Matrix4& newTransform,
		 const Matrix4& oldWorkingPose):
		editor(editor), frame(frame),
		oldTransform(frame->GetTransform()),
		newTransform(newTransform),
		oldWorkingPose(editor->GetPose()->GetTransform(frame)) { }
		void Perform() override {
			frame->SetTransform(newTransform);
			editor->GetPose()->SetTransform(frame, newTransform);
		}
		void Revert() override {
			frame->SetTransform(oldTransform);
			editor->GetPose()->SetTransform(frame, oldWorkingPose);
		}
		std::string GetName() override {
			return _Tr("Editor", "Transform {0}", frame->GetFullPath());
		}
	};
	
	FrameTransformAbstractMode::FrameTransformAbstractMode() { }
	FrameTransformAbstractMode::~FrameTransformAbstractMode() { }
	
	Matrix4 FrameTransformAbstractMode::GetOriginalTransform(osobj::Frame *f) {
		auto it = originalTransform.find(f);
		if (it != originalTransform.end()) {
			return it->second;
		}
		SPAssert(editor);
		SPAssert(editor->GetPose());
		return editor->GetPose()->GetTransform(f);
	}
	
	Matrix4 FrameTransformAbstractMode::GetGlobalOriginalTransform(osobj::Frame *f) {
		auto m = GetOriginalTransform(f);
		auto *root = f;
		for (auto *p = f->GetParent(); p;
			 p = p->GetParent()) {
			m = GetOriginalTransform(p) * m;
			root = p;
		}
		for (const auto& rf: editor->GetScene()->GetRootFrames()) {
			if (rf->frame == root) {
				m = rf->matrix * m;
			}
		}
		return m;
	}
	
	void FrameTransformAbstractMode::Enter(Editor *e) {
		SPAssert(!editor);
		editor = e;
		
		// compute target frames
		targetFrames.clear();
		std::unordered_set<osobj::Frame *> selection;
		for (const auto& sel: e->GetSelectedFrames()) {
			selection.insert(sel);
		}
		std::function<void(osobj::Frame&)> scan =
		[&](osobj::Frame& f) {
			if (selection.find(&f) != selection.end()) {
				// found
				targetFrames.push_back(&f);
				return;
			}
			for (const auto& ch: f.GetChildren()) {
				scan(*ch);
			}
		};
		for (const auto& rf: e->GetScene()->GetRootFrames()) {
			scan(*rf->frame);
		}
		
		affectedFrames = ComputeAffectedFrames();
		auto *pose = editor->GetPose();
		for (auto *f: affectedFrames) {
			originalTransform.emplace(f,
									  pose->GetTransform(f));
		}
		
		
		EditorMode::Enter(e);
	}
	
	void FrameTransformAbstractMode::Leave(Editor *e) {
		SPAssert(editor == e);
		
		ForceCancel();
		
		editor = nullptr;
		EditorMode::Leave(e);
	}
	
	void FrameTransformAbstractMode::ActiveObjectChanged(osobj::Object *o) {
		// entering object editing mode?
		if (o) ForceCancel();
	}
	
	void FrameTransformAbstractMode::ActiveTimelineChanged(spades::editor::TimelineItem *tl) {
		// changing timeline
		ForceCancel();
	}
	
	void FrameTransformAbstractMode::SelectedFramesChanged() {
		ForceCancel();
	}
	
	void FrameTransformAbstractMode::ForceCancel() {
		Cancel();
	}
	
	void FrameTransformAbstractMode::Apply() {
		if (done) return;
		
		if (!editor->GetActiveTimeline()) {
			std::vector<Handle<Command>> cmds;
			auto *pose = editor->GetPose();
			SPAssert(pose);
			for (const auto& pair: originalTransform) {
				cmds.push_back(ToHandle<Command>
							   (new UpdateFrameTransformCommand
							   (pair.first, editor,
								pose->GetTransform(pair.first),
								pair.second)));
			}
			
			// build name
			std::string name;
			if (affectedFrames.size() == 0) {
				// ???
				done = true;
				return;
			} else if (affectedFrames.size() == 1) {
				name = cmds[0]->GetName();
			} else {
				name = _Tr("Editor", "Transform {0}, etc",
						   affectedFrames[0]->GetFullPath());
			}
			
			auto cm = MakeHandle<CompoundCommand>(name,
												  std::move(cmds));
			
			editor->GetCommandManager().Perform(cm);
		} else {
			// in animation mode, use will manually add
			// keyframe
		}
		
		done = true;
		editor->SetMode(MakeHandle<ObjectSelectionMode>());
	}
	
	void FrameTransformAbstractMode::Cancel() {
		if (done) return;
		
		// revert
		auto *pose = editor->GetPose();
		for (const auto& pair: originalTransform) {
			pose->SetTransform(pair.first, pair.second);
		}
		
		done = true;
		editor->SetMode(MakeHandle<ObjectSelectionMode>());
	}
	
#pragma mark - TranslateFrameMode
	
	TranslateFrameMode::TranslateFrameMode() { }
	TranslateFrameMode::~TranslateFrameMode() { }
	
	class TranslateFrameMode::View: public ModeView {
		Handle<TranslateFrameMode> mode;
		Editor& editor;
		Vector2 lastPos;
		bool first = true;
		
		struct Target {
			osobj::Frame *frame;
			Matrix4 m;
		};
		std::list<Target> targets;
		Vector3 center;
	protected:
		void OnMouseDown(MouseButton mb, const Vector2& v) override {
			if (mb == MouseButton::Left) {
				mode->Apply();
				return;
			} else if (mb == MouseButton::Right) {
				mode->Cancel();
				return;
			}
			ModeView::OnMouseDown(mb, v);
		}
		void OnMouseMove(const Vector2& v) override {
			if (!first) {
				auto delta = v - lastPos;
				const auto& sceneDef = editor.GetLastSceneDefinition();
				Vector3 delta3 = editor.UnprojectDelta(delta);
				delta3 *= Vector3::Dot(center - sceneDef.viewOrigin,
									   sceneDef.viewAxis[2]);
				
				auto *pose = editor.GetPose();
				SPAssert(pose);
				
				for (const auto& target: targets) {
					auto *f = target.frame;
					auto m = mode->GetOriginalTransform(f);
					auto shift = (target.m * Vector4(delta3, 0)).GetXYZ();
					pose->SetTransform(f, Matrix4::Translate(shift) * m);
				}
			} else {
				first = false;
				lastPos = v;
			}
		}
		void OnKeyDown(const std::string& key) override {
			if (key == "Escape") {
				mode->Cancel();
			}
			ModeView::OnKeyDown(key);
		}
	public:
		View(UIManager *m, Editor *e,
			 TranslateFrameMode *mode):
		ModeView(m, e), editor(*e), mode(mode) {
			
			center = Vector3(0, 0, 0);
			int count = 0;
			for (auto *f: mode->GetTargetFrames()) {
				Target target;
				target.frame = f;
				
				Matrix4 m = mode->GetGlobalOriginalTransform(f);
				center += m.GetOrigin(); ++ count;
				m = mode->GetOriginalTransform(f) * m.InversedFast();
				target.m = m;
				
				targets.emplace_back(target);
			}
			center /= count;
		}
		
		
	};
	
	
	UIElement *TranslateFrameMode::CreateView
	(UIManager *m, Editor *e) {
		return new View(m, e, this);
	}
	
	
	
#pragma mark - RotateFrameMode
	
	RotateFrameMode::RotateFrameMode() { }
	RotateFrameMode::~RotateFrameMode() { }
	
	class RotateFrameMode::View: public ModeView {
		Handle<RotateFrameMode> mode;
		Editor& editor;
		float lastAngle;
		bool first = true;
		
		struct Target {
			osobj::Frame *frame;
			Matrix4 m;
		};
		std::list<Target> targets;
		Vector2 origin;
		Vector2 lastPoint;
	protected:
		void OnMouseDown(MouseButton mb, const Vector2& v) override {
			if (mb == MouseButton::Left) {
				mode->Apply();
				return;
			} else if (mb == MouseButton::Right) {
				mode->Cancel();
				return;
			}
			ModeView::OnMouseDown(mb, v);
		}
		void OnMouseMove(const Vector2& v) override {
			if ((v - origin).GetPoweredLength() < 2.f) {
				// cannot compute angle
				return;
			}
			
			lastPoint = v;
			
			float angle = atan2f(v.y - origin.y, v.x - origin.x);
			if (!first) {
				angle -= lastAngle;
				const auto& sceneDef = editor.GetLastSceneDefinition();
				
				auto *pose = editor.GetPose();
				SPAssert(pose);
				
				for (const auto& target: targets) {
					auto *f = target.frame;
					auto m = mode->GetOriginalTransform(f);
					auto axis = (target.m * Vector4(sceneDef.viewAxis[2], 0)).GetXYZ();
					
					// TODO: rotate
					pose->SetTransform(f, m * Matrix4::Rotate(axis, angle));
				}
			} else {
				first = false;
				lastAngle = angle;
			}
		}
		void OnKeyDown(const std::string& key) override {
			if (key == "Escape") {
				mode->Cancel();
			}
			ModeView::OnKeyDown(key);
		}
		void RenderClient() override {
			auto *r = GetManager().GetRenderer();
			auto img = ToHandle(r->RegisterImage("Gfx/DashLine.tga"));
			if (first) {
				return;
			}
			auto v1 = origin;
			auto v2 = lastPoint;
			auto dir = (v2 - v1).Normalize();
			Vector2 side(dir.y, -dir.x);
			
			r->SetColorAlphaPremultiplied(Vector4(1, 1, 1, 1));
			r->DrawImage(img, v1 + side, v2 + side, v1 - side
						 , AABB2(0, img->GetHeight() * .25f, (v2 - v1).GetLength(), 0));
		}
	public:
		View(UIManager *m, Editor *e,
			 RotateFrameMode *mode):
		ModeView(m, e), editor(*e), mode(mode) {
			
			Vector3 center = Vector3(0, 0, 0);
			int count = 0;
			for (auto *f: mode->GetTargetFrames()) {
				Target target;
				target.frame = f;
				
				Matrix4 m = mode->GetGlobalOriginalTransform(f);
				center += m.GetOrigin(); ++ count;
				m = m.InversedFast();
				target.m = m;
				
				targets.emplace_back(target);
			}
			center /= count;
			
			center = editor.Project(center);
			origin = Vector2(center.x, center.y);
			lastPoint = origin;
		}
		
		
	};
	
	
	UIElement *RotateFrameMode::CreateView
	(UIManager *m, Editor *e) {
		return new View(m, e, this);
	}
	
	
} }
