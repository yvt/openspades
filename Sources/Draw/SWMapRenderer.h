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

#include <memory>
#include <vector>

#include "SWFeatureLevel.h"
#include "SWFeatureLevel.h"
#include <Client/SceneDefinition.h>
#include <Core/Math.h>
#include <Core/MiniHeap.h>
#include <Core/RefCountedObject.h>

namespace spades {
	namespace client {
		class GameMap;
	}

	class Bitmap;

	namespace draw {
		class SWRenderer;
		class SWMapRenderer {
			struct Line;
			struct LinePixel;

			int w, h;
			SWRenderer *renderer;

			SWFeatureLevel level;
			client::SceneDefinition sceneDef;
			Handle<client::GameMap> map;
			Bitmap *frameBuf;
			float *depthBuf;
			std::vector<Line> lines;
			std::vector<MiniHeap::Ref> rle;
			std::vector<size_t> rleLen;

			int lineResolution;

			typedef int8_t RleData;
			std::vector<RleData> rleBuf;

			MiniHeap rleHeap;

			template <SWFeatureLevel level>
			void BuildLine(Line &line, float minPitch, float maxPitch);
			void BuildRle(int x, int y, std::vector<RleData> &);

			template <SWFeatureLevel level, int undersamp>
			void RenderFinal(float yawMin, float yawMax, unsigned int numLines,
			                 unsigned int threadId, unsigned int numThreads);

			template <SWFeatureLevel level>
			void RenderInner(const client::SceneDefinition &, Bitmap *fb, float *depthBuffer);

		public:
			SWMapRenderer(SWRenderer *r, client::GameMap *, SWFeatureLevel level);
			~SWMapRenderer();

			void Render(const client::SceneDefinition &, Bitmap *fb, float *depthBuffer);

			void UpdateRle(int x, int y);
		};
	}
}
