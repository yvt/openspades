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
		class IGLDevice;
		class GLProgram;
		class GLSettings;
		class GLWaterRenderer {
			class IWaveTank;
			class StandardWaveTank;
			template <int SizeBits> class FFTWaveTank;

			GLRenderer &renderer;
			IGLDevice &device;
			GLSettings &settings;
			client::GameMap *map;

			std::vector<IWaveTank *> waveTanks;

			int w, h;

			size_t updateBitmapPitch;
			std::vector<uint32_t> updateBitmap;

			std::vector<uint32_t> bitmap;

			IGLDevice::UInteger texture;     // water color
			IGLDevice::UInteger waveTexture; // bumpmap

			struct Vertex;

			IGLDevice::UInteger buffer;
			IGLDevice::UInteger idxBuffer;
			size_t numIndices;

			IGLDevice::UInteger tempFramebuffer;
			IGLDevice::UInteger tempDepthTexture;

			IGLDevice::UInteger occlusionQuery;

			GLProgram *program;

			void BuildVertices();
			void MarkUpdate(int x, int y);

		public:
			GLWaterRenderer(GLRenderer &, client::GameMap *map);
			~GLWaterRenderer();

			static void PreloadShaders(GLRenderer &);

			void Render();

			void Update(float dt);

			void GameMapChanged(int x, int y, int z, client::GameMap *);

			IGLDevice::UInteger GetOcclusionQuery() { return occlusionQuery; }
		};
	} // namespace draw
} // namespace spades
