/*
 Copyright (c) 2019 yvt

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
#include "RandomAccessAdaptor.h"

#include <Core/Exception.h>
#include <Core/IStream.h>
#include <cstring>

namespace spades {
	RandomAccessAdaptor::RandomAccessAdaptor(IStream &inner) : inner{inner} {}

	RandomAccessAdaptor::~RandomAccessAdaptor() {
		// Do nothing - the buffer is automatically taken care of
	}

	void RandomAccessAdaptor::ExpandTo(std::size_t newBufferSize) {
		if (newBufferSize <= buffer.size()) {
			return;
		}

		std::size_t numAdditionalBytes = newBufferSize - buffer.size();
		std::size_t originalSize = buffer.size();

		try {
			// Expand the internal buffer
			buffer.resize(newBufferSize);

			// Read additional bytes from the underlying stream to the newly
			// allocated space of the internal buffer
			std::size_t numBytesRead = inner.Read(buffer.data() + originalSize, numAdditionalBytes);

			// Shrink the buffer to the actual read size
			buffer.resize(originalSize + numBytesRead);
		} catch (...) {
			// Roll back to the original state
			buffer.resize(originalSize);
			throw;
		}
	}

	bool RandomAccessAdaptor::TryRead(std::size_t offset, std::size_t size, char *output) {
		std::size_t newLen = offset + size;
		if (newLen < offset) {
			// `newLen` overflowed `size_t`.
			SPRaise("integer overflow");
		}

		ExpandTo(newLen);

		if (buffer.size() < newLen) {
			// Reached EOF
			return false;
		}

		std::memcpy(output, buffer.data() + offset, size);
		return true;
	}
} // namespace spades
