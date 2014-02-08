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
#include "View.h"
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	namespace client {
		class IFont;
	}
	namespace gui {
		class StartupScreenHelper;
		class StartupScreen: public View {
			friend class StartupScreenHelper;
			
			
			Handle<client::IRenderer> renderer;
			Handle<client::IAudioDevice> audioDevice;
			Handle<client::IFont> font;
			float timeToStartInitialization;
			bool startRequested = false;
			
			Handle<StartupScreenHelper> helper;
			Handle<asIScriptObject> ui;
			
			void DrawStartupScreen();
			void DoInit();
			
			void RestoreRenderer();
			
			void Start();
		protected:
			virtual ~StartupScreen();
		public:
			StartupScreen(client::IRenderer *, client::IAudioDevice *, StartupScreenHelper *helper);
			
			client::IRenderer *GetRenderer() { return &*renderer; }
			client::IAudioDevice *GetAudioDevice() { return &*audioDevice; }
			
			virtual void MouseEvent(float x, float y);
			virtual void KeyEvent(const std::string&,
								  bool down);
			virtual void TextInputEvent(const std::string&);
			virtual void TextEditingEvent(const std::string&,
										  int start, int len);
			virtual bool AcceptsTextInput();
			virtual AABB2 GetTextInputRect();
			virtual void WheelEvent(float x, float y);
			virtual bool NeedsAbsoluteMouseCoordinate();
			
			virtual void RunFrame(float dt);
			
			virtual void Closing();
			
			virtual bool WantsToBeClosed();
			
			static void Run();
		};;
	}
}
