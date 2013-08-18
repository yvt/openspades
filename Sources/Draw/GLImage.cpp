//
//  GLImage.cpp
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLImage.h"
#include "../Core/Bitmap.h"
#include "../Core/Debug.h"

namespace spades {
	namespace draw {
		GLImage::GLImage(IGLDevice::UInteger texObj,
						 IGLDevice *dev, float w, float h,
						 bool autoDelete): tex(texObj),
		device(dev),
		width(w), height(h),
		autoDelete(autoDelete){
			SPADES_MARK_FUNCTION();
		}
		GLImage::~GLImage(){
			SPADES_MARK_FUNCTION();
			if(autoDelete)
				device->DeleteTexture(tex);
		}
		void GLImage::Bind(IGLDevice::Enum target) {
			SPADES_MARK_FUNCTION();
			device->BindTexture(target, tex);
		}
		
		GLImage *GLImage::FromBitmap(spades::Bitmap *bmp,
									 spades::draw::IGLDevice *dev) {
			SPADES_MARK_FUNCTION();
			
			IGLDevice::UInteger tex;
			tex = dev->GenTexture();
			dev->BindTexture(IGLDevice::Texture2D, tex);
			dev->TexImage2D(IGLDevice::Texture2D, 0,
							IGLDevice::RGBA, bmp->GetWidth(), bmp->GetHeight(),
							0, IGLDevice::RGBA,
							IGLDevice::UnsignedByte, bmp->GetPixels());
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMagFilter,
							  IGLDevice::Linear);
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMinFilter,
							  IGLDevice::LinearMipmapNearest);
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapS,
							  IGLDevice::Repeat);
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapT,
							  IGLDevice::Repeat);
			dev->GenerateMipmap(IGLDevice::Texture2D);
			return new GLImage(tex, dev, bmp->GetWidth(), bmp->GetHeight());
		}
		
		void GLImage::SubImage(spades::Bitmap *bmp, int x, int y){
			Bind(IGLDevice::Texture2D);
			device->TexSubImage2D(IGLDevice::Texture2D, 0,
								  x, y, bmp->GetWidth(), bmp->GetHeight(),
								  IGLDevice::RGBA, IGLDevice::UnsignedByte,
								  bmp->GetPixels());
		}
	}
}

