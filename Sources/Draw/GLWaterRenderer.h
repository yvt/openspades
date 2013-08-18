//
//  GLWaterRenderer.h
//  OpenSpades
//
//  Created by yvt on 8/1/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IGLDevice.h"
#include <vector>
#include <stdint.h>

namespace spades {
	namespace client {
		class GameMap;
	}
	namespace draw {
		class GLRenderer;
		class IGLDevice;
		class GLProgram;
		class GLWaterRenderer{
			class IWaveTank;
			class StandardWaveTank;
			class FFTWaveTank;
			
			GLRenderer *renderer;
			IGLDevice *device;
			client::GameMap *map;
			
			IWaveTank *waveTank;
			
			int w, h;
			
			size_t updateBitmapPitch;
			std::vector<uint32_t> updateBitmap;
			
			std::vector<uint32_t> bitmap;
			
			IGLDevice::UInteger texture; // water color
			IGLDevice::UInteger waveTexture; // bumpmap
			
			struct Vertex;
			
			IGLDevice::UInteger buffer;
			IGLDevice::UInteger idxBuffer;
			size_t numIndices;
			
			IGLDevice::UInteger tempFramebuffer;
			IGLDevice::UInteger tempDepthTexture;
			
			GLProgram *program;
			GLProgram *programDepth;
			
			void BuildVertices();
			void MarkUpdate(int x, int y);
		public:
			GLWaterRenderer(GLRenderer *, client::GameMap *map);
			~GLWaterRenderer();
			
			void Render();
			
			void Update(float dt);
			
			void GameMapChanged(int x, int y, int z, client::GameMap *);
			
		};
	}
}