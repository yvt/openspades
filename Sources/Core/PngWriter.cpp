/*
 Copyright (c) 2014 OpenSpades Developers
 
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
#include <cstdint>

#include "IBitmapCodec.h"
#include "Bitmap.h"
#include "IStream.h"

#include "pnglite.h"

namespace {
	unsigned WriteCallback(void* input, std::size_t size, std::size_t numel, void* user_ptr) {
		auto stream = static_cast<spades::IStream*>(user_ptr);
		stream->Write(input, size * numel);
		return size * numel;
	}
}

namespace spades {
	class PngWriter : public IBitmapCodec {
	public:
		PngWriter() : IBitmapCodec() {
			png_init(nullptr, nullptr);
		}

		virtual bool CanLoad(){
			return false;
		}

		virtual bool CanSave(){
			return true;
		}

		virtual bool CheckExtension(const std::string& filename){
			return EndsWith(filename, ".png");
		}

		virtual std::string GetName(){
			static std::string name("libpng exporter");
			return name;
		}

		virtual Bitmap *Load(IStream *str){
			SPADES_MARK_FUNCTION();

			SPNotImplemented();
		}

		virtual void Save(IStream *stream, Bitmap *bmp){
			SPADES_MARK_FUNCTION();

			int err;

			png_t png_s;
			if((err = png_open_write(&png_s, &WriteCallback, stream))){
				SPRaise("Error while png_open_write: %s", png_error_string(err));
			}

			// Create flipped buffer
			std::vector<std::uint8_t> buf(bmp->GetWidth() * bmp->GetHeight() * 4);
			{
				std::uint32_t *pixels = bmp->GetPixels();
				auto width = bmp->GetWidth();
				auto rowLengthBytes = width * sizeof(std::uint8_t) * 4;

				for(long y = bmp->GetHeight() - 1; y >= 0; --y) {
					std::memcpy(&buf[y*rowLengthBytes], pixels, rowLengthBytes);
					pixels += width;
				}
			}
			if((err = png_set_data(&png_s, bmp->GetWidth(), bmp->GetHeight(), 8, PNG_TRUECOLOR_ALPHA, buf.data()))){
				SPRaise("Error while png_set_data: %s", png_error_string(err));
			}
		}
	};

	static PngWriter sharedCodec;
}
