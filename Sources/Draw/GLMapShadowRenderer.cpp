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

#include "GLMapShadowRenderer.h"
#include "GLProfiler.h"
#include "GLRadiosityRenderer.h"
#include "GLRenderer.h"
#include "IGLDevice.h"
#include <Client/GameMap.h>
#include <Core/Debug.h>

namespace spades {
	namespace draw {
		GLMapShadowRenderer::GLMapShadowRenderer(GLRenderer &renderer, client::GameMap *map)
		    : renderer(renderer), device(renderer.GetGLDevice()), map(map) {
			SPADES_MARK_FUNCTION();
			texture = device.GenTexture();
			coarseTexture = device.GenTexture();
			device.BindTexture(IGLDevice::Texture2D, texture);
			device.TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGBA, map->Width(), map->Height(),
			                  0, IGLDevice::RGBA, IGLDevice::UnsignedByte, NULL);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                    IGLDevice::Nearest);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                    IGLDevice::Nearest);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS, IGLDevice::Repeat);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT, IGLDevice::Repeat);

			device.BindTexture(IGLDevice::Texture2D, coarseTexture);
			device.TexImage2D(IGLDevice::Texture2D, 0, IGLDevice::RGBA8, map->Width() / CoarseSize,
			                  map->Height() / CoarseSize, 0, IGLDevice::BGRA,
			                  IGLDevice::UnsignedByte, NULL);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMagFilter,
			                    IGLDevice::Nearest);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureMinFilter,
			                    IGLDevice::Nearest);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapS, IGLDevice::Repeat);
			device.TexParamater(IGLDevice::Texture2D, IGLDevice::TextureWrapT, IGLDevice::Repeat);

			w = map->Width();
			h = map->Height();
			d = map->Depth();

			updateBitmapPitch = (w + 31) / 32;
			updateBitmap.resize(updateBitmapPitch * h);

			coarseBitmap.resize((w * h) >> (CoarseBits * 2));

			bitmap.resize(w * h);
			std::fill(updateBitmap.begin(), updateBitmap.end(), 0xffffffffUL);
			std::fill(bitmap.begin(), bitmap.end(), 0xffffffffUL);
		}

		GLMapShadowRenderer::~GLMapShadowRenderer() {
			SPADES_MARK_FUNCTION();

			device.DeleteTexture(texture);
			device.DeleteTexture(coarseTexture);
		}

		void GLMapShadowRenderer::Update() {
			SPADES_MARK_FUNCTION();

			GLProfiler::Context profiler(renderer.GetGLProfiler(), "Terrain Shadow Map");
			GLRadiosityRenderer *radiosity = renderer.GetRadiosityRenderer();

			std::vector<uint8_t> coarseUpdateBitmap;
			coarseUpdateBitmap.resize(coarseBitmap.size());
			std::fill(coarseUpdateBitmap.begin(), coarseUpdateBitmap.end(), 0);

			device.BindTexture(IGLDevice::Texture2D, texture);
			for (size_t i = 0; i < updateBitmap.size(); i++) {
				int y = static_cast<int>(i / updateBitmapPitch);
				int x = static_cast<int>((i - y * updateBitmapPitch) * 32);
				if (updateBitmap[i] == 0)
					continue;

				size_t bitmapPixelPosBase = i * 32;

				uint32_t pixels[32];
				bool modified = false;
				for (int j = 0; j < 32; j++) {
					pixels[j] = GeneratePixel(x + j, y);
					if (bitmap[bitmapPixelPosBase + j] != pixels[j]) {
						if (radiosity) {
							int dist = pixels[j] >> 24;
							radiosity->GameMapChanged(x + j, (y + dist) & (h - 1), dist, map);

							dist = bitmap[bitmapPixelPosBase + j] >> 24;
							radiosity->GameMapChanged(x + j, (y + dist) & (h - 1), dist, map);
						}
						bitmap[bitmapPixelPosBase + j] = pixels[j];
						modified = true;
					}
				}

				if (modified) {
					if (!coarseUpdateBitmap[(x >> CoarseBits) +
					                        (y >> CoarseBits) * (w >> CoarseBits)])
						for (int j = 0; j < 32; j += CoarseSize)
							coarseUpdateBitmap[((x + j) >> CoarseBits) +
							                   (y >> CoarseBits) * (w >> CoarseBits)] = 1;

					device.TexSubImage2D(IGLDevice::Texture2D, 0, x, y, 32, 1, IGLDevice::RGBA,
					                     IGLDevice::UnsignedByte, pixels);
				}

				updateBitmap[i] = 0;
			}

			{
				bool coarseUpdated = false;
				int bx = 0, by = 0;
				for (size_t i = 0; i < coarseUpdateBitmap.size(); i++) {
					if (coarseUpdateBitmap[i]) {
						int minValue = -1, maxValue = 0;

						uint32_t *bmp = bitmap.data();
						bmp += bx + by * w;
						for (int y = 0; y < CoarseSize; y++) {
							for (int x = 0; x < CoarseSize; x++) {
								uint32_t value = bmp[x];
								int depth = (int)(value >> 24);
								if (minValue == -1) {
									minValue = maxValue = depth;
								} else {
									if (depth < minValue)
										minValue = depth;
									if (depth > maxValue)
										maxValue = depth;
								}
							}
							bmp += w;
						}

						uint32_t out = minValue << 16;
						out |= maxValue << 8;
						coarseBitmap[i] = out;

						coarseUpdated = true;
					}
					bx += CoarseSize;
					if (bx >= w) {
						bx = 0;
						by += CoarseSize;
					}
				}
				if (coarseUpdated) {
					GLProfiler::Context profiler(renderer.GetGLProfiler(),
					                             "Coarse Shadow Map Upload");

					device.BindTexture(IGLDevice::Texture2D, coarseTexture);
					device.TexSubImage2D(IGLDevice::Texture2D, 0, 0, 0, w >> CoarseBits,
					                     h >> CoarseBits, IGLDevice::BGRA, IGLDevice::UnsignedByte,
					                     coarseBitmap.data());
				}
			}
		}

		static uint32_t BuildPixel(int distance, uint32_t color, bool side) {
			int r = (uint8_t)(color);
			int g = (uint8_t)(color >> 8);
			int b = (uint8_t)(color >> 16);

			r >>= 2;
			g >>= 2;
			b >>= 2;

			SPAssert(r < 64);
			SPAssert(g < 64);
			SPAssert(b < 64);
			SPAssert(r >= 0);
			SPAssert(g >= 0);
			SPAssert(b >= 0);

			int ex1 = side ? 1 : 0, ex2 = 0, ex3 = 0;

			return r + (g << 8) + (b << 16) + (distance << 24) + (ex1 << 7) + (ex2 << 15) +
			       (ex3 << 23);
		}

		uint32_t GLMapShadowRenderer::GeneratePixel(int x, int y) {
			for (int z = 0; z < d; z++) {
				// z-plane hit
				if (map->IsSolid(x, y, z) && z < 63) {
					return BuildPixel(z, map->GetColor(x, y, z), false);
					// return (uint8_t)z;
				}

				y = y + 1;
				if (y == h)
					y = 0;

				// y-plane hit
				if (map->IsSolid(x, y, z) && z < 63) {
					return BuildPixel(z + 1, map->GetColor(x, y, z), true);
					// return (uint8_t)z + 1;
				}
			}
			return BuildPixel(64, map->GetColor(x, y == h ? 0 : y, 63), false);
		}

		void GLMapShadowRenderer::MarkUpdate(int x, int y) {
			x &= w - 1;
			y &= h - 1;
			updateBitmap[(x >> 5) + y * updateBitmapPitch] |= 1UL << (x & 31);
		}

		void GLMapShadowRenderer::GameMapChanged(int x, int y, int z, client::GameMap *m) {
			MarkUpdate(x, y - z);
			MarkUpdate(x, y - z - 1);
		}
	} // namespace draw
} // namespace spades
