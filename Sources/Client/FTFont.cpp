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

#include <cmath>
#include <ft2build.h>

#include FT_FREETYPE_H
#include FT_BITMAP_H

#include "FTFont.h"

#include <Client/IImage.h>
#include <Client/IRenderer.h>
#include <Core/Bitmap.h>
#include <Core/FileManager.h>
#include <Core/ThreadLocalStorage.h>
#include <Draw/SWRenderer.h>

namespace spades {
	namespace ngclient {
		namespace {
			struct FreeType {
				FT_Library library;
				FreeType() { FT_Init_FreeType(&library); }
				~FreeType() { FT_Done_FreeType(library); }
				operator FT_Library() const { return library; }
			};

			AutoDeletedThreadLocalStorage<FreeType> ft;

			FT_Library GetFreeType() {
				if (ft == nullptr) {
					ft = new FreeType();
				}
				return *ft;
			}
		};
		struct FTFaceWrapper {
			FT_Face face;
			std::string buffer;
			FTFaceWrapper(FT_Face f) : face(f) {}
			~FTFaceWrapper() { FT_Done_Face(face); }
			operator FT_Face() const { return face; }
		};
		struct FTBitmapWrapper {
			FT_Bitmap b;
			FTBitmapWrapper() { FT_Bitmap_New(&b); }
			~FTBitmapWrapper() { FT_Bitmap_Done(GetFreeType(), &b); }
			operator FT_Bitmap *() { return &b; }
			FT_Bitmap *operator->() { return &b; }
		};

		FTFontSet::FTFontSet() { SPADES_MARK_FUNCTION(); }

		FTFontSet::~FTFontSet() { SPADES_MARK_FUNCTION(); }

		void FTFontSet::AddFace(const std::string &fileName) {

			FT_Face face;
			std::string data = FileManager::ReadAllBytes(fileName.c_str());
			auto ret = FT_New_Memory_Face(
			  GetFreeType(), reinterpret_cast<const FT_Byte *>(data.data()), data.size(), 0, &face);
			if (ret) {
				SPRaise("Failed to load font %s: FreeType error %d", fileName.c_str(), ret);
			}

			auto *wr = new FTFaceWrapper(face);
			wr->buffer = std::move(data);
			faces.emplace_back(wr);
		}

		struct BinPlaceResult {
			client::IImage &image;
			int x, y;
			BinPlaceResult(client::IImage &image, int x, int y) : image(image), x(x), y(y) {}
		};

		struct FTFont::Bin {
			int const width, height;

			Handle<client::IImage> image;
			std::list<std::pair<int, int>> skyline;

			Bin(int width, int height, client::IRenderer &r) : width(width), height(height) {
				Handle<Bitmap> tmpbmp(new Bitmap(width, height), false);
				memset(tmpbmp->GetPixels(), 0, tmpbmp->GetWidth() * tmpbmp->GetHeight() * 4);
				image.Set(r.CreateImage(tmpbmp), false);
				skyline.emplace_back(0, 0);
				skyline.emplace_back(width, 0);
			}

			stmp::optional<BinPlaceResult> Place(Bitmap &bmp) {
				int bw = bmp.GetWidth(), bh = bmp.GetHeight();
				if (bw > width || bh > height) {
					SPRaise("Impossible to allocate bitmap of "
					        "%dx%d on %dx%d bin.",
					        bw, bh, width, height);
				}
				auto it = skyline.begin();
				auto it2 = it;
				++it2;

				auto bestIt1 = it, bestIt2 = it2;
				int bestWasted = std::numeric_limits<int>::max();
				int bestMaxY = 0;

				while (it2 != skyline.end() && it != skyline.end()) {
					while (it2 != skyline.end() && it2->first < it->first + bw) {
						++it2;
					}
					if (it2 == skyline.end()) {
						break;
					}

					auto maxY = std::max_element(it, it2, [](const std::pair<int, int> &a,
					                                         const std::pair<int, int> &b) {
						            return a.second < b.second;
						        })->second;

					if (maxY + bh < height) {

						int right = it->first + bw;
						int wasted = 0;
						for (auto it1 = it; it1 != it2;) {
							auto cur = it1++;
							auto segRight = it1->first;
							wasted += std::min(segRight, right) * (maxY - cur->second);
						}

						if (wasted < bestWasted) {
							bestIt1 = it;
							bestIt2 = it2;
							bestWasted = wasted;
							bestMaxY = maxY;
						}
					}

					++it;
				}

				if (bestWasted == std::numeric_limits<int>::max()) {
					// failed
					return stmp::optional<BinPlaceResult>();
				}

				SPAssert(bestIt1 != bestIt2);

				int right = bestIt1->first + bw;
				BinPlaceResult result(*image, bestIt1->first, bestMaxY);
				if (bestIt2->first == right) {
					it = bestIt1;
					++it;
					skyline.erase(it, bestIt2);
					bestIt1->second = bestMaxY + bh;
				} else {
					it = bestIt2;
					--it;
					if (it == bestIt1) {
						it = skyline.emplace(bestIt2, right, bestIt1->second);
						--bestIt2;
					} else {
						--bestIt2;
						bestIt2->first = right;
					}
					it = bestIt1;
					++it;
					skyline.erase(it, bestIt2);
					bestIt1->second = bestMaxY + bh;
				}

				image->Update(bmp, result.x, result.y);
				return result;
			}
		};

