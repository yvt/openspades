//
//  SDLRunner.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/IRunnable.h"
#include "../Imports/SDL.h"
#include <string>

namespace spades {
	namespace client{
		class Client;
		class IRenderer;
		class IAudioDevice;
	}
	namespace gui {
		class SDLRunner: public IRunnable {
			
		protected:
			std::string host;
			std::string playerName;
			std::string TranslateKey(const SDL_keysym&);
			std::string TranslateButton(Uint8 b);
			virtual int GetModState();
			void ProcessEvent(SDL_Event& event,
							  client::Client *);
			virtual void RunClientLoop(client::IRenderer *renderer, client::IAudioDevice *dev);
		public:
			SDLRunner(std::string host, std::string playerName);
			virtual ~SDLRunner();
			virtual void Run();
		};
	}
}
