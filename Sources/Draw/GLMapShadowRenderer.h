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

#pragma once

#include <cstdint>
#include <vector>

#include "IGLDevice.h"

namespace spades {
	namespace client {
		class GameMap;
	}
	namespace draw {
		class GLRenderer;
		class GLRadiosityRenderer;
		/** Generates a shadow map of the game map. */
		class GLMapShadowRenderer {
			friend class GLRadiosityRenderer;

			enum { CoarseSize = 8, CoarseBits = 3 };

			GLRenderer *renderer;
			IGLDevice *device;
			client::GameMap *map;
			IGLDevice::UInteger texture;
			IGLDevice::UInteger coarseTexture;

			int w, h, d;

			size_t updateBitmapPitch;
			std::vector<uint32_t> updateBitmap;

			std::vector<uint32_t> bitmap;
			std::vector<uint32_t> coarseBitmap;

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