		FTFont::FTFont(client::IRenderer *renderer, FTFontSet *fontSet, float height,
		               float lineHeight)
		    : client::IFont(renderer),
		      renderer(renderer),
		      lineHeight(lineHeight),
		      height(height),
		      fontSet(fontSet) {
			SPADES_MARK_FUNCTION();

			SPAssert(renderer);
			SPAssert(fontSet);

			binSize = 256;
			int targetSize = static_cast<int>(std::min(height * 4.f, 4000.f));
			while (binSize < targetSize) {
				binSize <<= 1;
			}

			baselineY = std::floor(height * 1.f);

			rendererIsLowQuality = dynamic_cast<draw::SWRenderer *>(renderer);

			bins.emplace_back(binSize, binSize, *renderer);
		}

		FTFont::~FTFont() { SPADES_MARK_FUNCTION(); }

		FTFont::Glyph *FTFont::GetGlyph(uint32_t code) {
			auto it = glyphMap.find(code);
			if (it != glyphMap.end()) {
				auto ref = it->second;
				return &ref.get();
			}
			for (const auto &face : fontSet->faces) {
				auto cId = FT_Get_Char_Index(*face, code);
				if (cId != 0) {
					auto it2 = glyphs.find(std::make_pair<FT_Face>(*face, cId));
					if (it2 == glyphs.end()) {

						FT_Set_Char_Size(*face, 0, static_cast<FT_F26Dot6>(height * 64.f), 72, 72);

						Glyph g;
						g.face = *face;
						g.charIndex = cId;
						FT_Load_Glyph(*face, cId, FT_LOAD_NO_HINTING);
						const auto &adv = g.face->glyph->advance;
						g.advance = Vector2(adv.x, adv.y) / (64.f);

						auto it3 =
						  glyphs.emplace(std::make_pair<FT_Face>(*face, cId), std::move(g));
						glyphMap.emplace(code, it3.first->second);
						return &it3.first->second;
					} else {
						return &it2->second;
					}
				}
			}
			return nullptr;
		}

		template <class T, class T2, class T3>
		void FTFont::SplitTextIntoGlyphs(const std::string &str, T onGlyph, T3 onFallback,
		                                 T2 onLineBreak) {
			// FIXME: ligatures?
			std::size_t i = 0;
			uint32_t breakCode = 0;
			while (i < str.size()) {
				std::size_t advance;
				auto code = GetCodePointFromUTF8String(str, i, &advance);
				i += advance;
				if (code == 10 || code == 13) {
					if (breakCode == 0)
						breakCode = code;
					if (code == breakCode) {
						onLineBreak();
					}
					continue;
				}

				auto *g = GetGlyph(code);
				if (g) {
					onGlyph(*g);
				} else {
					onFallback(code);
				}
			}
		}

		Vector2 FTFont::Measure(const std::string &str) {
			SPADES_MARK_FUNCTION();

			float maxWidth = 0.f;
			float x = 0.f;
			int lines = 1;

			SplitTextIntoGlyphs(str,
			                    [&](Glyph &g) {
				                    x += g.advance.x;
				                    maxWidth = std::max(x, maxWidth);
				                },
			                    [&](uint32_t codepoint) {
				                    x += MeasureFallback(codepoint, height);
				                    maxWidth = std::max(x, maxWidth);
				                },
			                    [&]() {
				                    ++lines;
				                    x = 0.f;
				                });

			return Vector2(maxWidth, lines * lineHeight);
		}

