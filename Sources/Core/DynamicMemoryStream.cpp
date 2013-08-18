//
//  DynamicMemoryStream.cpp
//  OpenSpades
//
//  Created by yvt on 8/9/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "DynamicMemoryStream.h"
#include "../Core/Debug.h"
#include "../Core/Exception.h"
#include <stdlib.h>
#include <string.h>

namespace spades {
#define MaxSize ((uint64_t)((size_t)(-1)))
	DynamicMemoryStream::DynamicMemoryStream(): position(0) {
		SPADES_MARK_FUNCTION();
	}
	DynamicMemoryStream::~DynamicMemoryStream() {
		SPADES_MARK_FUNCTION();
		
	}
	
	int DynamicMemoryStream::ReadByte() {
		SPADES_MARK_FUNCTION();
		if(position >= (uint64_t)memory.size()){
			return -1;
		}else{
			return memory[(size_t)(position++)];
		}
	}
	
	size_t DynamicMemoryStream::Read(void *buf, size_t bytes) {
		SPADES_MARK_FUNCTION();
		
		if(position >= (uint64_t)memory.size()) {
			return 0;
		}
		
		uint64_t maxBytes = memory.size() - (size_t)position;
		if((uint64_t)bytes > maxBytes)
			bytes = (size_t)maxBytes;
		
		memcpy(buf, memory.data() + (size_t)position, bytes);
		position += bytes;
		return bytes;
	}
	
	std::string DynamicMemoryStream::Read(size_t bytes) {
		SPADES_MARK_FUNCTION();
		
		if(position >= (uint64_t)memory.size()) {
			return std::string();
		}
		
		uint64_t maxBytes = memory.size() - (size_t)position;
		if((uint64_t)bytes > maxBytes)
			bytes = (size_t)maxBytes;
		
		std::string ret((const char *)(memory.data() + (size_t)position), bytes);
		position += bytes;
		return ret;
	}
	
	void DynamicMemoryStream::WriteByte(int b) {
		SPADES_MARK_FUNCTION();
		
		if((uint64_t)position + 1 > MaxSize) {
			SPRaise("Memory block size exceeded");
		}
		
		size_t minSize = (size_t)position + 1;
		if(minSize > memory.size())
			memory.resize(minSize);
		
		memory[(size_t)position] = b;
		position++;
	}
	
	void DynamicMemoryStream::Write(const void *data, size_t bytes) {
		
		if((uint64_t)position + (uint64_t)bytes > MaxSize ||
		   (uint64_t)position + (uint64_t)bytes < position) {
			SPRaise("Memory block size exceeded");
		}
		
		size_t minSize = (size_t)position + bytes;
		if(minSize > memory.size())
			memory.resize(minSize);
		
		memcpy(memory.data() + (size_t)position, data, bytes);
		position+= bytes;

	}
	
	uint64_t DynamicMemoryStream::GetPosition() {
		return position;
	}
	
	void DynamicMemoryStream::SetPosition(uint64_t pos) {
		position = pos;
	}
	
	uint64_t DynamicMemoryStream::GetLength() {
		return (uint64_t)memory.size();
	}
	
	void DynamicMemoryStream::SetLength(uint64_t l){
		if(l > MaxSize) {
			SPRaise("Memory block size exceeded");
		}
		memory.resize((size_t)l);
	}
}


