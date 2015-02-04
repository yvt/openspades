/*
 Copyright (c) 2013 OpenSpades Developers
 
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

#include "IStream.h"
#include <vector>

namespace spades {
	class DynamicMemoryStream: public IStream {
		std::vector<unsigned char> memory;
		uint64_t position;
	public:
		DynamicMemoryStream();
		virtual ~DynamicMemoryStream();
		
		virtual int ReadByte();
		virtual size_t Read(void *, size_t bytes);
		virtual std::string Read(size_t maxBytes);
		
		virtual void WriteByte(int);
		virtual void Write(const void *, size_t bytes);
		
		virtual uint64_t GetPosition();
		virtual void SetPosition(uint64_t);
		
		virtual uint64_t GetLength();
		/** prohibited */
		virtual void SetLength(uint64_t);
	};
}