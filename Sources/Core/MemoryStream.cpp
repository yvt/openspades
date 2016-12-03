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

#include "Debug.h"
#include "Exception.h"
#include "MemoryStream.h"

namespace spades {
	MemoryStream::MemoryStream(char *buffer, size_t len, bool allowWrite) {
		SPADES_MARK_FUNCTION();
		memory = (unsigned char *)buffer;
		position = 0;
		length = len;
		canWrite = allowWrite;
	}

	MemoryStream::MemoryStream(const char *buffer, size_t len) {
		SPADES_MARK_FUNCTION();
		memory = (unsigned char *)const_cast<char *>(buffer);
		position = 0;
		length = len;
		canWrite = false;
	}

	MemoryStream::~MemoryStream() { SPADES_MARK_FUNCTION(); }

	int MemoryStream::ReadByte() {
		SPADES_MARK_FUNCTION();
		if (position < length)
			return memory[(size_t)(position++)];
		else
			return -1;
	}

	size_t MemoryStream::Read(void *data, size_t bytes) {
		SPADES_MARK_FUNCTION();
		if (position >= length) {
			return 0;
		}
		uint64_t maxBytes = length - position;
		if ((uint64_t)bytes > maxBytes)
			bytes = (size_t)maxBytes;
		memcpy(data, memory + (size_t)position, (size_t)bytes);
		position += (uint64_t)bytes;
		return bytes;
	}

	std::string MemoryStream::Read(size_t bytes) {
		SPADES_MARK_FUNCTION();
		if (position >= length) {
			return std::string();
		}
		uint64_t maxBytes = length - position;
		if ((uint64_t)bytes > maxBytes)
			bytes = (size_t)maxBytes;
		std::string s((char *)(memory + position), bytes);
		position += (uint64_t)bytes;
		return s;
	}

	void MemoryStream::WriteByte(int byte) {
		SPADES_MARK_FUNCTION();
		if (canWrite) {
			SPRaise("Write prohibited");
		}
		if (position >= length) {
			SPRaise("Attempted to write beyond the specified memory range.");
		} else {
			memory[(size_t)(position++)] = (unsigned char)byte;
		}
	}

	void MemoryStream::Write(const void *data, size_t bytes) {
		SPADES_MARK_FUNCTION();
		if (canWrite) {
			SPRaise("Write prohibited");
		}
		if (position + (uint64_t)bytes > length ||
		    position + (uint64_t)bytes < position) { // integer overflow check
			SPRaise("Attempted to write beyond the specified memory range.");
		} else {
			memcpy(memory + (size_t)position, data, bytes);
		}
	}

	uint64_t MemoryStream::GetPosition() { return position; }

	void MemoryStream::SetPosition(uint64_t pos) { position = pos; }

	uint64_t MemoryStream::GetLength() { return length; }

	void MemoryStream::SetLength(uint64_t len) {
		SPRaise("Changing the length of MemroyStream is prohibited.");
	}
}
