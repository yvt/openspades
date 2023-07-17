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

#include "SWImage.h"
#include <Core/FileManager.h>
#include <Core/IStream.h>

namespace spades {
	namespace draw {
		namespace {
			inline uint32_t ProcessPixel(uint32_t col) {
				unsigned int alpha = static_cast<unsigned int>(col >> 24);
				alpha += (alpha >> 7); // [0,255] to [0,256]

				unsigned int r = static_cast<unsigned int>((col >> 0) & 0xff);
				unsigned int g = static_cast<unsigned int>((col >> 8) & 0xff);
				unsigned int b = static_cast<unsigned int>((col >> 16) & 0xff);
				r = (r * alpha) >> 8;
				g = (g * alpha) >> 8;
				b = (b * alpha) >> 8;

				col &= 0xff000000;
				col |= b | (g << 8) | (r << 16); // swap RGB/BGR
				return col;
			}
		} // namespace

		SWImage::SWImage(Bitmap &m)
		    : ew(m.GetWidth()),
		      eh(m.GetHeight()),
		      isWhite(false),
		      w(static_cast<float>(m.GetWidth())),
		      h(static_cast<float>(m.GetHeight())),
		      iw(1.f / w),
		      ih(1.f / h) {
			bmp.resize(ew * eh);

			// premultiplied alpha
			{
				uint32_t *inpix = m.GetPixels();
				uint32_t *outpix = bmp.data();
				bool foundNonWhite = false;
				for (std::size_t i = ew * eh; i; i--) {
					uint32_t col = *(inpix++);
					col = ProcessPixel(col);
					*(outpix++) = col;

					if (col != 0xffffffff)
						foundNonWhite = true;
				}
				isWhite = !foundNonWhite;
			}
		}

		SWImage::SWImage(int w, int h)
		    : ew(w),
		      eh(h),
		      isWhite(false),
		      w(static_cast<float>(ew)),
		      h(static_cast<float>(eh)),
		      iw(1.f / w),
		      ih(1.f / h) {
			bmp.reserve(ew * eh);
		}

		SWImage::~SWImage() {}

		void SWImage::Update(Bitmap &inBmp, int x, int y) {
			SPADES_MARK_FUNCTION();
			if (x < 0 || y < 0 || x + inBmp.GetWidth() > ew || y + inBmp.GetHeight() > eh) {
				SPRaise("Out of range.");
			}

			{
				int bw = inBmp.GetWidth();
				int bh = inBmp.GetHeight();
				for (int yy = 0; yy < bh; ++yy) {
					uint32_t *inpix = inBmp.GetPixels();
					uint32_t *outpix = bmp.data();
					outpix += x + (y + yy) * ew;
					inpix += yy * bw;
					for (unsigned int j = bw; j; --j) {
						*(outpix++) = ProcessPixel(*(inpix++));
					}
				}
			}

			isWhite = false;
		}

		SWImageManager::~SWImageManager() {}

		Handle<SWImage> SWImageManager::RegisterImage(const std::string &name) {
			auto it = images.find(name);
			if (it == images.end()) {
				Handle<Bitmap> bitmap = Bitmap::Load(name);
				Handle<SWImage> image = CreateImage(*bitmap);
				images.insert(std::make_pair(name, image));
				return image;
			} else {
				return it->second;
			}
		}

		Handle<SWImage> SWImageManager::CreateImage(Bitmap &bitmap) {
			return Handle<SWImage>::New(bitmap);
		}

		void SWImageManager::ClearCache() { images.clear(); }
	} // namespace draw
} // namespace spades
