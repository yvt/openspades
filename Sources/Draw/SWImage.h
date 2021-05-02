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

#include <unordered_map>

#include <Client/IImage.h>
#include <Core/Bitmap.h>

namespace spades {
	namespace draw {
		class SWImage : public client::IImage {
			// Handle<Bitmap> rawBmp;

			std::vector<uint32_t> bmp;
			int ew, eh; // exact size

			bool isWhite;

			float w, h;
			float iw, ih;

		protected:
			~SWImage();

		public:
			SWImage(Bitmap &bmp);
			SWImage(int w, int h);

			uint32_t *GetRawBitmap() { return bmp.data(); }
			int GetRawWidth() { return ew; }
			int GetRawHeight() { return eh; }

			bool IsWhiteImage() { return isWhite; }

			void Update(Bitmap &, int x, int y) override;

			float GetWidth() override { return w; }
			float GetHeight() override { return h; }
			float GetInvWidth() { return iw; }
			float GetInvHeight() { return ih; }
		};

		class SWImageManager {
			std::unordered_map<std::string, Handle<SWImage>> images;

		public:
			SWImageManager() {}
			~SWImageManager();

			Handle<SWImage> RegisterImage(const std::string &);
			Handle<SWImage> CreateImage(Bitmap &);

			void ClearCache();
		};
	} // namespace draw
} // namespace spades
