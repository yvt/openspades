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

#include "../Client/IImage.h"
#include "IGLDevice.h"

namespace spades {
	class Bitmap;
	namespace draw {
		class IGLDevice;
		class GLImage: public client::IImage {
			IGLDevice *device;
			IGLDevice::UInteger tex;
			float width, height;
			bool autoDelete;
		public:
			GLImage(IGLDevice::UInteger textureObject,
					IGLDevice *device, float w, float h,
					bool autoDelete = true);
			static GLImage *FromBitmap(Bitmap *, IGLDevice *);
			virtual ~GLImage();
			void Bind(IGLDevice::Enum target);
			
			virtual float GetWidth() { return width; }
			virtual float GetHeight() { return height; }
			
			void SubImage(Bitmap *bmp, int x, int y);
		};
	}
}
