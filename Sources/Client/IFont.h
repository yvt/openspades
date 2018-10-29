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

#include <Core/Math.h>
#include <Core/RefCountedObject.h>

namespace spades {
	namespace client {
		class FallbackFontRenderer;
		class IRenderer;
		class IFont : public RefCountedObject {
			Handle<FallbackFontRenderer> fallback;

		protected:
			virtual ~IFont();

			float MeasureFallback(uint32_t unicodeCodePoint, float size);

			/** Draws a unicode character using fallback fonts.
			 * @param color Premultiplied alpha color value. */
			void DrawFallback(uint32_t unicodeCodePoint, Vector2 offset, float size, Vector4 color);

		public:
			IFont(IRenderer *);
			virtual Vector2 Measure(const std::string &) = 0;

			/**
			 * Draws a specified string.
			 *
			 * @param offset Specifies the origin point of the rendered string.
			 * @param scale Scaling factor relative to the default size of the font.
			 * @param color A non-premultiplied alpha color value.
			 */
			virtual void Draw(const std::string &, Vector2 offset, float scale, Vector4 color) = 0;
			virtual void DrawShadow(const std::string &message, const Vector2 &offset, float scale,
			                        const Vector4 &color, const Vector4 &shadowColor);
		};
	}
}
