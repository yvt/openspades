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

#include <cstring>
#include <memory>
#include <unordered_map>

#include "IFont.h"
#include "IRenderer.h"
#include <Core/Exception.h>
#include <Core/FileManager.h>
#include <Core/IStream.h>
#include <Draw/SWRenderer.h> // FIXME: better way to check whether linear interpolation is performed

namespace spades {
	namespace client {

		class IImage;

		class FallbackFontManager;
		struct FallbackFont;

		// FallbackFontManager cannot save IImage because multiple images
		// come from different renderers.
		class FallbackFontRenderer : public RefCountedObject {
			std::vector<IImage *> images;
			IRenderer *renderer;
			FallbackFontManager *manager;
			Handle<IImage> whiteImage; // used for last resort
			bool roundSize;

			struct FindResult {
				IImage *img;
				float size;
				float sizeInverse;
				float advance;
				int x, y, w, h;
				int offX, offY;
				FindResult()
				    : img(nullptr),
				      sizeInverse(0.f),
				      advance(0.f),
				      x(0),
				      y(0),
				      w(0),
				      h(0),
				      offX(0),
				      offY(0) {}
			};
			FindResult FindGlyph(uint32_t);

		protected:
			~FallbackFontRenderer();

		public:
			FallbackFontRenderer(IRenderer *renderer, FallbackFontManager *manager);

			void Draw(uint32_t unicode, Vector2 offset, float size, Vector4 color);
			float Measure(uint32_t unicode, float size);
		};

		struct FallbackFont {
		private:
		public:
			struct GlyphInfo {
				uint32_t unicode;
				uint16_t x, y;
				uint8_t w, h;
				uint16_t advance;
				int16_t offX, offY;
			};
			std::string imagePath;
			std::unordered_map<uint32_t, GlyphInfo> glyphs;
			float fontSize;
			float fontSizeInverse;

			FallbackFont(const std::string &path) {
				std::unique_ptr<IStream> s(FileManager::OpenForReading(path.c_str()));
				char buf[17];
				buf[16] = 0;
				if (s->Read(buf, 16) < 16) {
					SPRaise("Reading %s: file truncated", path.c_str());
				}
				if (std::strcmp(buf, "OpenSpadesFontFl")) {
					SPRaise("Reading %s: font file corrupted", path.c_str());
				}

				uint32_t numGlyphs;
				if (s->Read(&numGlyphs, 4) < 4) {
					SPRaise("Reading %s: file truncated", path.c_str());
				}

				uint32_t fontSize;
				if (s->Read(&fontSize, 4) < 4) {
					SPRaise("Reading %s: file truncated", path.c_str());
				}
				this->fontSize = (float)fontSize;
				fontSizeInverse = 1.f / fontSize;

				std::vector<GlyphInfo> infos;
				infos.resize(static_cast<std::size_t>(numGlyphs));
				size_t siz = infos.size() * sizeof(GlyphInfo);
				if (s->Read(infos.data(), siz) < siz) {
					SPRaise("Reading %s: file truncated: trying to read %d byte(s)", path.c_str(),
					        (int)siz);
				}

				for (auto i = infos.begin(); i != infos.end(); ++i) {
					glyphs[i->unicode] = *i;
				}

				// remove .ospfont and add .tga
				std::string p = path.substr(0, path.size() - 8);
				imagePath = p + ".tga";
				if (FileManager::FileExists(imagePath.c_str())) {
					return;
				}
				imagePath = p + ".png";
				if (FileManager::FileExists(imagePath.c_str())) {
					return;
				}
				SPRaise("Reading %s: failed to find %s.(png|tga)", path.c_str(), p.c_str());
			}
		};

		class FallbackFontManager {

			FallbackFontManager() {
				auto files = FileManager::EnumFiles("Gfx/Fonts");
				std::sort(files.begin(), files.end());
				for (auto file = files.begin(); file != files.end(); ++file) {
					if (file->rfind(".ospfont") != file->size() - 8) {
						continue;
					}

					std::string path = "Gfx/Fonts/" + *file;
					auto fnt = new FallbackFont(path);
					fonts.push_back(fnt);
				}
			}

		public:
			std::vector<FallbackFont *> fonts;

			static FallbackFontManager *GetInstance() {
				static FallbackFontManager *inst = nullptr;
				if (inst == nullptr) {
					inst = new FallbackFontManager();
				}
				return inst;
			}
			FallbackFontRenderer *CreateRenderer(IRenderer *r) {
				return new FallbackFontRenderer(r, this);
			}
		};

