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

#include <mutex>
#include <vector>

#include <Core/RefCountedObject.h>

namespace spades {
	namespace client {
		class GameMap;
	}
	namespace draw {
		class SWRenderer;
		class SWImage;

		class SWFlatMapRenderer {
			SWRenderer &r;
			Handle<SWImage> img;
			Handle<client::GameMap> map;
			int w, h;
			std::mutex updateInfoLock;
			std::vector<uint32_t> updateMap;
			std::vector<uint32_t> updateMap2;
			bool volatile needsUpdate;

			uint32_t GeneratePixel(int x, int y);

		public:
			SWFlatMapRenderer(SWRenderer &r, Handle<client::GameMap>);
			~SWFlatMapRenderer();

			SWImage &GetImage() {
				Update();
				return *img;
			}

			void Update(bool firstTime = false);
			void SetNeedsUpdate(int x, int y);
		};
	} // namespace draw
} // namespace spades
