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

#pragma once

#include <cstdint>
#include <string>

#include "Debug.h"
#include <Core/RefCountedObject.h>

namespace spades {
	class IStream;
	class Bitmap : public RefCountedObject {
		int w, h;
		uint32_t *pixels;
		bool autoDelete;

	protected:
		~Bitmap();

	public:
		Bitmap(int w, int h);
		Bitmap(uint32_t *pixels, int w, int h);

		static Bitmap *Load(const std::string &);
		static Bitmap *Load(IStream *); // must be seekable
		void Save(const std::string &);

		uint32_t *GetPixels() { return pixels; }
		int GetWidth() { return w; }
		int GetHeight() { return h; }

		uint32_t GetPixel(int x, int y);
		void SetPixel(int x, int y, uint32_t);

		Handle<Bitmap> Clone();
	};
}
