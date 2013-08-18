//
//  DynamicMemoryStream.h
//  OpenSpades
//
//  Created by yvt on 8/9/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

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