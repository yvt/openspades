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

namespace spades {
	namespace client {
		class IFont;
	}
	namespace gui {
		class MainScreen: public View {
			Handle<client::IRenderer> renderer;
			Handle<client::IAudioDevice> audioDevice;
			Handle<View> subview;
			Handle<client::IFont> font;
			bool shouldBeClosed;
		protected:
			virtual ~MainScreen();
		public:
			MainScreen(client::IRenderer *, client::IAudioDevice *);
			
			client::IRenderer *GetRenderer() { return &*renderer; }
			client::IAudioDevice *GetAudioDevice() { return &*audioDevice; }
			
			virtual void MouseEvent(float x, float y);
			virtual void KeyEvent(const std::string&,
								  bool down);
			virtual void CharEvent(const std::string&);
			
			virtual void RunFrame(float dt);
			
			virtual void Closing();
			
			virtual bool WantsToBeClosed() { return shouldBeClosed; }
		};;
	}
}
