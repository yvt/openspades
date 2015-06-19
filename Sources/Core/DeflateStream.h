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
#include <zlib.h>
#include <vector>

namespace spades {
	class DeflateStream: public IStream {
		IStream *baseStream;
		CompressMode mode;
		z_stream zstream;
		bool autoClose;
		bool valid;
		uint64_t position;
		
		// when compressing:
		//  input buffer
		// when decompressing:
		//  output buffer
		std::vector<char> buffer;
		
		
		// input buffer that wasn't used in FillBuffer
		// is stored here
		std::vector<char> nextbuffer;
		
		// decompression only
		bool reachedEOF;
		size_t bufferPos;
		
		void CompressBuffer();
		void FillBuffer();
	public:
		DeflateStream(IStream *stream, CompressMode mode, bool autoClose = false);
		virtual ~DeflateStream();
		
		virtual int ReadByte();
		virtual size_t Read(void *, size_t bytes);
		
		virtual void WriteByte(int);
		virtual void Write(const void *, size_t bytes);
		
		virtual uint64_t GetPosition();
		virtual void SetPosition(uint64_t);
		
		virtual uint64_t GetLength();
		virtual void SetLength(uint64_t);
		
		/** Must be called when all data was written, in case of compressing, or output will be corrupted */
		void DeflateEnd();
	};
}
