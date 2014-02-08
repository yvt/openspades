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

#include "Fonts.h"
#include "Quake3Font.h"
#include "IRenderer.h"
#include "FontData.h"

namespace spades {
	namespace client {
		IFont *CreateSquareDesignFont(IRenderer *renderer) {
			auto *designFont = new Quake3Font(renderer,
										renderer->RegisterImage("Gfx/Fonts/UnsteadyOversteer.tga"),
										(const int *)UnsteadyOversteerMap,
										30,
										18);
			designFont->SetGlyphYRange(9.f, 24.f);
			SPLog("Font 'Unsteady Oversteer' Loaded");
			return designFont;
		}
		IFont *CreateLargeFont(IRenderer *renderer) {
			auto *bigTextFont = new Quake3Font(renderer,
										 renderer->RegisterImage("Gfx/Fonts/SquareFontBig.png"),
										 (const int*)SquareFontBigMap,
										 48,
										 8, true);
			bigTextFont->SetGlyphYRange(11.f, 37.f);
			SPLog("Font 'SquareFont (Large)' Loaded");
			return bigTextFont;
		}
		IFont *CreateGuiFont(IRenderer *renderer) {
			auto *font = new Quake3Font(renderer,
										  renderer->RegisterImage("Gfx/Fonts/CMUSansCondensed.png"),
										  (const int*)CMUSansCondensedMap,
										  20,
										  4,
										  true);
			font->SetGlyphYRange(4.f, 16.f);
			SPLog("Font 'CMUSansCondensed' Loaded");
			return font;
		}
	}
}

