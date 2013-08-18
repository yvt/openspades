//
//  GLImageManager.cpp
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLImageManager.h"
#include "GLImage.h"
#include "IGLDevice.h"
#include "../Core/Bitmap.h"
#include "../Core/FileManager.h"
#include "../Core/IStream.h"
#include "../Core/Debug.h"

namespace spades {
	namespace draw {
		GLImageManager::GLImageManager(IGLDevice *dev):
		device(dev) {
			SPADES_MARK_FUNCTION();
		}
		
		GLImageManager::~GLImageManager() {
			SPADES_MARK_FUNCTION();
			
			for(std::map<std::string, GLImage *>::iterator it = images.begin(); it != images.end(); it++){
				delete it->second;
			}
		}
		
		GLImage *GLImageManager::RegisterImage(const std::string &name) {
			SPADES_MARK_FUNCTION();
			
			std::map<std::string, GLImage *>::iterator it;
			it = images.find(name);
			if(it == images.end()){
				GLImage *img = CreateImage(name);
				images[name] = img;
				return img;
			}
			return it->second;
		}
		
		GLImage *GLImageManager::CreateImage(const std::string &name) {
			SPADES_MARK_FUNCTION();
			
			Bitmap *bmp;
			bmp = Bitmap::Load(name);
			/*
			IStream *stream = FileManager::OpenForReading(name.c_str());
			try{
				bmp = Bitmap::FromTarga(stream);
			}catch(...){
				delete stream;
				throw;
			}*/
			
			return GLImage::FromBitmap(bmp, device);
		}
		
		
	}
}

