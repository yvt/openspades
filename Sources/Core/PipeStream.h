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
#include <memory>
#include <tuple>

#include <Core/IStream.h>

namespace spades {
	/**
	 * Create a pipe and return a pair of streams for writing and reading,
	 * respectively.
	 *
	 * Hanging up behaviours:
	 *  - If the writer hangs up, the reader will get an EOF for further reads.
	 *  - If the reader hangs up, the writer silently discards the written data.
	 */
	std::tuple<std::unique_ptr<IStream>, std::unique_ptr<IStream>> CreatePipeStream();
} // namespace spades
