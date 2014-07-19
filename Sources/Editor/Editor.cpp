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

#include "Editor.h"
#include "MainView.h"
#include "UIElement.h"
#include <NGClient/FTFont.h>
#include "Buttons.h"
#include "Fields.h"
#include "ScrollBar.h"
#include "OutlinerWindow.h"
#include "Commands.h"
#include "Scene.h"
#include <Core/ModelTree.h>
#include "SceneRenderer.h"
#include "ObjectMode.h"

namespace spades { namespace editor {

	class Editor::Internal:
	public SceneListener {
		Editor& e;
	public:
		Internal(Editor& e): e(e) {
		}
		void RootFrameAdded(RootFrame *) override { }
		void RootFrameRemoved(RootFrame *)  override { }
		
		void TimelineAdded(TimelineItem *) override { }
		void TimelineRemoved(TimelineItem *t) override { }
	};
	
	Editor::Editor(client::IRenderer *renderer,
				   client::IAudioDevice *audio):
	renderer(renderer),
	audio(audio),
	ui(new UIManager(renderer), false),
	viewCenter(0, 0, 0),
	viewAngle(0, M_PI * 0.2f, 0),
	viewDistance(10.f),
	commandManager(MakeHandle<CommandManager>()),
	internal(new Internal(*this)) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(renderer);
		SPAssert(audio);
		
		renderer->Init();
		
		selRenderer.reset(new SelectionRenderer(renderer));
		
		mainView.Set(new MainView(ui, *this), false);
		ui->SetRootElement(mainView);
		
		auto fontSet = MakeHandle<ngclient::FTFontSet>();
		fontSet->AddFace("Gfx/Fonts/SourceSansPro-Semibold.ttf");
		fontSet->AddFace("Gfx/Fonts/mplus-2m-medium.ttf");
		
		auto font = MakeHandle<ngclient::FTFont>(renderer, fontSet,
												 12.f, 12.f * 1.25f);
		ui->SetFont(font);
		
		auto fontSet2 = MakeHandle<ngclient::FTFontSet>();
		fontSet2->AddFace("Gfx/Fonts/SourceSansPro-Bold.ttf");
		fontSet2->AddFace("Gfx/Fonts/mplus-2m-bold.ttf");
		
		titleFont = MakeHandle<ngclient::FTFont>(renderer, fontSet2,
												11.f, 11.f * 1.25f);
		
		{
			auto ed = MakeHandle<OutlinerWindow>(ui,
												 *this);
			ed->SetBounds(AABB2(20, 200, 120, 150));
			mainView->AddChildToBack(ed);
		}
		
		// for testing
		auto scene = MakeHandle<Scene>();
		auto loader = MakeHandle<osobj::Loader>("Models/Player/");
		auto obj = ToHandle(loader->LoadFrame("lower.osobj"));
		auto rf = MakeHandle<RootFrame>();
		rf->frame = obj;
		rf->name = "lower.osobj";
		scene->AddRootFrame(rf);
		SetScene(scene);
		
		SetMode(MakeHandle<ObjectSelectionMode>());
	}
	
	Editor::~Editor() {
		
	}
	
	void Editor::AddListener(EditorListener *l) {
		SPAssert(l);
		listeners.insert(l);
	}
	
	void Editor::RemoveListener(EditorListener *l) {
		listeners.erase(l);
	}
	
	void Editor::SetScene(Scene *s) {
		if (scene == s) return;
		scene = s;
		for (auto *l: listeners)
			l->SceneChanged(s);
		
		sceneRenderer = s ? MakeHandle<SceneRenderer>(s, renderer) :
		ToHandle<SceneRenderer>(nullptr);
	}
	
	void Editor::SetActiveTimeline(TimelineItem *s) {
		if (activeTimeline == s) return;
		
		// exit object editing mode
		if (s) SetActiveObject(nullptr);
		
		activeTimeline = s;
		for (auto *l: listeners)
			l->ActiveTimelineChanged(s);
	}
	
	void Editor::SetActiveObject(osobj::Object *s) {
		if (activeObject == s) return;
		
		// exit animation mode
		if (s) SetActiveTimeline(nullptr);
		
		activeObject = s;
		for (auto *l: listeners)
			l->ActiveObjectChanged(s);
	}
	
	void Editor::SetSelectedFrames(const decltype(selectedFrames) &f) {
		selectedFrames = f;
		for (auto *l: listeners)
			l->SelectedFramesChanged();
	}
	
	bool Editor::IsSelected(osobj::Frame *f) {
		return std::find(selectedFrames.begin(),
						 selectedFrames.end(), f) !=
		selectedFrames.end();
	}
	
