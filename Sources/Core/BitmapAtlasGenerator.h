//
//  BitmapAtlasGenerator.h
//  OpenSpades
//
//  Created by yvt on 7/28/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <vector>
#include <map>

namespace spades {
	class Bitmap;
	class BitmapAtlasGenerator{
	public:
		struct Item {
			Bitmap *bitmap;
			int x, y, w, h;
		};
		struct Result {
			std::vector<Item> items;
			Bitmap *bitmap;
		};
	private:
		std::vector<Bitmap *> bmps;
		int MinRectSize();
		int MinWidth();
		int MinHeight();
	public:
		BitmapAtlasGenerator();
		~BitmapAtlasGenerator();
		
		void AddBitmap(Bitmap *);
		Result Pack();
	};
}
