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
#include <Core/RefCountedObject.h>

namespace spades {
	namespace client {
		class FontManager : public RefCountedObject {
		public:
			FontManager(IRenderer *);

			IFont *GetSquareDesignFont() { return squareDesignFont; }
			IFont *GetLargeFont() { return largeFont; }
			IFont *GetMediumFont() { return mediumFont; }
			IFont *GetHeadingFont() { return headingFont; }
			IFont *GetGuiFont() { return guiFont; }

		protected:
			~FontManager() override;

		private:
			Handle<IFont> squareDesignFont;
			Handle<IFont> largeFont;
			Handle<IFont> mediumFont;
			Handle<IFont> headingFont;
			Handle<IFont> guiFont;
		};
	}
}
