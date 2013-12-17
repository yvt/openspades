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
#include <memory>

#include "SDLRunner.h"

#include <FL/Fl.H> // for FLTK message pumping
#include <Draw/GLRenderer.h>
#include <Client/Client.h>
#include <Audio/ALDevice.h>
#include <Audio/YsrDevice.h>
#include <ctype.h>
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/ConcurrentDispatch.h>
#include <Core/Math.h>

SPADES_SETTING(r_videoWidth, "1024");
SPADES_SETTING(r_videoHeight, "640");
SPADES_SETTING(r_fullscreen, "0");
SPADES_SETTING(r_colorBits, "32");
SPADES_SETTING(r_depthBits, "16");
SPADES_SETTING(r_vsync, "1");
SPADES_SETTING(r_allowSoftwareRendering, "0");
#if defined(WIN32)
SPADES_SETTING(r_audioDriver, "openal");		//until we have ?ysr? for windows..
#else
SPADES_SETTING(r_audioDriver, "ysr");
#endif

namespace spades {
	namespace gui {
		
		SDLRunner::SDLRunner() {
		}
		
		SDLRunner::~SDLRunner() {
			
		}
		
		client::IAudioDevice *SDLRunner::CreateAudioDevice() {
			if(EqualsIgnoringCase(r_audioDriver, "openal")) {
				return new audio::ALDevice();
			}else if(EqualsIgnoringCase(r_audioDriver, "ysr")) {
				return new audio::YsrDevice();
			}else{
				SPRaise("Unknown audio driver name: %s (openal or ysr expected)", r_audioDriver.CString());
			}
		}
		
		std::string SDLRunner::TranslateButton(Uint8 b){
			SPADES_MARK_FUNCTION();
			switch(b){
				case SDL_BUTTON_LEFT: return "LeftMouseButton";
				case SDL_BUTTON_RIGHT: return "RightMouseButton";
				case SDL_BUTTON_MIDDLE: return "MiddleMouseButton";
				case SDL_BUTTON_X1: return "MouseButton4";
				case SDL_BUTTON_X2: return "MouseButton5";
				default: return std::string();
			}
		}
		
		std::string SDLRunner::TranslateKey(const SDL_Keysym &k){
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
                case SDLK_SLASH:
                    return "/";
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
				case SDL_MOUSEWHEEL:
					view->WheelEvent(-event.wheel.x, -event.wheel.y);
					break;
				case SDL_KEYDOWN:
					view->KeyEvent(TranslateKey(event.key.keysym),
									true);
					break;
				case SDL_KEYUP:
					view->KeyEvent(TranslateKey(event.key.keysym),
									false);
					break;
				case SDL_TEXTINPUT:
					view->TextInputEvent(event.text.text);
					break;
				case SDL_TEXTEDITING:
					view->TextEditingEvent(event.edit.text, event.edit.start, event.edit.length);
					break;
				case SDL_WINDOWEVENT:
					if(event.window.type ==  SDL_WINDOWEVENT_FOCUS_GAINED){
						SDL_ShowCursor(0);
						SDL_SetRelativeMouseMode( SDL_TRUE );
						mActive = true;
					}else if( event.window.type ==  SDL_WINDOWEVENT_FOCUS_LOST ) {
						SDL_SetRelativeMouseMode( SDL_FALSE );
						mActive = false;
						SDL_ShowCursor(1);
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
				bool editing = false;
				
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
					
					bool ed = view->AcceptsTextInput();
					if(ed && !editing) {
						SDL_StartTextInput();
					}else if(!ed && editing){
						SDL_StopTextInput();
					}
					editing = ed;
					if(editing){
						AABB2 rt = view->GetTextInputRect();
						SDL_Rect srt;
						srt.x = (int)rt.GetMinX();
						srt.y = (int)rt.GetMinY();
						srt.w = (int)rt.GetWidth();
						srt.h = (int)rt.GetHeight();
						SDL_SetTextInputRect(&srt);
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
				std::string caption;
				{
					caption = PACKAGE_STRING;
#if !NDEBUG
					caption.append( " DEBUG build" );
#endif
#ifdef OPENSPADES_COMPILER_STR
					caption.append( " " OPENSPADES_COMPILER_STR );	//add compiler to window title
#endif
				}
				
				SDL_Window *window;
				
				Uint32 sdlFlags;
				
				sdlFlags = SDL_WINDOW_OPENGL;
				if(r_fullscreen)
					sdlFlags |= SDL_WINDOW_FULLSCREEN;
				
				
				SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
				SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, r_depthBits);
				SDL_GL_SetSwapInterval(r_vsync);
				if(!r_allowSoftwareRendering)
					SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
				/* someday...
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
				SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
				*/
				
				window = SDL_CreateWindow(caption.c_str(),
										  SDL_WINDOWPOS_CENTERED,
										  SDL_WINDOWPOS_CENTERED,
										  r_videoWidth, r_videoHeight, sdlFlags);
				
				if(!window){
					std::string msg = SDL_GetError();
					SPRaise("Failed to create graphics window: %s", msg.c_str());
				}
				
				SDL_SetRelativeMouseMode(SDL_TRUE);
				SDL_ShowCursor(0);
				mActive = true;
				
				{
					SDLGLDevice glDevice(window);
					Handle<draw::GLRenderer> renderer(new draw::GLRenderer(&glDevice), false);
					Handle<client::IAudioDevice> audio(CreateAudioDevice(), false);
					
					RunClientLoop(renderer, audio);
					
				}
			}catch(...){
				SDL_Quit();
				throw;
			}
			
			SDL_Quit();
		}
	}
}
