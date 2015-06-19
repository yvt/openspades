/*
 Copyright (c) 2013 OpenSpades Developers
 
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

#include "IBitmapCodec.h"
#include "Debug.h"
#include "Exception.h"
#include "IStream.h"
#include "Bitmap.h"
#include <Imports/SDL.h>
#include <string.h>
#include <memory>

namespace spades {
	class SdlImageReader: public IBitmapCodec {
	public:
		virtual std::string GetName() = 0;
		
		virtual SDL_Surface *LoadSdlImage(const std::string& data) = 0;
		
		virtual bool CanLoad() { return true; }
		virtual bool CanSave() { return false; }
		
		virtual bool CheckExtension(const std::string&) = 0;
		
		virtual Bitmap *Load(IStream *stream){
			SPADES_MARK_FUNCTION();
			
			// read all
			std::string data = stream->ReadAllBytes();
			
			auto deleter = [](SDL_Surface *s){SDL_FreeSurface(s);};
			
			// copy to buffer
			std::unique_ptr<SDL_Surface, decltype(deleter)> imgraw(LoadSdlImage(data), deleter);
			if(imgraw == nullptr) {
				SPRaise("SDL surface was not loaded.");
			}
			
			std::unique_ptr<SDL_Surface, decltype(deleter)> img(SDL_ConvertSurfaceFormat(imgraw.get(), SDL_PIXELFORMAT_ABGR8888, 0), deleter);
			if(img == nullptr) {
				SPRaise("SDL surface was loaded, but format conversion failed.");
			}
			
			const unsigned char* inPixels =
			(const unsigned char *)img->pixels;
			int width = img->w;
			int height = img->h;
			int pitch = img->pitch;
			
			Handle<Bitmap> bmp;
			bmp.Set(new Bitmap(width, height), false);
			try{
				unsigned char *outPixels = (unsigned char *)bmp->GetPixels();
				
				if(pitch == width * 4){
					// if the pitch matches the requirement of Bitmap,
					// just use it
					memcpy(outPixels, inPixels, pitch * height);
				} else {
					// convert
					for(int y = 0; y < height; y++){
						memcpy(outPixels, inPixels, width * 4);
						outPixels += width * 4;
						inPixels += pitch;
					}
				}
				return bmp.Unmanage();
			}catch(...){
				throw;
			}
		}
		
		virtual void Save(IStream *, Bitmap *) {
			SPADES_MARK_FUNCTION();
			SPNotImplemented();
		}
	};
	
	class StringSdlRWops {
		SDL_RWops *op;
		std::string str;
	public:
		StringSdlRWops(std::string s):str(s) {
			op = SDL_RWFromConstMem(str.data(), str.size());
		}
		~StringSdlRWops() {
			SDL_RWclose(op);
		}
		operator SDL_RWops *() { return op; }
	};
	
	class SdlImageImageReader: public SdlImageReader {
	public:
		virtual std::string GetName(){
			return "SDL_image Image Reader";
		}
		
		virtual SDL_Surface *LoadSdlImage(const std::string& data) {
			StringSdlRWops ops(data);
			int flags = IMG_INIT_PNG|IMG_INIT_JPG;
			int initted = IMG_Init(flags);
			if((initted & flags) != flags) {
				SPRaise("IMG_Init failed: %s", IMG_GetError());
			}
			auto *s = IMG_Load_RW(ops, 0);
			if(s == nullptr) {
				SPRaise("IMG_Load_RW failed: %s", IMG_GetError());
			}
			return s;
		}
		
		virtual bool CheckExtension(const std::string& fn) {
			return EndsWith(fn, ".png") || EndsWith(fn, ".jpg") || EndsWith(fn, ".tif") || EndsWith(fn, ".bmp");
		}
	} imgReader;
	
	
}

