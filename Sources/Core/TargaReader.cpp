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

#include "Bitmap.h"
#include "Debug.h"
#include "Exception.h"
#include "IBitmapCodec.h"
#include "IStream.h"

typedef struct _TargaHeader {
	unsigned char id_length, colormap_type, image_type;
	unsigned short colormap_index, colormap_length;
	unsigned char colormap_size;
	unsigned short x_origin, y_origin, width, height;
	unsigned char pixel_size, attributes;
} TargaHeader;
// TODO: endian conversion
#define LittleShort(a) (a)

typedef unsigned char byte;

namespace spades {
	class TargaReader : public IBitmapCodec {
	public:
		bool CanLoad() override { return true; }
		bool CanSave() override { return false; }

		bool CheckExtension(const std::string &filename) override {
			return EndsWith(filename, ".tga");
		}

		std::string GetName() override {
			static std::string name("Quake 3 Derived Targa Importer");
			return name;
		}

		Bitmap *Load(IStream *str) override {
			SPADES_MARK_FUNCTION();

			unsigned columns, rows, numPixels;
			byte *pixbuf;
			int row, column;
			byte *buf_p;
			byte *end;
			union {
				byte *b;
				void *v;
			} buffer;
			TargaHeader targa_header;
			byte *targa_rgba;
			int length;
			std::vector<byte> bufferStorage;

			//
			// load the file
			//
			length = (int)(str->GetLength() - str->GetPosition());
			if (length < 18) {
				SPRaise("LoadTGA: header too short");
			}

			bufferStorage.resize(length);
			str->Read(bufferStorage.data(), (size_t)length);
			buffer.v = bufferStorage.data();

			buf_p = buffer.b;
			end = buffer.b + length;

			targa_header.id_length = buf_p[0];
			targa_header.colormap_type = buf_p[1];
			targa_header.image_type = buf_p[2];

			memcpy(&targa_header.colormap_index, &buf_p[3], 2);
			memcpy(&targa_header.colormap_length, &buf_p[5], 2);
			targa_header.colormap_size = buf_p[7];
			memcpy(&targa_header.x_origin, &buf_p[8], 2);
			memcpy(&targa_header.y_origin, &buf_p[10], 2);
			memcpy(&targa_header.width, &buf_p[12], 2);
			memcpy(&targa_header.height, &buf_p[14], 2);
			targa_header.pixel_size = buf_p[16];
			targa_header.attributes = buf_p[17];

			targa_header.colormap_index = LittleShort(targa_header.colormap_index);
			targa_header.colormap_length = LittleShort(targa_header.colormap_length);
			targa_header.x_origin = LittleShort(targa_header.x_origin);
			targa_header.y_origin = LittleShort(targa_header.y_origin);
			targa_header.width = LittleShort(targa_header.width);
			targa_header.height = LittleShort(targa_header.height);

			buf_p += 18;

			if (targa_header.image_type != 2 && targa_header.image_type != 10 &&
			    targa_header.image_type != 3) {
				SPRaise("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported");
			}

			if (targa_header.colormap_type != 0) {
				SPRaise("LoadTGA: colormaps not supported");
			}

			if ((targa_header.pixel_size != 32 && targa_header.pixel_size != 24) &&
			    targa_header.image_type != 3) {
				SPRaise("LoadTGA: Only 32 or 24 bit images supported (no colormaps)");
			}

			columns = targa_header.width;
			rows = targa_header.height;
			numPixels = columns * rows * 4;

			if (!columns || !rows || numPixels > 0x7FFFFFFF || numPixels / columns / 4 != rows) {
				SPRaise("LoadTGA:  invalid image size");
			}

			std::vector<byte> outPixels;
			outPixels.resize(numPixels);
			targa_rgba = outPixels.data();

			if (targa_header.id_length != 0) {
				if (buf_p + targa_header.id_length > end)
					SPRaise("LoadTGA: header too short");

				buf_p += targa_header.id_length; // skip TARGA image comment
			}

			if (targa_header.image_type == 2 || targa_header.image_type == 3) {
				if (buf_p + columns * rows * targa_header.pixel_size / 8 > end) {
					SPRaise("LoadTGA: file truncated");
				}

				// Uncompressed RGB or gray scale image
				for (row = rows - 1; row >= 0; row--) {
					pixbuf = targa_rgba + row * columns * 4;
					for (column = 0; column < columns; column++) {
						unsigned char red, green, blue, alphabyte;
						switch (targa_header.pixel_size) {

							case 8:
								blue = *buf_p++;
								green = blue;
								red = blue;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;

							case 24:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = 255;
								break;
							case 32:
								blue = *buf_p++;
								green = *buf_p++;
								red = *buf_p++;
								alphabyte = *buf_p++;
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								break;
							default:
								SPRaise("LoadTGA: illegal pixel_size '%d' ",
								        targa_header.pixel_size);
								break;
						}
					}
				}
			} else if (targa_header.image_type == 10) { // Runlength encoded RGB images
				unsigned char red, green, blue, alphabyte, packetHeader, packetSize, j;

				for (row = rows - 1; row >= 0; row--) {
					pixbuf = targa_rgba + row * columns * 4;
					for (column = 0; column < columns;) {
						if (buf_p + 1 > end)
							SPRaise("LoadTGA: file truncated");
						packetHeader = *buf_p++;
						packetSize = 1 + (packetHeader & 0x7f);
						if (packetHeader & 0x80) { // run-length packet
							if (buf_p + targa_header.pixel_size / 8 > end)
								SPRaise("LoadTGA: file truncated");
							switch (targa_header.pixel_size) {
								case 24:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = 255;
									break;
								case 32:
									blue = *buf_p++;
									green = *buf_p++;
									red = *buf_p++;
									alphabyte = *buf_p++;
									break;
								default:
									SPRaise("LoadTGA: illegal pixel_size '%d' ",
									        targa_header.pixel_size);
									break;
							}

							for (j = 0; j < packetSize; j++) {
								*pixbuf++ = red;
								*pixbuf++ = green;
								*pixbuf++ = blue;
								*pixbuf++ = alphabyte;
								column++;
								if (column == columns) { // run spans across rows
									column = 0;
									if (row > 0)
										row--;
									else
										goto breakOut;
									pixbuf = targa_rgba + row * columns * 4;
								}
							}
						} else { // non run-length packet

							if (buf_p + targa_header.pixel_size / 8 * packetSize > end)
								SPRaise("LoadTGA: file truncated");
							for (j = 0; j < packetSize; j++) {
								switch (targa_header.pixel_size) {
									case 24:
										blue = *buf_p++;
										green = *buf_p++;
										red = *buf_p++;
										*pixbuf++ = red;
										*pixbuf++ = green;
										*pixbuf++ = blue;
										*pixbuf++ = 255;
										break;
									case 32:
										blue = *buf_p++;
										green = *buf_p++;
										red = *buf_p++;
										alphabyte = *buf_p++;
										*pixbuf++ = red;
										*pixbuf++ = green;
										*pixbuf++ = blue;
										*pixbuf++ = alphabyte;
										break;
									default:
										SPRaise("LoadTGA: illegal pixel_size '%d' ",
										        targa_header.pixel_size);
										break;
								}
								column++;
								if (column == columns) { // pixel packet run spans across rows
									column = 0;
									if (row > 0)
										row--;
									else
										goto breakOut;
									pixbuf = targa_rgba + row * columns * 4;
								}
							}
						}
					}
				breakOut:;
				}
			}

#if 0
			// TTimo: this is the chunk of code to ensure a behavior that meets TGA specs
			// bit 5 set => top-down
			if (targa_header.attributes & 0x20) {
				unsigned char *flip = (unsigned char*)malloc (columns*4);
				unsigned char *src, *dst;

				for (row = 0; row < rows/2; row++) {
					src = targa_rgba + row * 4 * columns;
					dst = targa_rgba + (rows - row - 1) * 4 * columns;

					memcpy (flip, src, columns*4);
					memcpy (src, dst, columns*4);
					memcpy (dst, flip, columns*4);
				}
				free (flip);
			}
#endif
			// instead we just print a warning
			if (targa_header.attributes & 0x20) {
				// SPRaise( PRINT_WARNING, "WARNING: '%s' TGA file header declares top-down image,
				// ignoring\n", name);
			}

			Bitmap *bmp = new Bitmap(columns, rows);
			byte *out = (byte *)bmp->GetPixels();
			for (unsigned int i = 0; i < numPixels; i++)
				out[i] = targa_rgba[i];

			return bmp;
		}
		void Save(IStream *stream, Bitmap *bmp) override {
			SPADES_MARK_FUNCTION();
			SPUnreachable();
		}
	};

	static TargaReader sharedCodec;
}
