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

#include "SDLAsyncRunner.h"
#include "../Core/Thread.h"
#include "../Client/Client.h"
#include "../Client/AsyncRenderer.h"
#include "../Core/ConcurrentDispatch.h"

namespace spades {
	namespace gui {
		SDLAsyncRunner::SDLAsyncRunner(const ServerAddress& host,
									   std::string pn):
		SDLRunner(host, pn){
			currentClient = NULL;
		}
		
		SDLAsyncRunner::~SDLAsyncRunner(){
			
		}
		
		class SDLAsyncRunner::ClientThread: public Thread {
		public:
			SDLAsyncRunner *runner;
			client::IRenderer *renderer;
			client::IAudioDevice *audio;
			virtual void Run() {
				SPLog("Starting Client Thread");
				runner->ClientThreadProc(renderer, audio);
				SPLog("Client Thread Done");
			}
		};
		
		void SDLAsyncRunner::RunClientLoop(client::IRenderer *renderer,
										   client::IAudioDevice *audio) {
			client::AsyncRenderer asyncRenderer(renderer,
												DispatchQueue::GetThreadQueue());
			modState = 0;
			ClientThread cliThread;
			cliThread.runner = this;
			cliThread.renderer = &asyncRenderer;
			cliThread.audio = audio;
			cliThread.Start();
			
			SPLog("Main event loop started");
			while(cliThread.IsAlive()) {
				
				
				modState = SDLRunner::GetModState();
				
				if(currentClient){
					
					class SDLEventDispatch: public ConcurrentDispatch {
						SDLAsyncRunner *runner;
						SDL_Event ev;
					public:
						SDLEventDispatch(SDLAsyncRunner *runner,
										 const SDL_Event& ev):
						runner(runner),
						ev(ev){
							
						}
						virtual void Run() {
							client::Client *cli = runner->currentClient;
							if(cli){
								runner->ProcessEvent(ev, cli);
							}
						}
					};
					
					SDL_Event event;
					
					if(SDL_WaitEvent(&event)){
						{
							SDLEventDispatch *disp = new SDLEventDispatch(this, event);
							
							// FIXME: cliQueue may be deleted..
							if(cliQueue){
								disp->StartOn(cliQueue);
								disp->Release();
							}else{
								delete disp;
							}
						}
						
						while(SDL_PollEvent(&event)) {
							SDLEventDispatch *disp = new SDLEventDispatch(this, event);
							
							// FIXME: cliQueue may be deleted..
							if(cliQueue){
								disp->StartOn(cliQueue);
								disp->Release();
							}else{
								delete disp;
							}
						}
					}
					
					
					
				}else{
					SDL_Event event;
					SDL_WaitEvent(&event);
					
					if(event.type == SDL_QUIT){
						break;
					}
				}
				
				DispatchQueue::GetThreadQueue()->ProcessQueue();
			}
			
			SPLog("Main event loop ended");
			if(!clientError.empty()){
				SPLog("Client reported an error: \n%s",
					  clientError.c_str());
				SPRaise("Client error:\n%s", clientError.c_str());
			}
		}
		
		void SDLAsyncRunner::ClientThreadProc(client::IRenderer *renderer, client::IAudioDevice *audio){
			try{
				client::Client client(renderer, audio,
									  host, playerName);
				Uint32 ot = SDL_GetTicks();
				bool running = true;
				
				currentClient = &client;
				cliQueue = DispatchQueue::GetThreadQueue();
				
				bool lastShift = false;
				bool lastCtrl = false;
				
				while(running){
					
					
					DispatchQueue::GetThreadQueue()->ProcessQueue();
					
					Uint32 dt = SDL_GetTicks() - ot;
					client.RunFrame((float)dt / 1000.f);
					ot += dt;
					
					if(client.WantsToBeClosed()){
						client.Closing();
						running = false;
						break;
					}
					
					int modState = GetModState();
					if(modState & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)){
						if(!lastCtrl){
							client.KeyEvent("Control", true);
							lastCtrl = true;
						}
					}else{
						if(lastCtrl){
							client.KeyEvent("Control", false);
							lastCtrl = false;
						}
					}
					
					if(modState & (KMOD_SHIFT | KMOD_LSHIFT | KMOD_RSHIFT)){
						if(!lastShift){
							client.KeyEvent("Shift", true);
							lastShift = true;
						}
					}else{
						if(lastShift){
							client.KeyEvent("Shift", false);
							lastShift = false;
						}
					}
					
					//Fl::check();
				}
				
				cliQueue = NULL;
				currentClient = NULL;
			}catch(const std::exception& ex){
				SPLog("Exiting Client Event Loop due to exception:\n%s",
					  ex.what());
				cliQueue = NULL;
				currentClient = NULL;
				clientError = ex.what();
			}
		}
		
		
	}
}
