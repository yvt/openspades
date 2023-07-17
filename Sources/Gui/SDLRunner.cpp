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

#include <cctype>
#include <cstring>
#include <memory>

#include "SDLRunner.h"

#include "Icon.h"
#include "SDLGLDevice.h"
#include <Audio/ALDevice.h>
#include <Audio/NullDevice.h>
#include <Audio/YsrDevice.h>
#include <Client/Client.h>
#include <Core/ConcurrentDispatch.h>
#include <Core/Debug.h>
#include <Core/Disposable.h>
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Core/Math.h>
#include <Core/Settings.h>
#include <Draw/GLRenderer.h>
#include <Draw/SWPort.h>
#include <Draw/SWRenderer.h>
#include <OpenSpades.h>

SPADES_SETTING(r_videoWidth);
SPADES_SETTING(r_videoHeight);
DEFINE_SPADES_SETTING(r_fullscreen, "0");
DEFINE_SPADES_SETTING(r_vsync, "1");
DEFINE_SPADES_SETTING(r_allowSoftwareRendering, "0");
DEFINE_SPADES_SETTING(r_renderer, "gl");
#ifdef __APPLE__
DEFINE_SPADES_SETTING(s_audioDriver, "ysr");
#else
DEFINE_SPADES_SETTING(s_audioDriver, "openal");
#endif
DEFINE_SPADES_SETTING(cl_fps, "0");

namespace spades {
	namespace gui {

		SDLRunner::SDLRunner() : m_hasSystemMenu(false) {}

		SDLRunner::~SDLRunner() {}

		client::IAudioDevice *SDLRunner::CreateAudioDevice() {
			if (EqualsIgnoringCase(s_audioDriver, "openal")) {
				return new audio::ALDevice();
			} else if (EqualsIgnoringCase(s_audioDriver, "ysr")) {
				return new audio::YsrDevice();
			} else if (EqualsIgnoringCase(s_audioDriver, "null")) {
				return new audio::NullDevice();
			} else {
				SPRaise("Unknown audio driver name: %s (openal or ysr expected)",
				        s_audioDriver.CString());
			}
		}

		std::string SDLRunner::TranslateButton(Uint8 b) {
			SPADES_MARK_FUNCTION();
			switch (b) {
				case SDL_BUTTON_LEFT: return "LeftMouseButton";
				case SDL_BUTTON_RIGHT: return "RightMouseButton";
				case SDL_BUTTON_MIDDLE: return "MiddleMouseButton";
				case SDL_BUTTON_X1: return "MouseButton4";
				case SDL_BUTTON_X2: return "MouseButton5";
				default: return std::string();
			}
		}

		std::string SDLRunner::TranslateKey(const SDL_Keysym &k) {
			SPADES_MARK_FUNCTION();

			switch (k.sym) {
				case SDLK_ESCAPE: return "Escape";
				case SDLK_LEFT: return "Left";
				case SDLK_RIGHT: return "Right";
				case SDLK_UP: return "Up";
				case SDLK_DOWN: return "Down";
				case SDLK_SPACE: return " ";
				case SDLK_TAB: return "Tab";
				case SDLK_BACKSPACE: return "BackSpace";
				case SDLK_DELETE: return "Delete";
				case SDLK_RETURN: return "Enter";
				case SDLK_SLASH: return "/";
				default: return std::string(SDL_GetScancodeName(k.scancode));
			}
		}

		int SDLRunner::GetModState() { return SDL_GetModState(); }

