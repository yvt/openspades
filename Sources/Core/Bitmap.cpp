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

#include "Bitmap.h"
#include "Debug.h"
#include "Exception.h"
#include "FileManager.h"
#include "IBitmapCodec.h"
#include "IStream.h"
#include <Core/Debug.h>
#include <ScriptBindings/ScriptManager.h>

namespace spades {
	namespace {
		void ValidateDimensions(int width, int height) {
			SPADES_MARK_FUNCTION();

			// Do not allow creating a huge image so that we can catch misuses
			// and memory corruption
			if (width < 1 || height < 1 || width > 8192 || height > 8192) {
				SPRaise("Invalid dimensions: %dx%d", width, height);
			}
		}

		inline void ValidatePoint(const Bitmap &bitmap, int x, int y) {
			if (static_cast<unsigned int>(x) >= static_cast<unsigned int>(bitmap.GetWidth()) ||
			    static_cast<unsigned int>(y) >= static_cast<unsigned int>(bitmap.GetHeight())) {
				SPRaise("The point (%d, %d) is out of bounds of bitmap of size %dx%d", x, y,
				        bitmap.GetWidth(), bitmap.GetHeight());
			}
		}
	} // namespace

	Bitmap::Bitmap(int width, int height)
	    : width{width}, height{height}, pixels{nullptr}, autoDelete{true} {
		SPADES_MARK_FUNCTION();

		ValidateDimensions(width, height);

		pixels = new uint32_t[width * height];
		SPAssert(pixels != nullptr);
	}

	Bitmap::Bitmap(uint32_t *pixels, int width, int height)
	    : width{width}, height{height}, pixels{pixels}, autoDelete{false} {
		SPADES_MARK_FUNCTION();

		ValidateDimensions(width, height);
		SPAssert(pixels != nullptr);
	}

	Bitmap::~Bitmap() {
		SPADES_MARK_FUNCTION();

		if (autoDelete) {
			delete[] pixels;
		}
	}

	Handle<Bitmap> Bitmap::Load(const std::string &filename) {
		std::vector<IBitmapCodec *> codecs = IBitmapCodec::GetAllCodecs();
		std::string errMsg;
		for (IBitmapCodec *codec : codecs) {
			if (codec->CanLoad() && codec->CheckExtension(filename)) {
				// give it a try.
				// open error shouldn't be handled here
				std::unique_ptr<IStream> str{FileManager::OpenForReading(filename.c_str())};
				try {
					return {codec->Load(str.get()), false};
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

	Handle<Bitmap> Bitmap::Load(IStream &stream) {
		std::vector<IBitmapCodec *> codecs = IBitmapCodec::GetAllCodecs();
		auto pos = stream.GetPosition();
		std::string errMsg;
		for (IBitmapCodec *codec : codecs) {
			if (codec->CanLoad()) {
				// give it a try.
				// open error shouldn't be handled here
				try {
					stream.SetPosition(pos);
					return {codec->Load(&stream), false};
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
		for (IBitmapCodec *codec : codecs) {
			if (codec->CanSave() && codec->CheckExtension(filename)) {
				std::unique_ptr<IStream> str{FileManager::OpenForWriting(filename.c_str())};

				codec->Save(str.get(), this);
				return;
			}
		}

		SPRaise("Bitmap codec not found for filename: %s", filename.c_str());
	}

	uint32_t Bitmap::GetPixel(int x, int y) const {
		ValidatePoint(*this, x, y);
		return pixels[x + y * width];
	}

	void Bitmap::SetPixel(int x, int y, uint32_t p) {
		ValidatePoint(*this, x, y);
		pixels[x + y * width] = p;
	}

	Handle<Bitmap> Bitmap::Clone() {
		Handle<Bitmap> b = Handle<Bitmap>::New(width, height);
		std::memcpy(b->GetPixels(), pixels, static_cast<std::size_t>(width * height * 4));
		return b;
	}
} // namespace spades
