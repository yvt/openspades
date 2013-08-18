//
//  GLMapShadowRenderer.h
//  OpenSpades
//
//  Created by yvt on 7/23/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IGLDevice.h"
#include <vector>
#include <stdint.h>

namespace spades {
	namespace client{
		class GameMap;
	}
	namespace draw {
		class GLRenderer;
		class GLRadiosityRenderer;
		/** Generates a shadow map of the game map. */
		class GLMapShadowRenderer {
			friend class GLRadiosityRenderer;
			
			GLRenderer *renderer;
			IGLDevice *device;
			client::GameMap *map;
			IGLDevice::UInteger texture;
			
			int w, h, d;
			
			size_t updateBitmapPitch;
			std::vector<uint32_t> updateBitmap;
			
			std::vector<uint32_t> bitmap;
			
			uint32_t GeneratePixel(int x, int y);
			void MarkUpdate(int x, int y);
		public:
			GLMapShadowRenderer(GLRenderer *renderer, client::GameMap *map);
			~GLMapShadowRenderer();
			
			void GameMapChanged(int x, int y, int z, client::GameMap *);
			
			void Update();
			
			IGLDevice::UInteger GetTexture() { return texture; }
		};
	}
}