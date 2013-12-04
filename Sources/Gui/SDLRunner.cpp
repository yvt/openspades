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

#include <OpenSpades.h>
#include "SDLGLDevice.h"

#include "SDLRunner.h"

#include <FL/Fl.H> // for FLTK message pumping
#include <Draw/GLRenderer.h>
#include <Client/Client.h>
#include <Audio/ALDevice.h>
#include <ctype.h>
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/ConcurrentDispatch.h>

SPADES_SETTING(r_videoWidth, "1024");
SPADES_SETTING(r_videoHeight, "640");
SPADES_SETTING(r_fullscreen, "0");
SPADES_SETTING(r_colorBits, "32");
SPADES_SETTING(r_depthBits, "16");

namespace spades {
	namespace gui {
		
		SDLRunner::SDLRunner() {
		}
		
		SDLRunner::~SDLRunner() {
			
		}
		
		std::string SDLRunner::TranslateButton(Uint8 b){
			SPADES_MARK_FUNCTION();
			switch(b){
				case SDL_BUTTON_LEFT: return "LeftMouseButton";
				case SDL_BUTTON_RIGHT: return "RightMouseButton";
				case SDL_BUTTON_MIDDLE: return "MiddleMouseButton";
				case SDL_BUTTON_WHEELUP: return "WheelUp";
				case SDL_BUTTON_WHEELDOWN: return "WheelDown";
				default: return std::string();
			}
		}
		
		std::string SDLRunner::TranslateKey(const SDL_keysym &k){
			SPADES_MARK_FUNCTION();
			
			
			switch(k.sym){
				case SDLK_ESCAPE:
					return "Escape";
				case SDLK_LEFT:
					return "Left";
				case SDLK_RIGHT:
					return "Right";
				case SDLK_UP:
					return "Up";
				case SDLK_DOWN:
					return "Down";
				case SDLK_SPACE:
					return " ";
				case SDLK_TAB:
					return "Tab";
				case SDLK_BACKSPACE:
				case 127:
					return "BackSpace";
				case SDLK_RETURN:
					return "Enter";
				default:
					if((k.sym >= 0 && k.sym < 128 && isalnum(k.sym))){
						static std::string charKeys[128];
						int ind = k.sym;
						if(charKeys[ind].empty())
							charKeys[ind] += (char)k.sym;
						return charKeys[ind];
					}
					return std::string();
			}
		}
		
		int SDLRunner::GetModState() {
			return SDL_GetModState();
		}
		
		void SDLRunner::ProcessEvent(SDL_Event &event,
									 View *view) {
			switch (event.type) {
				case SDL_QUIT:
					view->Closing();
					//running = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					view->KeyEvent(TranslateButton(event.button.button), true);
					break;
				case SDL_MOUSEBUTTONUP:
					view->KeyEvent(TranslateButton(event.button.button), false);
					break;
				case SDL_MOUSEMOTION:
					if( mActive ) {
						view->MouseEvent(event.motion.xrel, event.motion.yrel);
					}
					break;
				case SDL_KEYDOWN:
					if(event.key.keysym.unicode){
						int uni = event.key.keysym.unicode;
						if(uni > 0 && uni < 128 &&
						   uni != 8 &&	// no backspace
						   uni != 27 &&	// no escape
						   uni != 13 && uni != 10 &&
						   uni != 127){ // no enter
							std::string s;
							s += (char)uni;
							view->CharEvent(s);
						}
					}
					view->KeyEvent(TranslateKey(event.key.keysym),
									true);
					break;
				case SDL_KEYUP:
					view->KeyEvent(TranslateKey(event.key.keysym),
									false);
					break;
				case SDL_ACTIVEEVENT:
					if( event.active.state & (SDL_APPACTIVE|SDL_APPINPUTFOCUS) ) {		//any of the 2 is good
						if(event.active.gain){
							SDL_ShowCursor(0);
							SDL_WM_GrabInput( SDL_GRAB_ON );
							mActive = true;
						}else{
							SDL_WM_GrabInput( SDL_GRAB_OFF );
							mActive = false;
							SDL_ShowCursor(1);
						}
					}
					break;
				default:
					break;
			}
		}
		
		void SDLRunner::RunClientLoop(spades::client::IRenderer *renderer, spades::client::IAudioDevice *audio)
		{
			{
				Handle<View> view(CreateView(renderer, audio), false);
				Uint32 ot = SDL_GetTicks();
				bool running = true;
				
				
				bool lastShift = false;
				bool lastCtrl = false;
				
				SPLog("Starting Client Loop");
				
				while(running){
					SDL_Event event;
					
					
					DispatchQueue::GetThreadQueue()->ProcessQueue();
					
					Uint32 dt = SDL_GetTicks() - ot;
					ot += dt;
					if((int32_t)dt > 0)
						view->RunFrame((float)dt / 1000.f);
					
					if(view->WantsToBeClosed()){
						view->Closing();
						running = false;
						SPLog("Close requested by Client");
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
					
					while(SDL_PollEvent(&event)) {
						ProcessEvent(event, view);
					}
					//Fl::check();
				}
				
				SPLog("Leaving Client Loop");
			}
		}
		
		void SDLRunner::Run() {
			SPADES_MARK_FUNCTION();
			SDL_Init(SDL_INIT_VIDEO);
			try{
				{
					std::string pkg = PACKAGE_STRING;
#if !NDEBUG
					pkg.append( " DEBUG build" );
#endif
#ifdef OPENSPADES_COMPILER_STR
					pkg.append( " " OPENSPADES_COMPILER_STR );	//add compiler to window title
#endif
					SDL_WM_SetCaption(pkg.c_str(), pkg.c_str());
				}
				
				SDL_Surface *surface;
				
				int sdlFlags = SDL_OPENGL | SDL_DOUBLEBUF;
				if(r_fullscreen)
					sdlFlags |= SDL_FULLSCREEN;
				
				// OpenGL core profile: supported by SDL 1.3 or later
				//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
				//SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
				
				SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
				SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, r_depthBits);
				
				surface = SDL_SetVideoMode(r_videoWidth, r_videoHeight, r_colorBits, sdlFlags);
				if(!surface){
					std::string msg = SDL_GetError();
					SPRaise("Failed to initialize video device: %s", msg.c_str());
				}
				
				
				SDL_WM_GrabInput(SDL_GRAB_ON);
				SDL_EnableUNICODE(1);
				SDL_ShowCursor(0);
				mActive = true;
				
				{
					SDLGLDevice glDevice(surface);
					Handle<draw::GLRenderer> renderer(new draw::GLRenderer(&glDevice), false);
					audio::ALDevice audio;
					
					RunClientLoop(renderer, &audio);
					
				}
			}catch(...){
				SDL_Quit();
				throw;
			}
			
			SDL_Quit();
		}
	}
}
