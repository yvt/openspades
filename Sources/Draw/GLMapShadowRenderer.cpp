//
//  GLMapShadowRenderer.cpp
//  OpenSpades
//
//  Created by yvt on 7/23/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLMapShadowRenderer.h"
#include "../Client/GameMap.h"
#include "IGLDevice.h"
#include "../Core/Debug.h"
#include "GLRadiosityRenderer.h"
#include "GLRenderer.h"

namespace spades{
	namespace draw {
		GLMapShadowRenderer::GLMapShadowRenderer(GLRenderer *renderer,
												 client::GameMap *map):
		renderer(renderer),
		device(renderer->GetGLDevice()), map(map){
			SPADES_MARK_FUNCTION();
			texture = device->GenTexture();
			device->BindTexture(IGLDevice::Texture2D, texture);
			device->TexImage2D(IGLDevice::Texture2D, 0,
							   IGLDevice::RGBA,
							   map->Width(), map->Height(),
							   0, IGLDevice::RGBA, IGLDevice::UnsignedByte,
							   NULL);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMagFilter,
							  IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMinFilter,
							  IGLDevice::Nearest);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapS,
							  IGLDevice::Repeat);
			device->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureWrapT,
							  IGLDevice::Repeat);
			
			w = map->Width();
			h = map->Height();
			d = map->Depth();
			
			updateBitmapPitch = (w + 31) / 32;
			updateBitmap.resize(updateBitmapPitch * h);
			
			bitmap.resize(w * h);
			std::fill(updateBitmap.begin(), updateBitmap.end(),
					  0xffffffffUL);
		}
		
		GLMapShadowRenderer::~GLMapShadowRenderer(){
			SPADES_MARK_FUNCTION();
			
			device->DeleteTexture(texture);
		}
		
		void GLMapShadowRenderer::Update() {
			SPADES_MARK_FUNCTION();
			
			GLRadiosityRenderer *radiosity = renderer->GetRadiosityRenderer();
			
			device->BindTexture(IGLDevice::Texture2D, texture);
			for(size_t i = 0; i < updateBitmap.size(); i++){
				int y = i / updateBitmapPitch;
				int x = (i - y * updateBitmapPitch) * 32;
				if(updateBitmap[i] == 0)
					continue;
				
				size_t bitmapPixelPosBase = i * 32;
				
				uint32_t pixels[32];
				for(int j = 0; j < 32; j++){
					pixels[j] = GeneratePixel(x + j, y);
					if(bitmap[bitmapPixelPosBase + j] != pixels[j]){
						if(radiosity) {
							int dist = pixels[j] >> 24;
							radiosity->GameMapChanged(x + j, (y + dist) & (h-1),
													  dist,
													  map);
							
							dist = bitmap[bitmapPixelPosBase + j] >> 24;
							radiosity->GameMapChanged(x + j, (y + dist) & (h-1),
													  dist,
													  map);
						}
						bitmap[bitmapPixelPosBase + j] = pixels[j];
					}
				}
				
				device->TexSubImage2D(IGLDevice::Texture2D,
									  0, x, y, 32, 1,
									  IGLDevice::RGBA, IGLDevice::UnsignedByte,
									  pixels);
				
				updateBitmap[i] = 0;
			}
		}
		
		static uint32_t BuildPixel(int distance, uint32_t color, bool side) {
			int r = (uint8_t)(color);
			int g = (uint8_t)(color >> 8);
			int b = (uint8_t)(color >> 16);
			
			r>>=2; g>>=2; b>>=2;
			
			SPAssert(r < 64);
			SPAssert(g < 64);
			SPAssert(b < 64);
			SPAssert(r >= 0);
			SPAssert(g >= 0);
			SPAssert(b >= 0);
			
			int ex1 = side ? 1 : 0, ex2 = 0, ex3 = 0;
			
			return r + (g << 8) + (b << 16) + (distance << 24) +
			(ex1 << 7) + (ex2 << 15) + (ex3 << 23);
		}
		
		uint32_t GLMapShadowRenderer::GeneratePixel(int x, int y) {
			for(int z = 0; z < d; z++){
				// z-plane hit
				if(map->IsSolid(x, y, z) && z < 63){
					return BuildPixel(z, map->GetColor(x, y, z), false);
					//return (uint8_t)z;
				}
				
				y = y + 1;
				if(y == h)
					y = 0;
				
				// y-plane hit
				if(map->IsSolid(x, y, z) && z < 63){
					return BuildPixel(z + 1, map->GetColor(x, y, z), true);
					//return (uint8_t)z + 1;
				}
			}
			return BuildPixel(64, map->GetColor(x, y==h?0:y, 63), false);
		}
		
		void GLMapShadowRenderer::MarkUpdate(int x, int y) {
			x &= w - 1;
			y &= h - 1;
			updateBitmap[(x >> 5) + y * updateBitmapPitch] |=
			1UL << (x & 31);
		}
		
		void GLMapShadowRenderer::GameMapChanged(int x,
												 int y,
												 int z,
												 client::GameMap *m){
			MarkUpdate(x, y - z);
			MarkUpdate(x, y - z - 1);
			
		}
	}
}

