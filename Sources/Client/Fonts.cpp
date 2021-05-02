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
#include <regex>

#include "FTFont.h"
#include "FontData.h"
#include "Fonts.h"
#include "IRenderer.h"
#include "Quake3Font.h"
#include <Core/FileManager.h>

namespace spades {
	namespace client {
		namespace {
			std::regex const g_fontNameRe(".*\\.(?:otf|ttf|ttc)", std::regex::icase);

			struct GlobalFontInfo {
				std::shared_ptr<ngclient::FTFontSet> guiFontSet;

				GlobalFontInfo() {
					SPLog("Loading built-in fonts");

					guiFontSet = std::make_shared<ngclient::FTFontSet>();

					if (FileManager::FileExists("Gfx/Fonts/AlteDIN1451.ttf")) {
						guiFontSet->AddFace("Gfx/Fonts/AlteDIN1451.ttf");
						SPLog("Font 'Alte DIN 1451' loaded");
					} else {
						SPLog("Font 'Alte DIN 1451' was not found");
					}

					// Preliminary custom font support
					auto files = FileManager::EnumFiles("Fonts");
					for (const auto &name : files) {
						if (!std::regex_match(name, g_fontNameRe)) {
							continue;
						}
						SPLog("Loading custom font '%s'", name.c_str());

						auto path = "Fonts/" + name;
						guiFontSet->AddFace(path);
					}
				}

				static GlobalFontInfo &GetInstance() {
					static GlobalFontInfo instance;
					return instance;
				}
			};
		} // namespace

		FontManager::FontManager(IRenderer *renderer) {
			{
				auto font = Handle<Quake3Font>::New(
				  renderer,
				  renderer->RegisterImage("Gfx/Fonts/SquareFontBig.png").GetPointerOrNull(),
				  (const int *)SquareFontBigMap, 48, 8.f, true);
				font->SetGlyphYRange(11.f, 37.f);
				SPLog("Font 'SquareFont (Large)' Loaded");
				squareDesignFont = std::move(font).Cast<IFont>();
			}
			largeFont = Handle<ngclient::FTFont>::New(
			              renderer, GlobalFontInfo::GetInstance().guiFontSet, 34.f, 48.f)
			              .Cast<IFont>();
			mediumFont = Handle<ngclient::FTFont>::New(
			               renderer, GlobalFontInfo::GetInstance().guiFontSet, 24.f, 32.f)
			               .Cast<IFont>();
			headingFont = Handle<ngclient::FTFont>::New(
			                renderer, GlobalFontInfo::GetInstance().guiFontSet, 20.f, 26.f)
			                .Cast<IFont>();
			guiFont = Handle<ngclient::FTFont>::New(
			            renderer, GlobalFontInfo::GetInstance().guiFontSet, 16.f, 20.f)
			            .Cast<IFont>();
		}

		FontManager::~FontManager() {}
	} // namespace client
} // namespace spades
