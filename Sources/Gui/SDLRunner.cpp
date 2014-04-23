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
#include <cstring>

#include "SDLRunner.h"

#include <Draw/GLRenderer.h>
#include <Client/Client.h>
#include <Audio/ALDevice.h>
#include <Audio/YsrDevice.h>
#include <Audio/NullDevice.h>
#include <ctype.h>
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Core/Debug.h>
#include <Core/Settings.h>
#include <Core/ConcurrentDispatch.h>
#include <Core/Math.h>
#include <Draw/SWRenderer.h>
#include <Draw/SWPort.h>

SPADES_SETTING(r_videoWidth, "1024");
SPADES_SETTING(r_videoHeight, "640");
SPADES_SETTING(r_fullscreen, "0");
SPADES_SETTING(r_colorBits, "32");
SPADES_SETTING(r_depthBits, "16");
SPADES_SETTING(r_vsync, "1");
SPADES_SETTING(r_allowSoftwareRendering, "0");
SPADES_SETTING(r_renderer, "gl");
#ifdef __APPLE__
SPADES_SETTING(s_audioDriver, "ysr");
#else
SPADES_SETTING(s_audioDriver, "openal");
#endif

namespace spades {
	namespace gui {
		
		SDLRunner::SDLRunner():
		m_hasSystemMenu(false){
		}
		
		SDLRunner::~SDLRunner() {
			
		}
		
		client::IAudioDevice *SDLRunner::CreateAudioDevice() {
			if(EqualsIgnoringCase(s_audioDriver, "openal")) {
				return new audio::ALDevice();
			}else if(EqualsIgnoringCase(s_audioDriver, "ysr")) {
				return new audio::YsrDevice();
			}else if(EqualsIgnoringCase(s_audioDriver, "null")) {
				return new audio::NullDevice();
			}else{
				SPRaise("Unknown audio driver name: %s (openal or ysr expected)", s_audioDriver.CString());
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
					return "BackSpace";
				case SDLK_DELETE:
					return "Delete";
				case SDLK_RETURN:
					return "Enter";
                		case SDLK_SLASH:
                    			return "/";
				default:
					return std::string( SDL_GetScancodeName( k.scancode ) );
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
						// FIXME: this might fail with cg_smp
						if(view->NeedsAbsoluteMouseCoordinate()) {
							view->MouseEvent(event.motion.x, event.motion.y);
						}else{
							view->MouseEvent(event.motion.xrel, event.motion.yrel);
						}
					}
					break;
				case SDL_MOUSEWHEEL:
					view->WheelEvent(-event.wheel.x, -event.wheel.y);
					break;
				case SDL_KEYDOWN:
					if(!event.key.repeat)
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
				bool lastGui = false;
				bool lastAlt = false;
				bool absoluteMouseCoord = true;
				
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
					
					if(modState & KMOD_SHIFT){
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
					
					bool ab = view->NeedsAbsoluteMouseCoordinate();
					if(ab != absoluteMouseCoord) {
						absoluteMouseCoord = ab;
						SDL_SetRelativeMouseMode(absoluteMouseCoord?SDL_FALSE:SDL_TRUE);
					}
					
					while(SDL_PollEvent(&event)) {
						ProcessEvent(event, view);
					}
				}
				
				SPLog("Leaving Client Loop");
			}
		}
		
		
		auto SDLRunner::GetRendererType() -> RendererType {
			if(EqualsIgnoringCase(r_renderer, "gl"))
				return RendererType::GL;
			else if(EqualsIgnoringCase(r_renderer, "sw"))
				return RendererType::SW;
			else
				SPRaise("Unknown renderer name: %s", r_renderer.CString());
		}
		
		class SDLSWPort: public draw::SWPort {
			SDL_Window *wnd;
			SDL_Surface *surface;
			bool adjusted;
			int actualW, actualH;
			
