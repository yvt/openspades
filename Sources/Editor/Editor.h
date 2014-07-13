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

#pragma once

#include <Core/RefCountedObject.h>
#include <Client/IRenderer.h>
#include <Client/IAudioDevice.h>
#include <Gui/View.h>
#include <Client/IFont.h>
#include <set>

namespace spades { namespace editor {
	
	class UIElement;
	class UIManager;
	class MainView;
	class CommandManager;
	class Scene;
	class EditorListener;
	
	class Editor: public gui::View {
		Handle<client::IRenderer> renderer;
		Handle<client::IAudioDevice> audio;
		std::set<EditorListener *> listeners;
		
		Handle<UIManager> ui;
		Handle<MainView> mainView;
		
		Handle<client::IFont> titleFont;
		
		Handle<Scene> scene;
		
		Handle<CommandManager> commandManager;
		
		Vector3 viewCenter;
		Vector3 viewAngle;
		float viewDistance;
		
		client::SceneDefinition CreateSceneDefinition();
		client::SceneDefinition sceneDef;
		
	protected:
		~Editor();
	public:
		
		Editor(client::IRenderer *,
			   client::IAudioDevice *);
		
		client::IRenderer *GetRenderer() { return renderer; }
		client::IAudioDevice *GetAudioDevice() { return audio; }
		
		client::IFont *GetTitleFont() const { return titleFont; }
		
		CommandManager& GetCommandManager() const { return *commandManager; }
		Scene *GetScene() const { return scene; }
		void SetScene(Scene *);
		
		void AddListener(EditorListener *);
		void RemoveListener(EditorListener *);
		
		void Turn(const Vector2&);
		void Strafe(const Vector2&);
		void SideMove(const Vector2&);
		
		/*---- implementations of gui::View ----*/
		void MouseEvent(float x, float y) override;
		void KeyEvent(const std::string&,
					  bool down) override;
		void TextInputEvent(const std::string&) override;
		void TextEditingEvent(const std::string&,
							  int start, int len) override;
		bool AcceptsTextInput() override;
		AABB2 GetTextInputRect() override;
		bool NeedsAbsoluteMouseCoordinate() override;
		void WheelEvent(float x, float y) override;
		
		void RunFrame(float dt) override;
		
		void Closing() override;
		
		bool WantsToBeClosed() override;
		
		
		
		
	};
	
	class EditorListener {
	public:
		virtual ~EditorListener() { }
		virtual void SceneChanged(Scene *) { }
	};
	
} }
