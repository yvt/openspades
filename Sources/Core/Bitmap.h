//
//  Bitmap.h
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <stdint.h>
#include <string>
#include "Debug.h"

namespace spades {
	class IStream;
	class Bitmap {
		int w, h;
		uint32_t *pixels;
		
	public:
		Bitmap(int w, int h);
		~Bitmap();
		
		static Bitmap *Load(const std::string&);
		void Save(const std::string&);
		
		uint32_t *GetPixels() { return pixels; }
		int GetWidth() { return w; }
		int GetHeight() { return h; }
		
	};
}