		void FTFont::RenderGlyph(Glyph &g) {
			if (g.image)
				return;

			FT_Set_Char_Size(g.face, 0, static_cast<FT_F26Dot6>(height * 64.f), 72, 72);
			FT_Load_Glyph(g.face, g.charIndex, FT_LOAD_NO_HINTING);
			FT_Render_Glyph(g.face->glyph, FT_RENDER_MODE_NORMAL);

			auto &bmp = g.face->glyph->bitmap;
			FTBitmapWrapper outbmp;
			FT_Bitmap_Convert(GetFreeType(), &bmp, outbmp, 1);

			SPAssert(outbmp->pixel_mode == FT_PIXEL_MODE_GRAY);

			Handle<Bitmap> spbmp(new Bitmap(outbmp->width + 1, outbmp->rows + 1), false);

			memset(spbmp->GetPixels(), 0, 4 * spbmp->GetWidth() * spbmp->GetHeight());

			for (int y = 0; y < outbmp->rows; ++y) {
				const auto *inpixs = outbmp->buffer + y * outbmp->width;
				auto *outpixs = spbmp->GetPixels();
				outpixs += y * spbmp->GetWidth();
				for (int x = 0; x < outbmp->width; ++x) {
					uint32_t v = *inpixs;
					v = (v << 24) | 0xffffff;
					*outpixs = v;
					outpixs++;
					inpixs++;
				}
			}

			// allocate bin
			auto result = bins.back().Place(*spbmp);
			if (!result) {
				// bin full
				bins.emplace_back(binSize, binSize, *renderer);
				result = bins.back().Place(*spbmp);
				SPAssert(result);
			}

			AABB2 bounds((*result).x, (*result).y, spbmp->GetWidth() - 1, spbmp->GetHeight() - 1);

			Vector2 offs(g.face->glyph->bitmap_left, baselineY - g.face->glyph->bitmap_top);

			g.image.reset((*result).image, bounds, offs);
			g.bmp = spbmp;
		}

		void FTFont::RenderBlurGlyph(Glyph &g) {
			RenderGlyph(g);
			if (g.blurImage)
				return;

			enum { KernelSize = 6 };

			auto &orig = *g.bmp;
			Handle<Bitmap> newbmp(
			  new Bitmap(orig.GetWidth() + KernelSize, orig.GetHeight() + KernelSize), false);

			int const origW = orig.GetWidth();
			int const origH = orig.GetHeight();
			int const newW = newbmp->GetWidth();
			int const newH = newbmp->GetHeight();

			std::vector<uint8_t> buf;
			buf.resize(newW + KernelSize);
			for (int y = 0; y < newH; ++y) {
				int iy = y - (KernelSize >> 1);
				auto *pixels = newbmp->GetPixels() + y * newW;
				if (iy >= 0 && iy < origH) {
					{
						auto *inp = orig.GetPixels() + iy * origW;
						auto *outp = buf.data() + KernelSize;
						for (int x = 0; x < origW; ++x) {
							*(outp++) = *(inp++) >> 24;
						}
					}
					for (int x = 0; x < newW; ++x) {
						uint32_t sum;
						sum = static_cast<uint32_t>(buf[x]) * 3590U;
						sum += static_cast<uint32_t>(buf[x + 1]) * 8121U;
						sum += static_cast<uint32_t>(buf[x + 2]) * 13254U;
						sum += static_cast<uint32_t>(buf[x + 3]) * 15606U;
						sum += static_cast<uint32_t>(buf[x + 4]) * 13254U;
						sum += static_cast<uint32_t>(buf[x + 5]) * 8121U;
						sum += static_cast<uint32_t>(buf[x + 6]) * 3590U;
						*(pixels++) = (sum >> 16 << 24) | 0xffffff;
					}
				} else {
					memset(pixels, 0, newW * 4);
				}
			}

			buf.resize(newH + KernelSize);
			for (auto &e : buf)
				e = 0;
			for (int x = 0; x < newW; ++x) {
				auto *pixels = newbmp->GetPixels() + x;
				{
					auto *inp = pixels;
					auto *outp = buf.data() + (KernelSize >> 1);
					for (int y = 0; y < newH; ++y) {
						*(outp++) = *(inp) >> 24;
						inp += newW;
					}
				}
				for (int y = 0; y < newH; ++y) {
					uint32_t sum;
					sum = static_cast<uint32_t>(buf[y]) * 3590U;
					sum += static_cast<uint32_t>(buf[y + 1]) * 8121U;
					sum += static_cast<uint32_t>(buf[y + 2]) * 13254U;
					sum += static_cast<uint32_t>(buf[y + 3]) * 15606U;
					sum += static_cast<uint32_t>(buf[y + 4]) * 13254U;
					sum += static_cast<uint32_t>(buf[y + 5]) * 8121U;
					sum += static_cast<uint32_t>(buf[y + 6]) * 3590U;
					*(pixels) = (sum >> 16 << 24) | 0xffffff;
					pixels += newW;
				}
			}

			// allocate bin
			auto result = bins.back().Place(*newbmp);
			if (!result) {
				// bin full
				bins.emplace_back(binSize, binSize, *renderer);
				result = bins.back().Place(*newbmp);
				SPAssert(result);
			}

			AABB2 bounds((*result).x, (*result).y, newbmp->GetWidth(), newbmp->GetHeight());

			Vector2 offs = (*g.image).offset - Vector2(1, 1) * (KernelSize * 0.5f);

			g.blurImage.reset((*result).image, bounds, offs);
		}

