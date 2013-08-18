//
//  DeflateStream.h
//  OpenSpades
//
//  Created by yvt on 7/16/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

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
