//
//  GLFlatMapRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <vector>
#include "../Core/Math.h"

namespace spades {
	class Bitmap;
	namespace client{
		class GameMap;
	}
	namespace draw {
		class GLRenderer;
		class GLImage;
		class GLFlatMapRenderer{
			enum{
				ChunkSize = 16,
				ChunkBits = 4
			};
			
			GLRenderer *renderer;
			client::GameMap *map;
			std::vector<bool> chunkInvalid;
			
			GLImage *image;
			
			int chunkCols, chunkRows;
			
			Bitmap *GenerateBitmap(int x, int y, int w, int h);
		public:
			GLFlatMapRenderer(GLRenderer *renderer,
							  client::GameMap *map);
			~GLFlatMapRenderer();
			void Draw(const AABB2& dest,
					  const AABB2& src);
			
			void GameMapChanged(int x, int y, int z, client::GameMap *);
		};
	}
}