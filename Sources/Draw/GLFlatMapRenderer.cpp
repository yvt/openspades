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

#include "GLFlatMapRenderer.h"
#include <Client/GameMap.h>
#include <Core/Bitmap.h>
#include <Core/Debug.h>
#include "GLImage.h"
#include "GLRenderer.h"

namespace spades {
	namespace draw {
		GLFlatMapRenderer::GLFlatMapRenderer(GLRenderer *r, client::GameMap *m)
		    : renderer(r), map(m) {
			SPADES_MARK_FUNCTION();

			chunkRows = m->Height() >> ChunkBits;
			chunkCols = m->Width() >> ChunkBits;
			for (int i = 0; i < chunkRows * chunkCols; i++)
				chunkInvalid.push_back(false);

			Handle<Bitmap> bmp(GenerateBitmap(0, 0, m->Width(), m->Height()), false);
			try {
				image = static_cast<GLImage *>(renderer->CreateImage(bmp));
			} catch (...) {
				throw;
			}

			image->Bind(IGLDevice::Texture2D);
			IGLDevice *dev = renderer->GetGLDevice();
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                  IGLDevice::Nearest);
			dev->TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                  IGLDevice::Nearest);
		}

		GLFlatMapRenderer::~GLFlatMapRenderer() { image->Release(); }

		Bitmap *GLFlatMapRenderer::GenerateBitmap(int mx, int my, int w, int h) {
			SPADES_MARK_FUNCTION();
			Handle<Bitmap> bmp(new Bitmap(w, h), false);
			try {
				uint32_t *pixels = bmp->GetPixels();

				for (int y = 0; y < h; y++) {
					for (int x = 0; x < w; x++) {
						int px = mx + x, py = my + y;
						for (int z = 0; z < 64; z++) {
							if (map->IsSolid(px, py, z)) {
								uint32_t col = map->GetColor(px, py, z);
								col |= 0xff000000UL;
								*pixels = col;
								break;
							}
						}
						pixels++;
					}
				}
			} catch (...) {
				throw;
			}
			return bmp.Unmanage();
		}

		void GLFlatMapRenderer::GameMapChanged(int x, int y, int z, client::GameMap *map) {
			if (map != this->map)
				return;

			SPAssert(x >= 0);
			SPAssert(x < map->Width());
			SPAssert(y >= 0);
			SPAssert(y < map->Height());
			SPAssert(z >= 0);
			SPAssert(z < map->Depth());

			int chunkX = x >> ChunkBits;
			int chunkY = y >> ChunkBits;
			int chunkId = chunkX + chunkY * chunkCols;
			SPAssert(chunkId >= 0);
			SPAssert(chunkId < chunkCols * chunkRows);
			chunkInvalid[chunkId] = true;
		}

		void GLFlatMapRenderer::Draw(const AABB2 &dest, const AABB2 &src) {
			SPADES_MARK_FUNCTION();

			// update chunks
			for (size_t i = 0; i < chunkInvalid.size(); i++) {
				if (!chunkInvalid[i])
					continue;
				int chunkX = ((int)i) % chunkCols;
				int chunkY = ((int)i) / chunkCols;

				Handle<Bitmap> bmp(
				  GenerateBitmap(chunkX * ChunkSize, chunkY * ChunkSize, ChunkSize, ChunkSize),
				  false);
				try {
					image->SubImage(bmp, chunkX * ChunkSize, chunkY * ChunkSize);
				} catch (...) {
					throw;
				}
				chunkInvalid[i] = false;
			}

			renderer->DrawImage(image, dest, src);
		}
	}
}