		void FTFont::Draw(const std::string &str, Vector2 offset, float scale, Vector4 color) {
			SPADES_MARK_FUNCTION();

			auto lines = SplitIntoLines(str);

			float x = 0.f;
			float y = 0.f;

			color = Vector4(color.x * color.w, color.y * color.w, color.z * color.w, color.w);

			renderer->SetColorAlphaPremultiplied(color);

			SplitTextIntoGlyphs(
			  str,
			  [&](Glyph &g) {

				  RenderGlyph(g);

				  auto &img = *g.image;
				  auto srcBounds = img.bounds;
				  auto target = offset + (Vector2(x, y) + img.offset) * scale;
				  target = (target + .5f).Floor(); // for sharper rendering
				  AABB2 destBounds(target.x, target.y, srcBounds.GetWidth() * scale,
				                   srcBounds.GetHeight() * scale);

				  if (!rendererIsLowQuality) {
					  srcBounds = srcBounds.Inflate(.5f);
					  destBounds = destBounds.Inflate(.5f * scale);
				  }

				  renderer->DrawImage(&img.img, destBounds, srcBounds);

				  x += g.advance.x;
				  y += g.advance.y;
			  },
			  [&](uint32_t codepoint) {
				  DrawFallback(codepoint, offset + Vector2(x, y) * scale, height * scale, color);
				  x += MeasureFallback(codepoint, height);
			  },
			  [&]() {
				  x = 0.f;
				  y += lineHeight;
			  });
		}

		void FTFont::DrawBlurred(const std::string &str, Vector2 offset, float scale,
		                         Vector4 color) {
			SPADES_MARK_FUNCTION();

			auto lines = SplitIntoLines(str);

			float x = 0.f;
			float y = 0.f;

			color = Vector4(color.x * color.w, color.y * color.w, color.z * color.w, color.w);

			renderer->SetColorAlphaPremultiplied(color);

			SplitTextIntoGlyphs(
			  str,
			  [&](Glyph &g) {

				  RenderBlurGlyph(g);

				  auto &img = *g.blurImage;
				  auto srcBounds = img.bounds;
				  auto target = offset + (Vector2(x, y) + img.offset) * scale;
				  target = (target + .5f).Floor(); // for sharper rendering
				  AABB2 destBounds(target.x, target.y, srcBounds.GetWidth() * scale,
				                   srcBounds.GetHeight() * scale);

				  if (!rendererIsLowQuality) {
					  srcBounds = srcBounds.Inflate(.5f);
					  destBounds = destBounds.Inflate(.5f * scale);
				  }

				  renderer->DrawImage(&img.img, destBounds, srcBounds);

				  x += g.advance.x;
				  y += g.advance.y;
			  },
			  [&](uint32_t codepoint) {
				  DrawFallback(codepoint, offset + Vector2(x, y) * scale, height * scale, color);
				  x += MeasureFallback(codepoint, height);
			  },
			  [&]() {
				  x = 0.f;
				  y += lineHeight;
			  });
		}

		void FTFont::DrawShadow(const std::string &text, const Vector2 &offset, float scale,
		                        const Vector4 &color, const Vector4 &shadowColor) {
			DrawBlurred(text, offset, scale, shadowColor);
			Draw(text, offset, scale, color);
		}
	}
}
