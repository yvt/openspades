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

#include "Bitmap.h"
#include "Debug.h"
#include "Exception.h"
#include "IBitmapCodec.h"
#include "IStream.h"
#include "jpge.h"
#include <Core/Settings.h>

// FIXME: make this changable for every calls for "Save"
DEFINE_SPADES_SETTING(core_jpegQuality, "95");

namespace spades {
	class JpegWriter : public IBitmapCodec {

		class OutputStream : public jpge::output_stream {
			IStream *stream;

		public:
			OutputStream(IStream *stream) : stream(stream) {}
			virtual ~OutputStream() {}

			virtual bool put_buf(const void *Pbuf, int len) {
				try {
					stream->Write(Pbuf, len);
					return true;
				} catch (...) {
					return false;
				}
			}
		};

	public:
		bool CanLoad() override { return false; }
		bool CanSave() override { return true; }

		bool CheckExtension(const std::string &filename) override {
			return EndsWith(filename, ".jpg") || EndsWith(filename, ".jpeg") ||
			       EndsWith(filename, ".jpe");
		}

		std::string GetName() override {
			static std::string name("JPEG Exporter");
			return name;
		}

		Bitmap *Load(IStream *str) override {
			SPADES_MARK_FUNCTION();

			SPUnreachable();
		}
		void Save(IStream *stream, Bitmap *bmp) override {
			SPADES_MARK_FUNCTION();

			jpge::params params;
			params.m_quality = core_jpegQuality;
			if (params.m_quality < 1 || params.m_quality > 100) {
				SPRaise("Invalid core_jpegQuality");
			}

			OutputStream outStream(stream);
			jpge::jpeg_encoder encoder;

			if (!encoder.init(&outStream, bmp->GetWidth(), bmp->GetHeight(), 3, params)) {
				SPRaise("JPEG encoder initialization failed.");
			}

			auto *pixels = bmp->GetPixels();
			std::vector<uint8_t> lineBuffer;
			int w = bmp->GetWidth();
			int h = bmp->GetHeight();
			lineBuffer.resize(w * 3);
			for (jpge::uint pass = 0; pass < encoder.get_total_passes(); pass++) {
				for (int y = 0; y < h; y++) {
					auto *pix = pixels + (h - 1 - y) * w;
					for (auto *out = lineBuffer.data(), *end = out + w * 3; out != end;) {
						auto p = *(pix++);
						*(out++) = static_cast<uint8_t>(p);
						*(out++) = static_cast<uint8_t>(p >> 8);
						*(out++) = static_cast<uint8_t>(p >> 16);
					}
					if (!encoder.process_scanline(lineBuffer.data())) {
						SPRaise("JPEG encoder processing failed.");
					}
				}
				if (!encoder.process_scanline(nullptr)) {
					SPRaise("JPEG encoder processing failed.");
				}
			}

			encoder.deinit();
		}
	};

	static JpegWriter sharedCodec;
}
