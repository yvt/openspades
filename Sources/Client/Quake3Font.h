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

#include "IFont.h"

#define PROP_SPACE_WIDTH -2

namespace spades {
	namespace client {
		class IRenderer;
		class IImage;
		class Quake3Font : public IFont {

			enum GlyphType { Invalid = 0, Image, Space };
			struct GlyphInfo {
				GlyphType type;
				AABB2 imageRect;
				float advance;
			};

			IRenderer *renderer;
			IImage *tex;
			int glyphHeight;
			std::vector<GlyphInfo> glyphs;
			float spaceWidth;

			float yMin, yMax;

		protected:
			~Quake3Font();

		public:
			Quake3Font(IRenderer *, IImage *texture, const int *map, int glyphHeight,
			           float spaceWidth, bool extended = false);

			Vector2 Measure(const std::string &) override;
			void Draw(const std::string &, Vector2 offset, float scale, Vector4 color) override;
			void SetGlyphYRange(float yMin, float yMax);
		};
	}
}