	void Editor::SetMode(EditorMode *mode) {
		if (this->mode == mode) return;
		
		if (this->mode) {
			this->mode->Leave(this);
			this->mode = nullptr;
		}
		
		this->mode = mode;
		this->mode->Enter(this);
		
		for (auto *l: listeners)
			l->EditorModeChanged(mode);
	}
	
	void Editor::Select(osobj::Frame *f,
						bool append) {
		auto sel = GetSelectedFrames();
		if (!append) sel.clear();
		auto it = std::find(sel.begin(), sel.end(), f);
		if (it != sel.end()) {
			sel.erase(it);
		}
		sel.push_front(f);
		SetSelectedFrames(sel);
	}
	
	void Editor::Deselect(osobj::Frame *f) {
		auto sel = GetSelectedFrames();
		auto it = std::find(sel.begin(), sel.end(), f);
		if (it != sel.end()) {
			sel.erase(it);
			SetSelectedFrames(sel);
		}
	}
	
	void Editor::MouseEvent(float x, float y) {
		ui->MouseEvent(x, y);
	}
	
	void Editor::KeyEvent(const std::string &key, bool down) {
		ui->KeyEvent(key, down);
	}
	
	void Editor::TextInputEvent(const std::string &text) {
		ui->TextInputEvent(text);
	}
	
	void Editor::TextEditingEvent(const std::string &text, int start, int len) {
		ui->TextEditingEvent(text, start, len);
	}
	
	bool Editor::AcceptsTextInput() {
		return ui->AcceptsTextInput();
	}
	
	AABB2 Editor::GetTextInputRect() {
		return ui->GetTextInputRect();
	}
	
	bool Editor::NeedsAbsoluteMouseCoordinate() {
		return true;
	}
	
	void Editor::WheelEvent(float x, float y) {
		ui->WheelEvent(x, y);
	}
	
	void Editor::Closing() {
		
	}
	
	bool Editor::WantsToBeClosed() {
		return false;
	}
	
	void Editor::Turn(const spades::Vector2 &v) {
		viewAngle.x += v.x;
		viewAngle.y += v.y;
		if (viewAngle.y > M_PI * .49f)
			viewAngle.y = M_PI * .49f;
		if (viewAngle.y < M_PI * -.49f)
			viewAngle.y = M_PI * -.49f;
	}
	
	void Editor::SideMove(const Vector2& v) {
		auto def = CreateSceneDefinition();
		viewCenter += def.viewAxis[0] * v.x;
		viewCenter += def.viewAxis[1] * v.y;
	}
	void Editor::Strafe(const Vector2& v) {
		auto def = CreateSceneDefinition();
		viewCenter += def.viewAxis[0] * v.x;
		viewDistance *= expf(v.y * -.5f);
		viewDistance = std::max(viewDistance, .0001f);
		viewDistance = std::min(viewDistance, 20.f);
	}
	
	client::SceneDefinition Editor::CreateSceneDefinition() {
		client::SceneDefinition def;
		
		Vector3 eye = viewCenter;
		eye.x += cosf(viewAngle.x) * cosf(viewAngle.y) * viewDistance;
		eye.y += sinf(viewAngle.x) * cosf(viewAngle.y) * viewDistance;
		eye.z -= sinf(viewAngle.y) * viewDistance;
		
		Vector3 dir = (viewCenter - eye).Normalize();
		Vector3 up {0, 0, -1};
		
		Vector3 right = Vector3::Cross(up, dir).Normalize();
		up = Vector3::Cross(dir, right).Normalize();
		
		def.viewOrigin = eye;
		def.viewAxis[0] = -right;
		def.viewAxis[1] = up;
		def.viewAxis[2] = dir;
		
		def.fovX = 60.f * M_PI / 180.f;
		def.fovY = atanf(tanf(def.fovX * .5f) *
						 renderer->ScreenHeight() / renderer->ScreenWidth()) * 2.f;
		
		def.skipWorld = true;
		def.denyCameraBlur = true;
		def.depthOfFieldNearRange = 0.f;
		def.globalBlur = 0.f;
		def.radialBlur = 0.f;
		def.saturation = 1.f;
		def.time = 0;
		def.zNear = .01f;
		def.zFar = 100.f;
		
		return def;
	}
	
