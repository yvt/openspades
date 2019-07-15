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
#pragma once

#include <Core/Debug.h>
#include <Core/RefCountedObject.h>

namespace spades {
	/**
	 * Represents an iterator object.
	 *
	 * At the initial state, it does not point any element. `MoveNext` must be
	 * called first to move the iterator to the first element.
	 */
	template <class T> class Iterator : public RefCountedObject {
	public:
		/**
		 * Get the current item.
		 */
		virtual T GetCurrent() = 0;

		/**
		 * Advance the iterator.
		 *
		 * @return `true` if the iterator was successfully advanced to the next
		 *         element. `false` if it reached the end of the sequence.
		 */
		virtual bool MoveNext() = 0;
	};

	/** Represents an iterator that produces no elements. */
	template <class T> class EmptyIterator final : public Iterator<T> {
	public:
		T GetCurrent() override { SPUnreachable(); }
		bool MoveNext() override { return false; }
	};
} // namespace spades
