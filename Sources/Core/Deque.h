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

#pragma once

#include <cstdio>
#include <cstdlib>

#include <Core/Debug.h>

namespace spades {
	/** Deque implementation. NPOT is not fully supported. */
	template <typename T> class Deque {
		T *ptr;
		size_t length;
		size_t startPos;
		size_t capacity;

	public:
		Deque(size_t cap) {
			ptr = (T *)malloc(sizeof(T) * cap);
			startPos = 0;
			length = 0;
			capacity = cap;
		}

		~Deque() { free(ptr); }

		void Reserve(size_t newCap) {
			if (newCap <= capacity)
				return;
			T *newPtr = (T *)malloc(sizeof(T) * newCap);
			size_t pos = startPos;
			for (size_t i = 0; i < length; i++) {
				newPtr[i] = ptr[pos++];
				if (pos == capacity)
					pos = 0;
			}
			free(ptr);
			ptr = newPtr;
			startPos = 0;
			capacity = newCap;
		}

		void Push(const T &e) {
			if (length + 1 > capacity) {
				size_t newCap = capacity;
				while (newCap < length + 1)
					newCap <<= 1;
				Reserve(newCap);
			}
			size_t pos = startPos + length;
			if (pos >= capacity)
				pos -= capacity;
			ptr[pos] = e;
			length++;
		}

		T &Front() { return ptr[startPos]; }

		void Shift() {
			SPAssert(length > 0);
			startPos++;
			if (startPos == capacity)
				startPos = 0;
			length--;
		}

		size_t GetLength() const { return length; }

		bool IsEmpty() const { return length == 0; }
	};
}
