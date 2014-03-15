/*
 Copyright (c) 2014 Marco Schlumpp <marco.schlumpp@gmail.com>
 
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

#include <png.h>

#include "Core/IBitmapCodec.h"
#include "Core/Bitmap.h"
#include "Core/IStream.h"

namespace {
	struct pngwriter_state {
		spades::IStream *stream;
	};

	void pngwriter_write_data(png_structp png_ptr, png_bytep data, png_size_t length) {
		auto state = static_cast<pngwriter_state*>(png_get_io_ptr(png_ptr));
		state->stream->Write(data, length);
	}
}

namespace spades {
	class PNGWriter : public IBitmapCodec {
	public:
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

			// Initialize libpng
			png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING,
														  nullptr,
														  nullptr,
														  nullptr);
														  
			if(!png_ptr)
				SPRaise("Failed to initialize png writer");
			png_infop info_ptr = png_create_info_struct(png_ptr);
			if(!info_ptr) {
				png_destroy_write_struct(&png_ptr, nullptr);
				SPRaise("Failed to initialize png writer(info struct)");
			}
			// Error handling
			if(setjmp(png_jmpbuf(png_ptr))) {
				png_destroy_write_struct(&png_ptr, &info_ptr);
				SPRaise("Error in libpng");
			}

			// Fill info_ptr
			png_set_IHDR(png_ptr, info_ptr, bmp->GetWidth(), bmp->GetHeight(), 8,
						 PNG_COLOR_TYPE_RGB_ALPHA, PNG_INTERLACE_NONE,
						 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
			
			// Fill bitmap
			auto pixels = bmp->GetPixels();

			png_byte** row_pointers = static_cast<png_byte**>(png_malloc(png_ptr, bmp->GetHeight() * sizeof(png_byte*)));
			for(int y = bmp->GetHeight() - 1; y >= 0; --y) { // Fill the rows from the back to flip the image vertically
				png_byte* row = static_cast<png_byte*>(png_malloc(png_ptr, sizeof(std::uint8_t) * bmp->GetWidth() * 4));
				row_pointers[y] = row;
				std::memcpy(row, pixels, bmp->GetWidth() * 4);
				pixels += bmp->GetWidth();
			}

			// Setup IO
			pngwriter_state state{stream};
			png_set_write_fn(png_ptr, &state, pngwriter_write_data, nullptr);

			// Finally write the png
			png_set_rows(png_ptr, info_ptr, row_pointers);
			png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, nullptr);

			// Clean up
			for(std::size_t y = 0; y < bmp->GetHeight(); y++) {
				png_free(png_ptr, row_pointers[y]);
			}
			png_free(png_ptr, row_pointers);
			png_destroy_write_struct(&png_ptr, &info_ptr);
		}
	};

	static PNGWriter sharedCodec;
}
