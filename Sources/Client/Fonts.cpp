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

#include <memory>

#include "FTFont.h"
#include "FontData.h"
#include "Fonts.h"
#include "IRenderer.h"
#include "Quake3Font.h"

namespace spades {
	namespace client {
		namespace {
			Handle<ngclient::FTFontSet> guiFontSet;
		}

		FontManager::FontManager() {
			SPLog("Loading built-in fonts");

			guiFontSet.Set(new ngclient::FTFontSet(), false);
			guiFontSet->AddFace("Gfx/Fonts/AlteDIN1451.ttf");
			SPLog("Font 'Alte DIN 1451' Loaded");
		}

		FontManager::~FontManager() {}

		FontManager &FontManager::GetInstance() {
			static FontManager manager;
			return manager;
		}

		IFont *FontManager::CreateSquareDesignFont(IRenderer *renderer) {
			auto *bigTextFont =
			  new Quake3Font(renderer, renderer->RegisterImage("Gfx/Fonts/SquareFontBig.png"),
			                 (const int *)SquareFontBigMap, 48, 8, true);
			bigTextFont->SetGlyphYRange(11.f, 37.f);
			SPLog("Font 'SquareFont (Large)' Loaded");
			return bigTextFont;
		}
		IFont *FontManager::CreateLargeFont(IRenderer *renderer) {
			auto *font = new ngclient::FTFont(renderer, guiFontSet, 37.f, 40.f);
			return font;
		}
		IFont *FontManager::CreateGuiFont(IRenderer *renderer) {
			auto *font = new ngclient::FTFont(renderer, guiFontSet, 16.f, 20.f);
			return font;
		}
	}
}
