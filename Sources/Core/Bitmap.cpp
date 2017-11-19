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
#include <vector>

#include <Core/Debug.h>
#include "Bitmap.h"
#include "Debug.h"
#include "Exception.h"
#include "FileManager.h"
#include "IBitmapCodec.h"
#include "IStream.h"
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	Bitmap::Bitmap(int ww, int hh) : w(ww), h(hh), pixels(nullptr), autoDelete(true) {
		SPADES_MARK_FUNCTION();

		if (w < 1 || h < 1 || w > 8192 || h > 8192) {
			SPRaise("Invalid dimension: %dx%d", w, h);
		}

		pixels = new uint32_t[w * h];
		SPAssert(pixels != NULL);
	}

	Bitmap::Bitmap(uint32_t *pixels, int w, int h) : w(w), h(h), pixels(pixels), autoDelete(false) {
		SPADES_MARK_FUNCTION();

		if (w < 1 || h < 1 || w > 8192 || h > 8192) {
			SPRaise("Invalid dimension: %dx%d", w, h);
		}

		SPAssert(pixels != NULL);
	}

	Bitmap::~Bitmap() {
		SPADES_MARK_FUNCTION();

		if (autoDelete)
			delete[] pixels;
	}

	Bitmap *Bitmap::Load(const std::string &filename) {
		std::vector<IBitmapCodec *> codecs = IBitmapCodec::GetAllCodecs();
		std::string errMsg;
		for (size_t i = 0; i < codecs.size(); i++) {
			IBitmapCodec *codec = codecs[i];
			if (codec->CanLoad() && codec->CheckExtension(filename)) {
				// give it a try.
				// open error shouldn't be handled here
				StreamHandle str = FileManager::OpenForReading(filename.c_str());
				try {
					return codec->Load(str);
				} catch (const std::exception &ex) {
					errMsg += codec->GetName();
					errMsg += ":\n";
					errMsg += ex.what();
					errMsg += "\n\n";
				}
			}
		}

		if (errMsg.empty()) {
			SPRaise("Bitmap codec not found for filename: %s", filename.c_str());
		} else {
			SPRaise("No bitmap codec could load file successfully: %s\n%s\n", filename.c_str(),
			        errMsg.c_str());
		}
	}

	Bitmap *Bitmap::Load(IStream *stream) {
		std::vector<IBitmapCodec *> codecs = IBitmapCodec::GetAllCodecs();
		auto pos = stream->GetPosition();
		std::string errMsg;
		for (size_t i = 0; i < codecs.size(); i++) {
			IBitmapCodec *codec = codecs[i];
			if (codec->CanLoad()) {
				// give it a try.
				// open error shouldn't be handled here
				try {
					stream->SetPosition(pos);
					return codec->Load(stream);
				} catch (const std::exception &ex) {
					errMsg += codec->GetName();
					errMsg += ":\n";
					errMsg += ex.what();
					errMsg += "\n\n";
				}
			}
		}

		if (errMsg.empty()) {
			SPRaise("Bitmap codec not found for stream");
		} else {
			SPRaise("No bitmap codec could load file successfully: [stream]\n%s\n", errMsg.c_str());
		}
	}

	void Bitmap::Save(const std::string &filename) {
		std::vector<IBitmapCodec *> codecs = IBitmapCodec::GetAllCodecs();
		for (size_t i = 0; i < codecs.size(); i++) {
			IBitmapCodec *codec = codecs[i];
			if (codec->CanSave() && codec->CheckExtension(filename)) {
				StreamHandle str = FileManager::OpenForWriting(filename.c_str());

				codec->Save(str, this);
				return;
			}
		}

		SPRaise("Bitmap codec not found for filename: %s", filename.c_str());
	}

	uint32_t Bitmap::GetPixel(int x, int y) {
		SPAssert(x >= 0);
		SPAssert(y >= 0);
		SPAssert(x < w);
		SPAssert(y < h);
		return pixels[x + y * w];
	}

	void Bitmap::SetPixel(int x, int y, uint32_t p) {
		SPAssert(x >= 0);
		SPAssert(y >= 0);
		SPAssert(x < w);
		SPAssert(y < h);
		pixels[x + y * w] = p;
	}

	Handle<Bitmap> Bitmap::Clone() {
		Bitmap *b = new Bitmap(w, h);
		std::memcpy(b->GetPixels(), pixels, static_cast<std::size_t>(w * h * 4));
		return Handle<Bitmap>(b, false);
	}
}
