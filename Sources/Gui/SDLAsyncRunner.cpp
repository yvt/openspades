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
		SDLAsyncRunner::SDLAsyncRunner():
		rendererErrorOccured(false){
			currentView = NULL;
		}
		
		SDLAsyncRunner::~SDLAsyncRunner(){
			
		}
		
		class SDLAsyncRunner::ClientThread: public Thread {
		public:
			SDLAsyncRunner *runner;
			client::IRenderer *renderer;
			client::IAudioDevice *audio;
			virtual ~ClientThread() {
				renderer->Release();
			}
			virtual void Run() {
				SPLog("Starting Client Thread");
				runner->ClientThreadProc(renderer, audio);
				SPLog("Client Thread Done");
			}
		};
		
		void SDLAsyncRunner::RunClientLoop(client::IRenderer *renderer,
										   client::IAudioDevice *audio) {
			client::AsyncRenderer *asyncRenderer = new client::AsyncRenderer(renderer,
												DispatchQueue::GetThreadQueue());
			modState = 0;
			ClientThread *cliThread = new ClientThread();
			cliThread->runner = this;
			cliThread->renderer = asyncRenderer;
			cliThread->audio = audio;
			cliThread->Start();
			
			SPLog("Main event loop started");
			try{
				while(cliThread->IsAlive()) {
					
					
					modState = SDLRunner::GetModState();
					
					if(currentView){
						
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
								View *view = runner->currentView;
								if(view){
									runner->ProcessEvent(ev, view);
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
			}catch(const std::exception& ex){
				rendererErrorOccured = true;
				SPLog("Renderer error:\n%s", ex.what());
				cliThread->MarkForAutoDeletion();
				SPLog("Main event loop terminated");
				throw;
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
				Handle<View> view(CreateView(renderer, audio), false);
				Uint32 ot = SDL_GetTicks();
				bool running = true;
				
				currentView = view;
				cliQueue = DispatchQueue::GetThreadQueue();
				
				bool lastShift = false;
				bool lastCtrl = false;
				bool lastGui = false;
				bool lastAlt = false;
				
				while(running && !rendererErrorOccured){
					
					
					DispatchQueue::GetThreadQueue()->ProcessQueue();
					
					Uint32 dt = SDL_GetTicks() - ot;
					view->RunFrame((float)dt / 1000.f);
					ot += dt;
					
					if(view->WantsToBeClosed() || rendererErrorOccured){
						view->Closing();
						running = false;
						break;
					}
					
					int modState = GetModState();
					if(modState & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)){
						if(!lastCtrl){
							view->KeyEvent("Control", true);
							lastCtrl = true;
						}
					}else{
						if(lastCtrl){
							view->KeyEvent("Control", false);
							lastCtrl = false;
						}
					}
					
					if(modState & (KMOD_SHIFT | KMOD_LSHIFT | KMOD_RSHIFT)){
						if(!lastShift){
							view->KeyEvent("Shift", true);
							lastShift = true;
						}
					}else{
						if(lastShift){
							view->KeyEvent("Shift", false);
							lastShift = false;
						}
					}
					
					if(modState & KMOD_GUI){
						if(!lastGui){
							view->KeyEvent("Meta", true);
							lastGui = true;
						}
					}else{
						if(lastGui){
							view->KeyEvent("Meta", false);
							lastGui = false;
						}
					}
					
					if(modState & KMOD_ALT){
						if(!lastAlt){
							view->KeyEvent("Alt", true);
							lastAlt = true;
						}
					}else{
						if(lastAlt){
							view->KeyEvent("Alt", false);
							lastAlt = false;
						}
					}
					
					
					// TODO: support text inputing for cg_smp runner.
					//       see SDLRunner.cpp and grep SDL_StartTextInput
					
					//Fl::check();
				}
				
				cliQueue = NULL;
				currentView = NULL;
			}catch(const std::exception& ex){
				SPLog("Exiting Client Event Loop due to exception:\n%s",
					  ex.what());
				cliQueue = NULL;
				currentView = NULL;
				clientError = ex.what();
			}
		}
		
		
	}
}