		void SDLRunner::ProcessEvent(SDL_Event &event, View &view) {
			switch (event.type) {
				case SDL_QUIT:
					view.Closing();
					// running = false;
					break;
				case SDL_MOUSEBUTTONDOWN:
					view.KeyEvent(TranslateButton(event.button.button), true);
					break;
				case SDL_MOUSEBUTTONUP:
					view.KeyEvent(TranslateButton(event.button.button), false);
					break;
				case SDL_MOUSEMOTION:
					if (m_active) {
						if (view.NeedsAbsoluteMouseCoordinate()) {
							view.MouseEvent(event.motion.x, event.motion.y);
						} else {
							view.MouseEvent(event.motion.xrel, event.motion.yrel);
						}
					}
					break;
				case SDL_MOUSEWHEEL: view.WheelEvent(-event.wheel.x, -event.wheel.y); break;
				case SDL_KEYDOWN:
					if (!event.key.repeat) {
						if (event.key.keysym.sym == SDLK_RETURN &&
						    event.key.keysym.mod & (KMOD_LALT | KMOD_RALT)) {
							SDL_Window *window = SDL_GetWindowFromID(event.key.windowID);

							// Toggle fullscreen mode
							if (r_fullscreen) {
								if (!SDL_SetWindowFullscreen(window, 0)) {
									r_fullscreen = 0;
								} else {
									SPLog("Couldn't exit fullscreen mode: %s", SDL_GetError());
								}
							} else {
								if (!SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN)) {
									r_fullscreen = 1;
								} else {
									SPLog("Couldn't enter fullscreen mode: %s", SDL_GetError());
								}
							}
							return;
						}
						view.KeyEvent(TranslateKey(event.key.keysym), true);
					}
					break;
				case SDL_KEYUP: view.KeyEvent(TranslateKey(event.key.keysym), false); break;
				case SDL_TEXTINPUT: view.TextInputEvent(event.text.text); break;
				case SDL_TEXTEDITING:
					view.TextEditingEvent(event.edit.text, event.edit.start, event.edit.length);
					break;
				case SDL_WINDOWEVENT:
					if (event.window.type == SDL_WINDOWEVENT_FOCUS_GAINED) {
						SDL_ShowCursor(0);
						SDL_SetRelativeMouseMode(SDL_TRUE);
						m_active = true;
					} else if (event.window.type == SDL_WINDOWEVENT_FOCUS_LOST) {
						SDL_SetRelativeMouseMode(SDL_FALSE);
						m_active = false;
						SDL_ShowCursor(1);
					}
					break;
				default: break;
			}
		}

