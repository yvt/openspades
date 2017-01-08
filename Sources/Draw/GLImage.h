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

#include <Client/IImage.h>
#include "IGLDevice.h"

namespace spades {
	class Bitmap;
	namespace draw {
		class IGLDevice;
		class GLImage : public client::IImage {
			IGLDevice *device;
			IGLDevice::UInteger tex;
			float width, height;
			float invWidth, invHeight;
			bool autoDelete;
			bool valid;
			void MakeSureValid();

		protected:
			~GLImage();

		public:
			GLImage(IGLDevice::UInteger textureObject, IGLDevice *device, float w, float h,
			        bool autoDelete = true);
			static GLImage *FromBitmap(Bitmap *, IGLDevice *);
			void Bind(IGLDevice::Enum target);

			float GetWidth() override { return width; }
			float GetHeight() override { return height; }

			float GetInvWidth() { return invWidth; }
			float GetInvHeight() { return invHeight; }

			void SubImage(Bitmap *bmp, int x, int y);
			void Invalidate();

			void Update(Bitmap &bmp, int x, int y) override { SubImage(&bmp, x, y); }
		};
	}
}
