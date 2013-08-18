//
//  GLFlatMapRenderer.cpp
//  OpenSpades
//
//  Created by yvt on 7/20/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLFlatMapRenderer.h"
#include "GLRenderer.h"
#include "../Client/GameMap.h"
#include "../Core/Debug.h"
#include "../Core/Bitmap.h"
#include "GLImage.h"

namespace spades {
	namespace draw {
		GLFlatMapRenderer::GLFlatMapRenderer(GLRenderer *r,
											 client::GameMap *m):
		renderer(r), map(m){
			SPADES_MARK_FUNCTION();
			
			chunkRows = m->Height() >> ChunkBits;
			chunkCols = m->Width() >> ChunkBits;
			for(int i = 0; i < chunkRows * chunkCols; i++)
				chunkInvalid.push_back(false);
			
			Bitmap *bmp = GenerateBitmap(0, 0, m->Width(), m->Height());
			try{
				image = static_cast<GLImage *>(renderer->CreateImage(bmp));
				delete bmp;
			}catch(...){
				delete bmp;
				throw;
			}
			
			image->Bind(IGLDevice::Texture2D);
			IGLDevice *dev = renderer->GetGLDevice();
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMagFilter,
							  IGLDevice::Nearest);
			dev->TexParamater(IGLDevice::Texture2D,
							  IGLDevice::TextureMinFilter,
							  IGLDevice::Nearest);
			
		}
		
		GLFlatMapRenderer::~GLFlatMapRenderer() {
			delete image;
		}
		
		Bitmap *GLFlatMapRenderer::GenerateBitmap(int mx, int my, int w, int h){
			SPADES_MARK_FUNCTION();
			Bitmap *bmp = new Bitmap(w, h);
			try{
				uint32_t *pixels = bmp->GetPixels();
				
				for(int y = 0; y < h; y++){
					for(int x = 0; x < w; x++){
						int px = mx + x, py = my + y;
						for(int z = 0 ; z < 64; z++){
							if(map->IsSolid(px, py, z)){
								uint32_t col = map->GetColor(px, py, z);
								col |= 0xff000000UL;
								*pixels = col;
								break;
							}
						}
						pixels++;
					}
				}
			}catch(...){
				delete bmp;
				throw;
			}
			return bmp;
		}
		
		void GLFlatMapRenderer::GameMapChanged(int x, int y, int z,
											   client::GameMap *map){
			if(map != this->map)
				return;
			
			SPAssert(x >= 0); SPAssert(x < map->Width());
			SPAssert(y >= 0); SPAssert(y < map->Height());
			SPAssert(z >= 0); SPAssert(z < map->Depth());
			
			int chunkX = x >> ChunkBits;
			int chunkY = y >> ChunkBits;
			int chunkId = chunkX + chunkY * chunkCols;
			SPAssert(chunkId >= 0);
			SPAssert(chunkId < chunkCols * chunkRows);
			chunkInvalid[chunkId] = true;
		}
		
		void GLFlatMapRenderer::Draw(const AABB2& dest,
									 const AABB2& src) {
			SPADES_MARK_FUNCTION();
			
			// update chunks
			for(size_t i = 0; i < chunkInvalid.size(); i++){
				if(!chunkInvalid[i]) continue;
				int chunkX = ((int)i) % chunkCols;
				int chunkY = ((int)i) / chunkCols;
				
				Bitmap *bmp = GenerateBitmap(chunkX * ChunkSize,
											 chunkY * ChunkSize,
											 ChunkSize, ChunkSize);
				try{
					image->SubImage(bmp, chunkX * ChunkSize, chunkY * ChunkSize);
					delete bmp;
				}catch(...){
					delete bmp;
					throw;
				}
				chunkInvalid[i] = false;
			}
			
			renderer->DrawImage(image, dest, src);
		}
	}
}
