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

#include <Imports/SDL.h>

#include "IStream.h"

namespace spades {
	class SdlFileStream : public IStream {
		SDL_RWops *ops;
		bool autoClose;

	public:
		SdlFileStream(SDL_RWops *f, bool autoClose = false);
		SdlFileStream(const SdlFileStream &) = delete;
		void operator=(const SdlFileStream &) = delete;
		~SdlFileStream();

		int ReadByte() override;
		size_t Read(void *, size_t bytes) override;

		void WriteByte(int) override;
		void Write(const void *, size_t bytes) override;

		uint64_t GetPosition() override;
		void SetPosition(uint64_t) override;

		uint64_t GetLength() override;
		void SetLength(uint64_t) override;

		void Flush() override;
	};
}