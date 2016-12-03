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

#include "GLImage.h"
#include <Core/Bitmap.h>
#include <Core/Debug.h>

namespace spades {
	namespace draw {
		GLImage::GLImage(IGLDevice::UInteger texObj, IGLDevice *dev, float w, float h,
		                 bool autoDelete)
		    : tex(texObj),
		      device(dev),
		      width(w),
		      height(h),
		      autoDelete(autoDelete),
		      invWidth(1.f / w),
		      invHeight(1.f / h) {
			SPADES_MARK_FUNCTION();
			valid = true;
		}
		GLImage::~GLImage() {
			SPADES_MARK_FUNCTION();
			if (valid) {
				Invalidate();
			}
		}
		void GLImage::MakeSureValid() {
			if (!valid) {
				SPRaise("Attempted to use an invalid image.");
			}
		}
		void GLImage::Bind(IGLDevice::Enum target) {
			SPADES_MARK_FUNCTION();
			MakeSureValid();
			device->BindTexture(target, tex);
		}

		GLImage *GLImage::FromBitmap(spades::Bitmap *bmp, spades::draw::IGLDevice *dev) {
			SPADES_MARK_FUNCTION();

			IGLDevice::UInteger tex;
			tex = dev->GenTexture();
			dev->BindTexture(IGLDevice::Texture2D, tex);
			dev->TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGBA, bmp->GetWidth(),
			                bmp->GetHeight(), 0, IGLDevice::RGBA, IGLDevice::UnsignedByte,
			                bmp->GetPixels());
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter, IGLDevice::Linear);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                  IGLDevice::LinearMipmapNearest);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS, IGLDevice::Repeat);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT, IGLDevice::Repeat);
			dev->GenerateMipmap(IGLDevice::Texture2D);
			return new GLImage(tex, dev, bmp->GetWidth(), bmp->GetHeight());
		}

		void GLImage::SubImage(spades::Bitmap *bmp, int x, int y) {
			MakeSureValid();
			Bind(IGLDevice::Texture2D);
			device->TexSubImage2D(IGLDevice::Texture2D, 0, x, y, bmp->GetWidth(), bmp->GetHeight(),
			                      IGLDevice::RGBA, IGLDevice::UnsignedByte, bmp->GetPixels());
		}

		void GLImage::Invalidate() {
			SPADES_MARK_FUNCTION();
			MakeSureValid();
			valid = false;

			if (autoDelete)
				device->DeleteTexture(tex);
		}
	}
}
