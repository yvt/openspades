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

#include <Imports/SDL.h>
#include <Core/RefCountedObject.h>

namespace spades
{
	class Bitmap;

	/** Thrown when user wants to exit the program while its initialization. */
	class ExitRequestException : public std::exception {
	public:
		ExitRequestException() throw() {}
	};

	class SplashWindow {
		SDL_Window *window;
		SDL_Surface *surface;
		spades::Handle<spades::Bitmap> bmp;
		bool startupScreenRequested;

	public:
		SplashWindow();
		~SplashWindow();

		SDL_Window *GetWindow() { return window; }

		void PumpEvents();

		bool IsStartupScreenRequested() { return startupScreenRequested; }
	};

}
