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

#include "SWImage.h"
#include <Core/FileManager.h>
#include <Core/IStream.h>

namespace spades {
	namespace draw {
		SWImage::SWImage(Bitmap *m):
		rawBmp(m),
		w(static_cast<float>(m->GetWidth())),
		h(static_cast<float>(m->GetHeight())),
		iw(1.f / w), ih(1.f / h)
		{
			
		}
		
		SWImage::~SWImage() {
		}
		
		SWImageManager::~SWImageManager() {
			for(auto it = images.begin(); it != images.end(); it++)
				it->second->Release();
		}
		
		SWImage *SWImageManager::RegisterImage(const std::string &name) {
			auto it = images.find(name);
			if(it == images.end()) {
				Handle<Bitmap> vm;
				vm.Set(Bitmap::Load(name), false);
				auto *m = CreateImage(vm);
				images.insert(std::make_pair(name, m));
				m->AddRef();
				return m;
			}else{
				auto *image = it->second;
				image->AddRef();
				return image;
			}
		}
		
		SWImage *SWImageManager::CreateImage(Bitmap *vm) {
			return new SWImage(vm);
		}
	}
}