			Handle<Bitmap> framebuffer;
			void SetFramebufferBitmap() {
				if(adjusted){
					framebuffer.Set
					(new Bitmap(actualW, actualH), false);
				}else{
					framebuffer.Set
					(new Bitmap(reinterpret_cast<uint32_t*>(surface->pixels), surface->w, surface->h), false);
				}
			}
		protected:
			virtual ~SDLSWPort() {
				if(surface && SDL_MUSTLOCK(surface)) {
					SDL_UnlockSurface(surface);
				}
			}
		public:
			SDLSWPort(SDL_Window *wnd):
			wnd(wnd),
			surface(nullptr){
				surface = SDL_GetWindowSurface(wnd);
				// FIXME: check pixel format
				if(SDL_MUSTLOCK(surface)) {
					SDL_LockSurface(surface);
				}
				actualW = surface->w & ~7;
				actualH = surface->h & ~7;
				if(actualW != surface->w ||
				   actualH != surface->h) {
					SPLog("Surface size %dx%d doesn't match the software renderer's"
						  " requirements. Rounded to %dx%d using an intermediate surface.",
						  surface->w, surface->h, actualW, actualH);
					adjusted = true;
					memset(surface->pixels, 0, surface->w * surface->h * 4);
				}else{
					adjusted = false;
				}
				SetFramebufferBitmap();
			}
			virtual Bitmap *GetFramebuffer() {
				return framebuffer;
			}
			virtual void Swap() {
				if(adjusted) {
					int sy = (surface->h - actualH) >> 1;
					int sx = (surface->w - actualW) >> 1;
					uint32_t *outPixels = reinterpret_cast<uint32_t *>(surface->pixels);
					outPixels += sx + sy * (surface->pitch >> 2);
					
					uint32_t *inPixels = framebuffer->GetPixels();
					for(int y = 0; y < actualH; y++) {
						
						std::memcpy(outPixels, inPixels, actualW * 4);
						
						outPixels += surface->pitch >> 2;
						inPixels += actualW;
					}
				}
				if(SDL_MUSTLOCK(surface)) {
					SDL_UnlockSurface(surface);
				}
				SDL_UpdateWindowSurface(wnd);
				if(SDL_MUSTLOCK(surface)) {
					SDL_LockSurface(surface);
					if(!adjusted){
						SetFramebufferBitmap();
					}
				}
			}
			
		};
		
		client::IRenderer *SDLRunner::CreateRenderer(SDL_Window *wnd) {
			switch(GetRendererType()) {
				case RendererType::GL:
				{
					Handle<SDLGLDevice> glDevice(new SDLGLDevice(wnd), false);
					return new draw::GLRenderer(glDevice);
				}
				case RendererType::SW:
				{
					Handle<SDLSWPort> port(new SDLSWPort(wnd), false);
					return new draw::SWRenderer(port);
				}
				default:
					SPRaise("Invalid renderer type");
			}
		}
		
		void SDLRunner::Run(int width, int height) {
			SPADES_MARK_FUNCTION();
			SDL_Init(SDL_INIT_VIDEO);
			try{
				
				{
					SDL_version linked;
					SDL_GetVersion(&linked);
					SPLog("SDL Version: %d.%d.%d %s", linked.major, linked.minor, linked.patch,
						  SDL_GetRevision());
				}
				
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
				auto rtype = GetRendererType();
				
				Uint32 sdlFlags;
				
				switch(rtype) {
					case RendererType::GL:
						sdlFlags = SDL_WINDOW_OPENGL;
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
						
						break;
					case RendererType::SW:
						sdlFlags = 0;
						break;
				}
				
				if(!m_hasSystemMenu) {
					if(r_fullscreen)
						sdlFlags |= SDL_WINDOW_FULLSCREEN;
				
#ifdef __MACOSX__
					if(!r_fullscreen)
						sdlFlags |= SDL_WINDOW_BORDERLESS;
#endif
				}
				
				int w = width;
				int h = height;
				
				window = SDL_CreateWindow(caption.c_str(),
										  SDL_WINDOWPOS_CENTERED,
										  SDL_WINDOWPOS_CENTERED,
										  w, h, sdlFlags);
				
				if(!window){
					std::string msg = SDL_GetError();
					SPRaise("Failed to create graphics window: %s", msg.c_str());
				}
				
#ifdef __APPLE__
#elif __unix
				SDL_Surface *surface = nullptr;
				char filename[] = "Icons/hicolor/16x16/apps/openspades.png";
				if(FileManager::FileExists(filename)) {
					std::unique_ptr<IStream> s(FileManager::OpenForReading(filename));
					char buffer[1024]; // 1 kbyte is enough
					s->Read(buffer, sizeof(buffer));
					SDL_RWops *rwops = nullptr;
					rwops = SDL_RWFromConstMem(buffer, sizeof(buffer));
					if (rwops != nullptr) {
						surface = IMG_LoadPNG_RW(rwops);
						SDL_FreeRW(rwops);
					}
				}
				if(surface == nullptr) {
					std::string msg = SDL_GetError();
					SPLog("Failed to load icon: %s", msg.c_str());
				} else {
					SDL_SetWindowIcon(window, surface);
					SDL_FreeSurface(surface);
				}
#endif
				SDL_SetRelativeMouseMode(SDL_FALSE);
				SDL_ShowCursor(0);
				mActive = true;
				
				{
					Handle<client::IRenderer> renderer(CreateRenderer(window), false);
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
