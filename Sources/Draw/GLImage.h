//
//  GLImage.h
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

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
