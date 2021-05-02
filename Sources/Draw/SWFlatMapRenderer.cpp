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

#include <algorithm>

#include "SWFlatMapRenderer.h"
#include "SWImage.h"
#include "SWMapRenderer.h"
#include "SWRenderer.h"
#include <Client/GameMap.h>
#include <Core/Debug.h>
#include <Core/Exception.h>

namespace spades {
	namespace draw {
		SWFlatMapRenderer::SWFlatMapRenderer(SWRenderer &r, Handle<client::GameMap> inMap)
		    : r(r), map(std::move(inMap)), w(map->Width()), h(map->Height()), needsUpdate(true) {
			SPADES_MARK_FUNCTION();

			if (w & 31) {
				SPRaise("Map width must be a multiple of 32.");
			}

			img = Handle<SWImage>::New(map->Width(), map->Height());
			updateMap.resize(w * h / 32);
			std::fill(updateMap.begin(), updateMap.end(), 0xffffffff);
			updateMap2.resize(w * h / 32);
			std::fill(updateMap2.begin(), updateMap2.end(), 0xffffffff);

			Update(true);
		}

		SWFlatMapRenderer::~SWFlatMapRenderer() { SPADES_MARK_FUNCTION(); }

		void SWFlatMapRenderer::Update(bool firstTime) {
			SPADES_MARK_FUNCTION();
			{
				std::lock_guard<std::mutex> lock(updateInfoLock);

				if (!needsUpdate)
					return;
				needsUpdate = false;
				updateMap.swap(updateMap2);
				std::fill(updateMap.begin(), updateMap.end(), 0);
			}
			auto *outPixels = img->GetRawBitmap();

			auto *mapRenderer = r.mapRenderer.get();

			int idx = 0;
			for (int y = 0; y < h; y++) {
				for (int x = 0; x < w; x += 32) {
					uint32_t upd = updateMap2[idx];
					if (upd) {
						for (int i = 0; i < 32; i++) {
							if (upd & 1) {
								auto c = GeneratePixel(x + i, y);
								outPixels[i] = c;
								if (!firstTime) {
									mapRenderer->UpdateRle(x + i, y);
									mapRenderer->UpdateRle((x + i + 1) & (w - 1), y);
									mapRenderer->UpdateRle((x + i - 1) & (w - 1), y);
									mapRenderer->UpdateRle(x + i, (y + 1) & (h - 1));
									mapRenderer->UpdateRle(x + i, (y - 1) & (h - 1));
								}
							}
							upd >>= 1;
						}
						// updateMap[idx] = 0;
					}
					outPixels += 32;
					idx++;
				}
			}
		}

		uint32_t SWFlatMapRenderer::GeneratePixel(int x, int y) {
			const int depth = map->Depth();
			for (int z = 0; z < depth; z++) {
				if (map->IsSolid(x, y, z)) {
					uint32_t col = map->GetColor(x, y, z);
					col = (col & 0xff00) | ((col & 0xff) << 16) | ((col & 0xff0000) >> 16);
					col |= 0xff000000;
					return col;
				}
			}
			return 0; // shouldn't reach here for valid maps
		}

		void SWFlatMapRenderer::SetNeedsUpdate(int x, int y) {
			std::lock_guard<std::mutex> lock(updateInfoLock);
			needsUpdate = true;
			updateMap[(x + y * w) >> 5] |= 1 << (x & 31);
		}
	} // namespace draw
} // namespace spades