	Vector3 Editor::Unproject(const Vector2& v) {
		auto sh = Vector2(renderer->ScreenWidth(),
						  renderer->ScreenHeight()) * .5f;
		float fovX = tanf(sceneDef.fovX * .5f) / sh.x;
		float fovY = tanf(sceneDef.fovY * .5f) / sh.y;
		auto v2 = (v - sh) * Vector2(fovX, fovY);
		return sceneDef.viewAxis[0] * v2.x - sceneDef.viewAxis[1] * v2.y
		+ sceneDef.viewAxis[2];
	}
	Vector3 Editor::UnprojectDelta(const Vector2& v) {
		auto sh = Vector2(renderer->ScreenWidth(),
						  renderer->ScreenHeight()) * .5f;
		float fovX = tanf(sceneDef.fovX * .5f) / sh.x;
		float fovY = tanf(sceneDef.fovY * .5f) / sh.y;
		auto v2 = v * Vector2(fovX, fovY);
		return sceneDef.viewAxis[0] * v2.x - sceneDef.viewAxis[1] * v2.y;
	}
	
	void Editor::RunFrame(float dt) {
		sceneDef = CreateSceneDefinition();
		
		renderer->SetFogColor(Vector3(.5f, .5f, .5f));
		renderer->SetFogDistance(128.f);
		renderer->SetFogType(client::FogType::Classical);
		renderer->StartScene(sceneDef);
		
		if (sceneRenderer) {
			osobj::Pose *pose = GetPose();
			sceneRenderer->AddToScene(pose);
		}
		
		// draw grid
		for(int i = -10; i <= 10; ++i) {
			renderer->AddDebugLine(Vector3(i, -10, 0), Vector3(i, 10, 0),
								   i == 0 ? Vector4(.5, 0, 0, 1) :
								   Vector4(.8, .8, .8, 1));
			renderer->AddDebugLine(Vector3(-10, i, 0), Vector3(10, i, 0),
								   i == 0 ? Vector4(0, .5, 0, 1) :
								   Vector4(.8, .8, .8, 1));
		}
		
		renderer->AddDebugLine(Vector3(0, 0, -10), Vector3(0, 0, 10),
							   Vector4(0, 0, .5, 1));
		
		renderer->EndScene();
		
		// render selection
		// FIXME: maybe split this to another function?
		selRenderer->SetSceneDefiniton(sceneDef);
		if (activeObject) {
			renderer->SetColorAlphaPremultiplied(Vector4(1, 0.5, 0.5, 0.5));
			
			std::function<void(osobj::Frame *, const Matrix4&)> recurse;
			recurse = [&](osobj::Frame *f, const Matrix4& m) {
				auto mm = m * GetPose()->GetTransform(f);
				
				for (const auto& obj: f->GetObjects()) {
					if (obj == activeObject) {
						auto& vm = dynamic_cast<osobj::VoxelModelObject&>(*obj);
						selRenderer->RenderSelection(vm.GetModel(), mm);
					}
				}
				
				for (const auto& child: f->GetChildren()) {
					recurse(child, mm);
				}
			};
			for (const auto& rf: scene->GetRootFrames()) {
				recurse(rf->frame, rf->matrix);
			}
		} else {
			renderer->SetColorAlphaPremultiplied(Vector4(1, 1, 0.5, 0.5));
			for (const auto& sel: selectedFrames) {
				auto trans = GetPose()->GetTransform(sel);
				auto *f = sel->GetParent();
				while (f) {
					trans = GetPose()->GetTransform(f) * trans;
					if (f->GetParent() == nullptr) {
						for (const auto& rf: scene->GetRootFrames()) {
							if (rf->frame == f) {
								trans = rf->matrix * trans;
								break;
							}
						}
					}
					f = f->GetParent();
				}
				
				for (const auto& obj: sel->GetObjects()) {
					auto& vm = dynamic_cast<osobj::VoxelModelObject&>(*obj);
					selRenderer->RenderSelection(vm.GetModel(), trans);
				}
			}
		}
		
		// render UIs
		ui->Update(dt);
		ui->Render();
		renderer->FrameDone();
		renderer->Flip();
	}
	
	osobj::Pose *Editor::GetPose() {
		if (!workingPose) {
			workingPose = MakeHandle<osobj::Pose>();
			if (activeTimeline) {
				// TODO: apply animation
				activeTimeline->timeline->Move(0.f, *workingPose);
			}
		}
		return workingPose;
	}
	
	void Editor::ClearWorkingPose() {
		workingPose = nullptr;
	}
	
	void EditorMode::Enter(Editor *e) {
		e->AddListener(this);
	}
	
	void EditorMode::Leave(Editor *e) {
		e->RemoveListener(this);
	}
	
	
} }


