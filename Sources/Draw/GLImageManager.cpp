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

#include "GLImageManager.h"
#include "GLImage.h"
#include "IGLDevice.h"
#include "../Core/Bitmap.h"
#include "../Core/FileManager.h"
#include "../Core/IStream.h"
#include "../Core/Debug.h"
#include "GLRenderer.h"

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
		
		// draw all imaegs so that all textures are resident
		// TODO: call this after all images are loaded
		void GLImageManager::DrawAllImages(GLRenderer *r) {
			if(images.empty())
				return;
			int count = (int)images.size();
			int w = (int)ceil(sqrt((double)count)) + 1;
			int h = (count + w - 1) / w;
			float scrW = r->ScreenWidth();
			float scrH = r->ScreenHeight();
			
			int x = 0, y = 0;
			for(std::map<std::string, GLImage *>::iterator it = images.begin(); it != images.end(); it++){
				GLImage *img = it->second;
				
				r->DrawImage(img, AABB2(scrW * (float)y / (float)w,
										scrH * (float)y / (float)h,
										1.f / (float)w,
										1.f / (float)h));
				
				x++;
				if(x >= w){
					x = 0; y++;
				}
			}
		}
	}
}

