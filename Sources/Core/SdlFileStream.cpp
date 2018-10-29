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

#include "SdlFileStream.h"
#include "Debug.h"
#include "Exception.h"

namespace spades {
	SdlFileStream::SdlFileStream(SDL_RWops *f, bool ac) : ops(f), autoClose(ac) {
		SPADES_MARK_FUNCTION();

		if (!f)
			SPInvalidArgument("f");
	}

	SdlFileStream::~SdlFileStream() {
		SPADES_MARK_FUNCTION();

		if (autoClose && ops)
			SDL_RWclose(ops);
	}

	int SdlFileStream::ReadByte() {
		SPADES_MARK_FUNCTION();

		unsigned char ch;
		if (SDL_RWread(ops, &ch, 1, 1) < 1) {
			return -1;
		} else {
			return ch;
		}
	}

	size_t SdlFileStream::Read(void *buf, size_t bytes) {
		SPADES_MARK_FUNCTION_DEBUG();
		return SDL_RWread(ops, buf, 1, bytes);
	}

	void SdlFileStream::WriteByte(int byte) {
		SPADES_MARK_FUNCTION();

		auto ch = static_cast<unsigned char>(byte);
		if (SDL_RWwrite(ops, &ch, 1, 1) == 0) {
			SPRaise("I/O error.");
		}
	}

	void SdlFileStream::Write(const void *buf, size_t bytes) {
		SPADES_MARK_FUNCTION();

		if (SDL_RWwrite(ops, buf, 1, bytes) < bytes) {
			SPRaise("I/O error.");
		}
	}

	uint64_t SdlFileStream::GetPosition() {
		SPADES_MARK_FUNCTION_DEBUG();
		auto pos = SDL_RWtell(ops);
		if (pos == -1) {
			SPRaise("This stream doesn't support seeking.");
		}
		return static_cast<uint64_t>(pos);
	}

	void SdlFileStream::SetPosition(uint64_t pos) {
		SPADES_MARK_FUNCTION();

		if (pos > 0x7fffffffULL) {
			SPRaise("Currently SdlFileStream doesn't support 64-bit offset.");
		}

		if (SDL_RWseek(ops, static_cast<Sint64>(pos), RW_SEEK_SET) == -1) {
			SPRaise("This stream doesn't support seeking.");
		}
	}

	uint64_t SdlFileStream::GetLength() {
		SPADES_MARK_FUNCTION();

		uint64_t opos = GetPosition();
		if (SDL_RWseek(ops, 0, RW_SEEK_END) == -1) {
			SPRaise("This stream doesn't support seeking.");
		}
		uint64_t len = GetPosition();
		SetPosition(opos);
		return len;
	}

	void SdlFileStream::SetLength(uint64_t len) {
		SPADES_MARK_FUNCTION();
		SPUnsupported();
	}

	void SdlFileStream::Flush() {
		// SDL_rwops doesn't support flush.
		// by seeking we might be able to flush stream buffer.
		auto opos = GetPosition();
		SetPosition(0);
		int b = ReadByte();
		if (b != -1) {
			SetPosition(0);
			WriteByte(b);
		}
		SetPosition(opos);
	}
}
