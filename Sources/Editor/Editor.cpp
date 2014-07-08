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

namespace spades { namespace editor {

	
	Editor::Editor(client::IRenderer *renderer,
				   client::IAudioDevice *audio):
	renderer(renderer),
	audio(audio),
	ui(new UIManager(renderer), false),
	viewCenter(0, 0, 0),
	viewAngle(0, M_PI * 0.2f, 0),
	viewDistance(10.f) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(renderer);
		SPAssert(audio);
		
		renderer->Init();
		
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
		
		auto b = MakeHandle<Button>(ui);
		b->SetText("Hello World!");
		b->SetBounds(AABB2(120, 120, 100, 25));
		mainView->AddChildToFront(b);
		
		{
			auto ed = MakeHandle<Field>(ui);
			ed->SetBounds(AABB2(20, 20, 150, 20));
			ed->SetText("hoge");
			mainView->AddChildToBack(ed);
		}
		{
			auto ed = MakeHandle<ScrollBar>(ui);
			ed->SetBounds(AABB2(20, 60, 16, 100));
			mainView->AddChildToBack(ed);
		}
		{
			auto ed = MakeHandle<OutlinerWindow>(ui,
												 *this);
			ed->SetBounds(AABB2(20, 200, 120, 150));
			mainView->AddChildToBack(ed);
		}
	}
	
	Editor::~Editor() {
		
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
		viewAngle.x -= v.x;
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
		
		Vector3 right = Vector3::Cross(dir, up).Normalize();
		up = Vector3::Cross(right, dir).Normalize();
		
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
	
	void Editor::RunFrame(float dt) {
		sceneDef = CreateSceneDefinition();
		
		renderer->SetFogColor(Vector3(.5f, .5f, .5f));
		renderer->SetFogDistance(128.f);
		renderer->SetFogType(client::FogType::Classical);
		renderer->StartScene(sceneDef);
		
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
		
		ui->Update(dt);
		ui->Render();
		renderer->FrameDone();
		renderer->Flip();
	}
	
	
	
	
	
	
} }


