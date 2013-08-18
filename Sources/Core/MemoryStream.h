//
//  MemoryStream.h
//  OpenSpades
//
//  Created by yvt on 7/16/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IStream.h"

namespace spades {
	class MemoryStream: public IStream {
		unsigned char *memory;
		uint64_t position;
		uint64_t length;
		bool canWrite;
	public:
		MemoryStream(char *buffer, size_t length, bool allowWrite);
		MemoryStream(const char *buffer, size_t length);
		virtual ~MemoryStream();
		
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
