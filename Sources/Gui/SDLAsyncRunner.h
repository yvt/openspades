/*
 Copyright (c) 2013 OpenSpades Developers
 
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

#include "SDLRunner.h"
#include <Core/ConcurrentDispatch.h>
#include <Core/Mutex.h>
#include <atomic>

namespace spades {
	namespace client{
		class Client;
	}
	namespace gui {
		class SDLAsyncRunner: public SDLRunner {
			class ClientThread;
			ClientThread *cliThread;
			View *currentView;
			DispatchQueue *cliQueue;
			int modState;
			std::string clientError;
			
			struct PulledState {
				bool acceptsTextInput;
				SDL_Rect textInputRect;
				bool needsAbsoluteMouseCoord;
			};
			
			std::atomic<bool> rendererErrorOccured;
			PulledState state;
			Mutex stateMutex;
		protected:
			virtual int GetModState() { return modState; }
			virtual void RunClientLoop(client::IRenderer *renderer, client::IAudioDevice *dev);
			virtual void ClientThreadProc(client::IRenderer *renderer, client::IAudioDevice *dev);
		public:
			SDLAsyncRunner();
			virtual ~SDLAsyncRunner();
		};
	}
}