		void SDLRunner::RunClientLoop(spades::client::IRenderer *renderer,
		                              spades::client::IAudioDevice *audio) {
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

				while (running) {
					SDL_Event event;

					DispatchQueue::GetThreadQueue()->ProcessQueue();

					Uint32 dt = SDL_GetTicks() - ot;

					if ((float)cl_fps != 0) {
						// Limit the frame rate
						Uint32 desiredDelay = static_cast<Uint32>(1000.0f / (float)cl_fps);
						desiredDelay = std::max<Uint32>(std::min<Uint32>(desiredDelay, 200), 1);
						if (dt < desiredDelay) {
							SDL_Delay(desiredDelay - dt);

							// Remeasure the time delta
							dt = SDL_GetTicks() - ot;
						}
					}

					ot += dt;
					if ((int32_t)dt > 0) {
						view->RunFrame((float)dt / 1000.f);
						view->RunFrameLate((float)dt / 1000.f);
					}

					if (view->WantsToBeClosed()) {
						view->Closing();
						SPLog("Close requested by Client");
						break;
					}

					int modState = GetModState();
					if (modState & (KMOD_CTRL | KMOD_LCTRL | KMOD_RCTRL)) {
						if (!lastCtrl) {
							view->KeyEvent("Control", true);
							lastCtrl = true;
						}
					} else {
						if (lastCtrl) {
							view->KeyEvent("Control", false);
							lastCtrl = false;
						}
					}

					if (modState & KMOD_SHIFT) {
						if (!lastShift) {
							view->KeyEvent("Shift", true);
							lastShift = true;
						}
					} else {
						if (lastShift) {
							view->KeyEvent("Shift", false);
							lastShift = false;
						}
					}

					if (modState & KMOD_GUI) {
						if (!lastGui) {
							view->KeyEvent("Meta", true);
							lastGui = true;
						}
					} else {
						if (lastGui) {
							view->KeyEvent("Meta", false);
							lastGui = false;
						}
					}

					if (modState & KMOD_ALT) {
						if (!lastAlt) {
							view->KeyEvent("Alt", true);
							lastAlt = true;
						}
					} else {
						if (lastAlt) {
							view->KeyEvent("Alt", false);
							lastAlt = false;
						}
					}

					bool ed = view->AcceptsTextInput();
					if (ed && !editing) {
						SDL_StartTextInput();
					} else if (!ed && editing) {
						SDL_StopTextInput();
					}
					editing = ed;
					if (editing) {
						AABB2 rt = view->GetTextInputRect();
						SDL_Rect srt;
						srt.x = (int)rt.GetMinX();
						srt.y = (int)rt.GetMinY();
						srt.w = (int)rt.GetWidth();
						srt.h = (int)rt.GetHeight();
						SDL_SetTextInputRect(&srt);
					}

					bool ab = view->NeedsAbsoluteMouseCoordinate();
					if (ab != absoluteMouseCoord) {
						absoluteMouseCoord = ab;
						SDL_SetRelativeMouseMode(absoluteMouseCoord ? SDL_FALSE : SDL_TRUE);
					}

					while (SDL_PollEvent(&event)) {
						ProcessEvent(event, *view);
					}
				}

				SPLog("Leaving Client Loop");
			}
		}

		auto SDLRunner::GetRendererType() -> RendererType {
			if (EqualsIgnoringCase(r_renderer, "gl"))
				return RendererType::GL;
			else if (EqualsIgnoringCase(r_renderer, "sw"))
				return RendererType::SW;
			else
				SPRaise("Unknown renderer name: %s", r_renderer.CString());
		}

		class SDLSWPort : public draw::SWPort, public Disposable {
			SDL_Window *wnd;
			SDL_Surface *surface;
			bool adjusted;
			int actualW, actualH;

			Handle<Bitmap> framebuffer;

			void SetFramebufferBitmap() {
				if (adjusted) {
					framebuffer = Handle<Bitmap>::New(actualW, actualH);
				} else {
					framebuffer = Handle<Bitmap>::New(reinterpret_cast<uint32_t *>(surface->pixels),
					                                  surface->w, surface->h);
				}
			}

			void EnsureSurfaceIsValid() {
				if (!surface) {
					SPRaise("The SDL surface associated with this SDLSWPart has already been"
					        "destroyed.");
				}
			}

		protected:
			~SDLSWPort() {
				if (surface && SDL_MUSTLOCK(surface)) {
					SDL_UnlockSurface(surface);
				}
			}

		public:
			SDLSWPort(SDL_Window *wnd) : wnd(wnd), surface(nullptr) {
				surface = SDL_GetWindowSurface(wnd);
				// FIXME: check pixel format
				if (SDL_MUSTLOCK(surface)) {
					SDL_LockSurface(surface);
				}
				actualW = surface->w & ~7;
				actualH = surface->h & ~7;
				if (actualW != surface->w || actualH != surface->h) {
					SPLog("Surface size %dx%d doesn't match the software renderer's"
					      " requirements. Rounded to %dx%d using an intermediate surface.",
					      surface->w, surface->h, actualW, actualH);
					adjusted = true;
					memset(surface->pixels, 0, surface->w * surface->h * 4);
				} else {
					adjusted = false;
				}
				SetFramebufferBitmap();
			}

			void Dispose() override { surface = nullptr; }

			Bitmap &GetFramebuffer() override {
				EnsureSurfaceIsValid();

				return *framebuffer;
			}
			void Swap() override {
				EnsureSurfaceIsValid();

				if (adjusted) {
					int sy = (surface->h - actualH) >> 1;
					int sx = (surface->w - actualW) >> 1;
					uint32_t *outPixels = reinterpret_cast<uint32_t *>(surface->pixels);
					outPixels += sx + sy * (surface->pitch >> 2);

					uint32_t *inPixels = framebuffer->GetPixels();
					for (int y = 0; y < actualH; y++) {

						std::memcpy(outPixels, inPixels, actualW * 4);

						outPixels += surface->pitch >> 2;
						inPixels += actualW;
					}
				}
				if (SDL_MUSTLOCK(surface)) {
					SDL_UnlockSurface(surface);
				}
				SDL_UpdateWindowSurface(wnd);
				if (SDL_MUSTLOCK(surface)) {
					SDL_LockSurface(surface);
					if (!adjusted) {
						SetFramebufferBitmap();
					}
				}
			}
		};

		std::tuple<Handle<client::IRenderer>, Handle<Disposable>>
		SDLRunner::CreateRenderer(SDL_Window *wnd) {
			switch (GetRendererType()) {
				case RendererType::GL: {
					auto glDevice = Handle<SDLGLDevice>::New(wnd).Cast<draw::IGLDevice>();
					auto dummy = Handle<Disposable>::New(); // FIXME
					return std::make_tuple(
					  Handle<draw::GLRenderer>::New(std::move(glDevice)).Cast<client::IRenderer>(),
					  std::move(dummy));
				}
				case RendererType::SW: {
					auto port = Handle<SDLSWPort>::New(wnd).Cast<draw::SWPort>();
					return std::make_tuple(
					  Handle<draw::SWRenderer>::New(port).Cast<client::IRenderer>(),
					  port.Cast<Disposable>());
				}
				default: SPRaise("Invalid renderer type");
			}
		}

		void SDLRunner::Run(int width, int height) {
			SPADES_MARK_FUNCTION();
			SDL_Init(SDL_INIT_VIDEO);
			try {

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
					caption.append(" DEBUG build");
#endif
#ifdef OPENSPADES_COMPILER_STR
					caption.append(" " OPENSPADES_COMPILER_STR); // add compiler to window title
#endif
				}

				SDL_Window *window;
				auto rtype = GetRendererType();

				Uint32 sdlFlags;

				switch (rtype) {
					case RendererType::GL:
						sdlFlags = SDL_WINDOW_OPENGL;
						SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
						SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
						if (!r_allowSoftwareRendering)
							SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

						/* someday...
						 SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
						 SDL_GL_CONTEXT_PROFILE_CORE);
						 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
						 SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
						 */

						break;
					case RendererType::SW: sdlFlags = 0; break;
				}

				if (!m_hasSystemMenu) {
					if (r_fullscreen)
						sdlFlags |= SDL_WINDOW_FULLSCREEN;

#ifdef __MACOSX__
					if (!r_fullscreen)
						sdlFlags |= SDL_WINDOW_BORDERLESS;
#endif
				}

				window = SDL_CreateWindow(caption.c_str(), SDL_WINDOWPOS_CENTERED,
				                          SDL_WINDOWPOS_CENTERED, width, height, sdlFlags);

				if (!window) {
					std::string msg = SDL_GetError();
					SPRaise("Failed to create graphics window: %s", msg.c_str());
				}

#ifdef __APPLE__
#elif __unix
				SDL_Surface *icon = nullptr;
				SDL_RWops *icon_rw = nullptr;
				icon_rw = SDL_RWFromConstMem(g_appIconData, GetAppIconDataSize());
				if (icon_rw != nullptr) {
					icon = IMG_LoadPNG_RW(icon_rw);
					SDL_FreeRW(icon_rw);
				}
				if (icon == nullptr) {
					std::string msg = SDL_GetError();
					SPLog("Failed to load icon: %s", msg.c_str());
				} else {
					SDL_SetWindowIcon(window, icon);
					SDL_FreeSurface(icon);
				}
#endif
				SDL_SetRelativeMouseMode(SDL_FALSE);
				SDL_ShowCursor(0);
				m_active = true;

				{
					Handle<client::IRenderer> renderer;
					Handle<Disposable> windowReference;

					std::tie(renderer, windowReference) = CreateRenderer(window);

					Handle<client::IAudioDevice> audio(CreateAudioDevice(), false);

#ifndef __sun
					if (rtype == RendererType::GL) {
						int vsync = r_vsync;
						if (vsync != 0 && SDL_GL_SetSwapInterval(vsync) != 0) {
							SPRaise("SDL_GL_SetSwapInterval failed: %s", SDL_GetError());
						}
					}
#endif

					RunClientLoop(renderer.GetPointerOrNull(), audio.GetPointerOrNull());

					// `SDL_Window` and its associated resources will be inaccessible
					// past this point. Some referencing objects might be still alive due to
					// the indeterministic nature of AngelScript's tracing GC, so we explicitly
					// break such references right now.
					windowReference->Dispose();
				}
			} catch (...) {
				SDL_Quit();
				throw;
			}

			SDL_Quit();
		}
	} // namespace gui
} // namespace spades
