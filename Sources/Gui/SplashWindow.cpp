/*
 Copyright (c) 2016 yvt

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

#include "SplashWindow.h"

#include <Core/Bitmap.h>
#include <Core/MemoryStream.h>
#include "Icon.h"

static const unsigned char splashImage[] = {
#include "SplashImage.inc"
};

namespace spades
{
	SplashWindow::SplashWindow() : window(nullptr), surface(nullptr), startupScreenRequested(false) {

		spades::MemoryStream stream(reinterpret_cast<const char *>(splashImage),
									sizeof(splashImage));
		bmp = spades::Bitmap::Load(stream);

		SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_TIMER);
		window = SDL_CreateWindow("OpenSpades Splash Window", SDL_WINDOWPOS_CENTERED,
								  SDL_WINDOWPOS_CENTERED, bmp->GetWidth(), bmp->GetHeight(),
								  SDL_WINDOW_BORDERLESS);
		if (window == nullptr) {
			SPLog("Creation of splash window failed.");
			return;
		}

		surface = SDL_GetWindowSurface(window);
		if (surface == nullptr) {
			SPLog("Creation of splash window surface failed.");
			SDL_DestroyWindow(window);
			return;
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
		// put splash image
		auto *s = SDL_CreateRGBSurfaceFrom(bmp->GetPixels(), bmp->GetWidth(), bmp->GetHeight(), 32,
										   bmp->GetWidth() * 4, 0xff, 0xff00, 0xff0000, 0);
		SDL_BlitSurface(s, nullptr, surface, nullptr);
		SDL_FreeSurface(s);

		SDL_UpdateWindowSurface(window);
	}

	SplashWindow::~SplashWindow() {
		if (window)
			SDL_DestroyWindow(window);
	}

	void SplashWindow::PumpEvents() {
		SDL_PumpEvents();
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_KEYDOWN:
					switch (e.key.keysym.sym) {
						case SDLK_ESCAPE: throw ExitRequestException();
						case SDLK_SPACE: startupScreenRequested = true; break;
					}
					break;
				case SDL_QUIT: throw ExitRequestException();
			}
		}
	}
}
