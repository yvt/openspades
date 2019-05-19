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
#include <algorithm>
#include <vector>

#include <Core/Exception.h>
#include <Core/TMPUtils.h>

namespace spades {
	class IStream;

	/**
	 * Wraps a read-only stream to provide a random-accessible view into the data using a
	 * dynamically-growing internal buffer.
	 */
	class RandomAccessAdaptor {
	public:
		/**
		 * Constructs a `RandomAccessAdaptor` using the specified stream to supply the data read
		 * through the view.
		 *
		 * The specified stream must outlive the uses of the constructed `RandomAccessAdaptor`.
		 */
		RandomAccessAdaptor(IStream &inner);

		/**
		 * De-initializes a `RandomAcesssAdaptor` and release all resources associates with it.
		 *
		 * Note that the wrapped stream is not closed by this finalizer.
		 */
		~RandomAccessAdaptor();

		/**
		 * Try to read the value of type `T` at the specified offset.
		 *
		 * Various complicated rules regarding reinterpretation apply, so it's not generally
		 * a good idea to specfiy a non-POD type as `T`.
		 *
		 * `offset + sizeof(T)` must not overflow `size_t`. An exception is thrown in such a case,
		 * but probably should abort the program in the future.
		 */
		template <class T> stmp::optional<T> TryRead(std::size_t offset) {
			T data;
			if (TryRead(offset, sizeof(T), reinterpret_cast<char *>(&data))) {
				return {data};
			} else {
				return {};
			}
		}

		/**
		 * Read the value of type `T` at the specified offset. Throws an exception if an EOF is
		 * reached.
		 *
		 * See also: `TryRead`.
		 */
		template <class T> T Read(std::size_t offset) {
			auto data = TryRead<T>(offset);
			if (data) {
				return std::move(*data);
			} else {
				SPRaise("Unexpected EOF");
			}
		}

	private:
		/**
		 * Tries to ensure `buffer` is at least `newBufferSize` bytes long.
		 *
		 * The final size might be less than `newBufferSize` if an EOF is reached. This function is
		 * exception-safe - `buffer` is left in the original state if an exception occurs while
		 * reading the inner stream.
		 */
		void ExpandTo(std::size_t newBufferSize);

		/**
		 * Try to read `size` bytes at the specified offset to `output`.
		 *
		 * @return `true` if all of the bytes could be read.
		 */
		bool TryRead(std::size_t offset, std::size_t size, char *output);

		IStream &inner;
		std::vector<char> buffer;
	};
} // namespace spades
