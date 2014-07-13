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

#include "OutlinerWindow.h"
#include "Editor.h"
#include "ListView.h"
#include "TreeView.h"
#include "Buttons.h"
#include "Scene.h"
#include <Core/Strings.h>

namespace spades { namespace editor {
	
	class OutlinerTreeItemView: public UIElement {
		Handle<client::IImage> img;
	protected:
		virtual std::string GetText() = 0;
		virtual int GetImageIndex() = 0;
		virtual bool IsActive() { return false; }
		virtual bool IsGrayedOut() { return false; }
		
		void RenderClient() override {
			auto *f = GetFont();
			auto *r = GetManager().GetRenderer();
			auto sz = f->Measure(GetText());
			auto rt = GetScreenBounds();
			auto p = rt.min;
			p.y += (rt.GetHeight() - sz.y) * .5f - 1.f;
			p.x += 19.f;
			f->Draw(GetText(), p, 1.f,
						  Vector4(0, 0, 0, 1));
			
			if (!img) {
				img = ToHandle(r->RegisterImage("Gfx/UI/EditorItems.png"));
			}
			
			if (IsActive()) {
				r->SetColorAlphaPremultiplied(Vector4(0, 0, 0, 0.3));
				r->DrawImage(img, Vector2(rt.min.x, rt.min.y + (rt.GetHeight() - 16.f) * .5f),
							 AABB2(0, 0, 16.f, 16.f));
			}
			
			r->SetColorAlphaPremultiplied(Vector4(1, 1, 1, 1) *
										  (IsGrayedOut() ? .5f : 1.f));
			r->DrawImage(img, Vector2(rt.min.x, rt.min.y + (rt.GetHeight() - 16.f) * .5f),
						 AABB2(16.f * GetImageIndex(), 0, 16.f, 16.f));
		}
		
	public:
		OutlinerTreeItemView(UIManager *m):
		UIElement(m) {
		}
	};
	
	class ObjectTreeItem: public TreeViewItem {
		Handle<osobj::Object> rf;
	protected:
		~ObjectTreeItem() { }
	public:
		ObjectTreeItem(osobj::Object *rf): rf(rf) {
			SPAssert(rf);
		}
		virtual std::size_t GetNumChildren() {
			return 0;
		}
		virtual TreeViewItem *CreateChild(std::size_t index) {
			SPAssert(false);
			return nullptr;
		}
		virtual UIElement *CreateView(UIManager *m) {
			class ConcreteView: public OutlinerTreeItemView {
				Handle<osobj::Object> f;
			protected:
				std::string GetText() {
					return _Tr("Editor", "Object");
				}
				int GetImageIndex() { return 5; }
				bool IsActive() { return false; }
				bool IsGrayedOut() { return false; }
			public:
				ConcreteView(osobj::Object *f,
							 UIManager *m):
				OutlinerTreeItemView(m), f(f) { }
			};
			
			return new ConcreteView(rf, m);
		}
	};
	
