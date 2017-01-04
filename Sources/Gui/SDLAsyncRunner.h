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

#include <atomic>

#include "SDLRunner.h"
#include <Core/ConcurrentDispatch.h>
#include <Core/Mutex.h>

namespace spades {
	namespace client {
		class Client;
	}
	namespace gui {
		class SDLAsyncRunner : public SDLRunner {
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
			int GetModState() override { return modState; }
			void RunClientLoop(client::IRenderer *renderer, client::IAudioDevice *dev) override;
			void ClientThreadProc(client::IRenderer *renderer, client::IAudioDevice *dev);

		public:
			SDLAsyncRunner();
			~SDLAsyncRunner();
		};
	}
}
