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

#ifndef _MSC_VER
#include <unistd.h>
#else
#include <io.h>
#define ftruncate _chsize
#define fileno _fileno
#endif

#include "StdStream.h"
#include "Exception.h"

#include "Debug.h"

namespace spades {
	StdStream::StdStream(FILE *f, bool ac) : handle(f), autoClose(ac) {
		SPADES_MARK_FUNCTION();

		if (!f)
			SPInvalidArgument("f");
	}

	StdStream::~StdStream() {
		SPADES_MARK_FUNCTION();

		if (autoClose)
			fclose(handle);
	}

	int StdStream::ReadByte() {
		SPADES_MARK_FUNCTION();

		int ret = fgetc(handle);
		if (ret == EOF) {
			if (ferror(handle)) {
				SPRaise("I/O error.");
			} else {
				return -1;
			}
		} else {
			return ret;
		}
	}

	size_t StdStream::Read(void *buf, size_t bytes) {
		SPADES_MARK_FUNCTION_DEBUG();

		return fread(buf, 1, bytes, handle);
	}

	void StdStream::WriteByte(int byte) {
		SPADES_MARK_FUNCTION();

		if (fputc(byte, handle) != 0) {
			SPRaise("I/O error.");
		}
	}

	void StdStream::Write(const void *buf, size_t bytes) {
		SPADES_MARK_FUNCTION();

		if (fwrite(buf, 1, bytes, handle) < bytes) {
			SPRaise("I/O error.");
		}
	}

	uint64_t StdStream::GetPosition() {
		SPADES_MARK_FUNCTION_DEBUG();
		return ftell(handle);
	}

	void StdStream::SetPosition(uint64_t pos) {
		SPADES_MARK_FUNCTION();

		if (pos > 0x7fffffffULL) {
			SPRaise("Currently StdStream doesn't support 64-bit offset.");
		}
		fseek(handle, (long)pos, SEEK_SET);
	}

	uint64_t StdStream::GetLength() {
		SPADES_MARK_FUNCTION();

		uint64_t opos = GetPosition();
		fseek(handle, 0, SEEK_END);
		uint64_t len = GetPosition();
		SetPosition(opos);
		return len;
	}

	void StdStream::SetLength(uint64_t len) {
		SPADES_MARK_FUNCTION_DEBUG();

		// this is not safe
		ftruncate(fileno(handle), len);
	}

	void StdStream::Flush() { fflush(handle); }
}