	class ConstraintTreeItem: public TreeViewItem {
		Handle<osobj::Constraint> rf;
	protected:
		~ConstraintTreeItem() { }
	public:
		ConstraintTreeItem(osobj::Constraint *rf): rf(rf) {
			SPAssert(rf);
		}
		virtual std::size_t GetNumChildren() {
			return 0;
		}
		virtual TreeViewItem *CreateChild(std::size_t index) {
			SPAssert(false);
			return nullptr;
		}
		virtual UIElement *CreateView(UIManager *m) {
			class ConcreteView: public OutlinerTreeItemView {
				Handle<osobj::Constraint> f;
			protected:
				std::string GetText() {
					auto typ = f->GetType();
					switch (typ) {
						case osobj::ConstraintType::Ball:
							return _Tr("Editor", "Ball Joint");
						case osobj::ConstraintType::Hinge:
							return _Tr("Editor", "Hinge Joint");
						case osobj::ConstraintType::UniversalJoint:
							return _Tr("Editor", "Universal Joint");
						default:
							return "???";
					}
				}
				int GetImageIndex() { return 4; }
				bool IsActive() { return false; }
				bool IsGrayedOut() { return false; }
			public:
				ConcreteView(osobj::Constraint *f,
							 UIManager *m):
				OutlinerTreeItemView(m), f(f) { }
			};
			
			return new ConcreteView(rf, m);
		}
	};
	class FrameTreeItem: public TreeViewItem,
	osobj::FrameListener {
		Handle<osobj::Frame> rf;
		std::vector<osobj::Object *> objects;
		std::vector<osobj::Constraint *> constraints;
		std::vector<osobj::Frame *> children;
		std::size_t GetStartIndexForObjects() {
			return 0;
		}
		std::size_t GetStartIndexForConstraints() {
			return objects.size();
		}
		std::size_t GetStartIndexForChildFrames() {
			return objects.size() + constraints.size();
		}
		void ChildFrameAdded(osobj::Frame*, osobj::Frame *child) override {
			children.push_back(child);
			ChildAdded(children.size() - 1 + GetStartIndexForChildFrames(), 1);
		}
		void ChildFrameRemoved(osobj::Frame*, osobj::Frame *child) override {
			auto it = std::find(children.begin(), children.end(), child);
			if (it != children.end()) {
				auto idx = it - children.begin();
				children.erase(it);
				ChildRemoved(idx + GetStartIndexForChildFrames(), 1);
			}
		}
		void ConstraintAdded(osobj::Frame*e, osobj::Constraint *c) override {
			constraints.push_back(c);
			ChildAdded(children.size() - 1 + GetStartIndexForConstraints(), 1);
		}
		void ConstraintRemoved(osobj::Frame*, osobj::Constraint *c) override {
			auto it = std::find(constraints.begin(), constraints.end(), c);
			if (it != constraints.end()) {
				auto idx = it - constraints.begin();
				constraints.erase(it);
				ChildRemoved(idx + GetStartIndexForConstraints(), 1);
			}
		}
		void ObjectAdded(osobj::Frame*, osobj::Object *o) override {
			objects.push_back(o);
			ChildAdded(objects.size() - 1 + GetStartIndexForObjects(), 1);
		}
		void ObjectRemoved(osobj::Frame*, osobj::Object *o) override {
			auto it = std::find(objects.begin(), objects.end(), o);
			if (it != objects.end()) {
				auto idx = it - objects.begin();
				objects.erase(it);
				ChildRemoved(idx + GetStartIndexForObjects(), 1);
			}
		}
	protected:
		~FrameTreeItem() {
			rf->RemoveListener(this);
		}
	public:
		FrameTreeItem(osobj::Frame *rf): rf(rf) {
			SPAssert(rf);
			for (const auto& e: rf->GetChildren())
				children.push_back(e);
			for (const auto& o: rf->GetObjects())
				objects.push_back(o);
			for (const auto& c: rf->GetConstraints())
				constraints.push_back(c);
			rf->AddListener(this);
		}\
		std::size_t GetNumChildren() override {
			return children.size() + constraints.size() + objects.size();
		}
		TreeViewItem *CreateChild(std::size_t index) override {
			if (index >= GetStartIndexForChildFrames()) {
				return new FrameTreeItem
				(children.at
				 (index - GetStartIndexForChildFrames()));
			} else if (index >= GetStartIndexForConstraints()) {
				return new ConstraintTreeItem
				(constraints.at
				 (index - GetStartIndexForConstraints()));
			} else if (index >= GetStartIndexForObjects()) {
				return new ObjectTreeItem
				(objects.at
				 (index - GetStartIndexForObjects()));
			} else {
				return nullptr;
			}
		}
		UIElement *CreateView(UIManager *m) override {
			class ConcreteView: public OutlinerTreeItemView {
				Handle<osobj::Frame> f;
			protected:
				std::string GetText() {
					std::string text = f->GetId();
					const auto& tags = f->GetTags();
					if (!tags.empty()) {
						text += " [";
						bool first = true;
						for (const auto& s: tags) {
							if (!first) text += ", ";
							text += s;
						}
						text += "]";
					}
					return text;
				}
				int GetImageIndex() { return 2; }
				bool IsActive() { return false; }
				bool IsGrayedOut() { return false; }
			public:
				ConcreteView(osobj::Frame *f,
							 UIManager *m):
				OutlinerTreeItemView(m), f(f) { }
			};
			
			return new ConcreteView(rf, m);
		}
		void RecycleView(UIElement *) override {
		}
	};
	
	class RootFrameTreeItem: public TreeViewItem {
		Handle<RootFrame> rf;
	protected:
		~RootFrameTreeItem() { }
	public:
		RootFrameTreeItem(RootFrame *rf): rf(rf) {
			SPAssert(rf);
		}
		virtual std::size_t GetNumChildren() {
			return 1;
		}
		virtual TreeViewItem *CreateChild(std::size_t index) {
			return new FrameTreeItem(rf->frame);
		}
		virtual UIElement *CreateView(UIManager *m) {
			class ConcreteView: public OutlinerTreeItemView {
				Handle<RootFrame> f;
			protected:
				std::string GetText() {
					return f->name;
				}
				int GetImageIndex() { return 6; }
				bool IsActive() { return false; }
				bool IsGrayedOut() { return f->isForeignObject; }
			public:
				ConcreteView(RootFrame *f,
							 UIManager *m):
				OutlinerTreeItemView(m), f(f) { }
			};
			
			return new ConcreteView(rf, m);
		}
	};
	
