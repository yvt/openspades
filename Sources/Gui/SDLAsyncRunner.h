//
//  SDLAsyncRunner.h
//  OpenSpades
//
//  Created by yvt on 7/29/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "SDLRunner.h"
#include "../Core/ConcurrentDispatch.h"

namespace spades {
	namespace client{
		class Client;
	}
	namespace gui {
		class SDLAsyncRunner: public SDLRunner {
			class ClientThread;
			ClientThread *cliThread;
			client::Client *currentClient;
			DispatchQueue *cliQueue;
			int modState;
			std::string clientError;
		protected:
			virtual int GetModState() { return modState; }
			virtual void RunClientLoop(client::IRenderer *renderer, client::IAudioDevice *dev);
			virtual void ClientThreadProc(client::IRenderer *renderer, client::IAudioDevice *dev);
		public:
			SDLAsyncRunner(std::string host, std::string playerName);
			virtual ~SDLAsyncRunner();
		};
	}
}