		FallbackFontRenderer::FallbackFontRenderer(IRenderer *renderer,
		                                           FallbackFontManager *manager)
		    : renderer(renderer), manager(manager) {
			for (auto font = manager->fonts.begin(); font != manager->fonts.end(); ++font) {
				auto imgPath = (*font)->imagePath;
				auto *img = renderer->RegisterImage(imgPath.c_str()); // addref'd
				images.push_back(img);
			}
			whiteImage.Set(renderer->RegisterImage("Gfx/White.tga"), false);

			// SW renderer doesn't perform linear interpolation on
			// rendering images, so size rounding must be done to
			// avoid unreadable texts.
			roundSize = dynamic_cast<draw::SWRenderer *>(renderer) != nullptr;
		}

		FallbackFontRenderer::~FallbackFontRenderer() {
			for (auto img = images.begin(); img != images.end(); ++img) {
				(*img)->Release();
			}
			images.clear();
		}

		FallbackFontRenderer::FindResult FallbackFontRenderer::FindGlyph(uint32_t unicode) {
			FindResult result;
			for (size_t i = 0; i < manager->fonts.size(); i++) {
				auto *font = manager->fonts[i];
				const auto &it = font->glyphs.find(unicode);
				if (it == font->glyphs.end()) {
					continue;
				}
				result.img = images[i];
				result.size = font->fontSize;
				result.sizeInverse = font->fontSizeInverse;
				result.advance = it->second.advance;
				result.x = it->second.x;
				result.y = it->second.y;
				result.w = it->second.w;
				result.h = it->second.h;
				result.offX = it->second.offX;
				result.offY = it->second.offY;
				break;
			}
			return result;
		}

		void FallbackFontRenderer::Draw(uint32_t unicode, spades::Vector2 offset, float size,
		                                spades::Vector4 color) {
			renderer->SetColorAlphaPremultiplied(color);
			float x = offset.x;
			float y = offset.y;

			auto glyph = FindGlyph(unicode);
			if (glyph.img == nullptr) {
				// no glyph found! draw box in the last resort
				IImage *img = whiteImage;
				renderer->DrawImage(img, AABB2(x, y, size, 1.f));
				renderer->DrawImage(img, AABB2(x, y + size - 1.f, size, 1.f));
				renderer->DrawImage(img, AABB2(x, y + 1.f, 1.f, size - 2.f));
				renderer->DrawImage(img, AABB2(x + size - 1.f, y + 1.f, 1.f, size - 2.f));
				return;
			}

			if (glyph.w == 0) {
				// null glyph.
				return;
			}

			float scale = size * glyph.sizeInverse;
			if (roundSize || scale < 1.f) {
				float newScale = std::max(1.f, floorf(scale));
				// vertical-align: baseline
				offset.y += (scale - newScale) * glyph.size;
				scale = newScale;
			}

			AABB2 inRect(glyph.x, glyph.y, glyph.w, glyph.h);
			AABB2 outRect(glyph.offX, glyph.offY, glyph.w, glyph.h);

			// margin to make the border completely transparent
			// (considering the OpenGL's linear interpolation)
			if (!roundSize) {
				inRect.min.x -= 0.5f;
				inRect.min.y -= 0.5f;
				outRect.min.x -= 0.5f;
				outRect.min.y -= 0.5f;
				inRect.max.x += 0.5f;
				inRect.max.y += 0.5f;
				outRect.max.x += 0.5f;
				outRect.max.y += 0.5f;
			}

			outRect.min *= scale;
			outRect.max *= scale;

			outRect.min += offset;
			outRect.max += offset;

			renderer->DrawImage(glyph.img, outRect, inRect);
		}

		float FallbackFontRenderer::Measure(uint32_t unicode, float size) {
			auto glyph = FindGlyph(unicode);
			if (glyph.img == nullptr) {
				// no glyph found! draw box in the last resort
				return size + 1.f;
			}

			float scale = size * glyph.sizeInverse;
			if (roundSize || scale < 1.f) {
				float newScale = std::max(1.f, floorf(scale));
				scale = newScale;
			}
			return glyph.advance * scale;
		}

		IFont::IFont(IRenderer *r) {
			fallback.Set(FallbackFontManager::GetInstance()->CreateRenderer(r), false);
		}

		IFont::~IFont() {
			//---
		}

		float IFont::MeasureFallback(uint32_t unicodeCodePoint, float size) {
			return fallback->Measure(unicodeCodePoint, size);
		}

		void IFont::DrawFallback(uint32_t unicodeCodePoint, spades::Vector2 offset, float size,
		                         spades::Vector4 color) {
			fallback->Draw(unicodeCodePoint, offset, size, color);
		}

		void IFont::DrawShadow(const std::string &message, const Vector2 &offset, float scale,
		                       const Vector4 &color, const Vector4 &shadowColor) {
			Draw(message, offset + MakeVector2(1, 1), scale, shadowColor);
			Draw(message, offset, scale, color);
		}
	}
}