	class TimelineTreeItem: public TreeViewItem {
		Handle<TimelineItem> tl;
	protected:
		~TimelineTreeItem() { }
	public:
		TimelineTreeItem(TimelineItem *tl): tl(tl) {
			SPAssert(tl);
		}
		virtual std::size_t GetNumChildren() {
			return 0;
		}
		virtual TreeViewItem *CreateChild(std::size_t index) {
			return nullptr;
		}
		virtual UIElement *CreateView(UIManager *m) {
			class ConcreteView: public OutlinerTreeItemView {
				Handle<TimelineItem> f;
			protected:
				std::string GetText() {
					return f->name;
				}
				int GetImageIndex() { return 3; }
				bool IsActive() { return false; }
				bool IsGrayedOut() { return false; }
			public:
				ConcreteView(TimelineItem *f,
							 UIManager *m):
				OutlinerTreeItemView(m), f(f) { }
			};
			
			return new ConcreteView(tl, m);
		}
	};
	
	class SceneTreeItem: public TreeViewItem,
	SceneListener {
		Handle<Scene> scene;
		std::vector<RootFrame *> rootFrames;
		std::vector<TimelineItem *> timelines;
		
		void RootFrameAdded(RootFrame *f) override {
			rootFrames.push_back(f);
			ChildAdded(rootFrames.size() - 1, 1);
		}
		void RootFrameRemoved(RootFrame *f) override {
			auto it = std::find(rootFrames.begin(), rootFrames.end(),
								f);
			if (it != rootFrames.end()) {
				auto idx = it - rootFrames.begin();
				rootFrames.erase(it);
				ChildRemoved(idx, 1);
			}
		}
		
		void TimelineAdded(TimelineItem *t) override {
			timelines.push_back(t);
			ChildAdded(timelines.size() - 1 +
					   rootFrames.size(), 1);
		}
		void TimelineRemoved(TimelineItem *t) override {
			auto it = std::find(timelines.begin(), timelines.end(),
								t);
			if (it != timelines.end()) {
				auto idx = it - timelines.begin();
				timelines.erase(it);
				ChildRemoved(idx + rootFrames.size(), 1);
			}
		}
	protected:
		~SceneTreeItem() {
			scene->RemoveListener(this);
		}
	public:
		SceneTreeItem(Scene *s): scene(s) {
			SPAssert(s);
			
			for (const auto& rf: s->GetRootFrames())
				rootFrames.push_back(rf);
			for (const auto& tl: s->GetTimelines())
				timelines.push_back(tl);
			
			s->AddListener(this);
			
			SetExpanded(true);
		}
		std::size_t GetNumChildren() {
			return rootFrames.size() + timelines.size();
		}
		TreeViewItem *CreateChild(std::size_t index) {
			if (index < scene->GetRootFrames().size()) {
				return new RootFrameTreeItem
				(scene->GetRootFrames().at(index));
			} else {
				return new TimelineTreeItem
				(scene->GetTimelines().at(index - scene->GetRootFrames().size()));
			}
		}
		UIElement *CreateView(UIManager *m) {
			class ConcreteView: public OutlinerTreeItemView {
				Handle<Scene> f;
			protected:
				std::string GetText() {
					return _Tr("Editor", "Scene");
				}
				int GetImageIndex() { return 1; }
				bool IsActive() { return false; }
				bool IsGrayedOut() { return false; }
			public:
				ConcreteView(Scene *f,
							 UIManager *m):
				OutlinerTreeItemView(m), f(f) { }
			};
			
			return new ConcreteView(scene, m);
		}
	};
	
	class OutlinerWindow::Internal:
	public EditorListener {
		OutlinerWindow& w;
	public:
		Internal(OutlinerWindow& w): w(w) { }
		
		void SceneChanged(Scene *sc) override {
			if (sc) {
				auto model = MakeHandle<TreeViewModel>
				(&w.GetManager(), MakeHandle<SceneTreeItem>(sc));
				w.listView->SetModel(model);
			} else {
				w.listView->SetModel(nullptr);
			}
		}
	};
	
	OutlinerWindow::OutlinerWindow
	(UIManager *m, Editor&e):
	internal(new Internal(*this)),
	Window(m),
	editor(e) {
		listView = MakeHandle<ListView>(m);
		AddChildToFront(listView);
		
		auto *sc = e.GetScene();
		if (sc) {
			auto model = MakeHandle<TreeViewModel>
			(m, MakeHandle<SceneTreeItem>(sc));
			listView->SetModel(model);
		}
		
		e.AddListener(internal.get());
	}
	
	OutlinerWindow::~OutlinerWindow() {
		editor.RemoveListener(internal.get());
	}
	
	client::IFont *OutlinerWindow::GetTitleFont() {
		return editor.GetTitleFont();
	}
	
	std::string OutlinerWindow::GetTitle() {
		return _Tr("Editor", "Outline");
	}
	
	void OutlinerWindow::RenderClient() {
		Window::RenderClient();
		listView->SetBounds(GetClientBounds());
	}
	
	Vector2 OutlinerWindow::AdjustClientSize(const Vector2& sz) {
		auto rh = listView->GetRowHeight();
		auto sz2 = Vector2(sz.x, std::max(floorf(sz.y / rh + .5f), 3.f) * rh);
		return Window::AdjustClientSize(sz2);
	}
	
	
} }

