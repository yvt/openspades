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
			
			enum {
				CoarseSize = 8,
				CoarseBits = 3
			};
			
			GLRenderer *renderer;
			IGLDevice *device;
			client::GameMap *map;
			IGLDevice::UInteger texture;
			IGLDevice::UInteger coarseTexture;
			
			int w, h, d;
			
			size_t updateBitmapPitch;
			std::vector<uint32_t> updateBitmap;
			
			std::vector<uint32_t> bitmap;
			std::vector<uint16_t> coarseBitmap;
			
			uint32_t GeneratePixel(int x, int y);
			void MarkUpdate(int x, int y);
		public:
			GLMapShadowRenderer(GLRenderer *renderer, client::GameMap *map);
			~GLMapShadowRenderer();
			
			void GameMapChanged(int x, int y, int z, client::GameMap *);
			
			void Update();
			
			IGLDevice::UInteger GetTexture() { return texture; }
			IGLDevice::UInteger GetCoarseTexture() { return coarseTexture; }
		};
	}
}