//
//  StdStream.cpp
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "StdStream.h"
#include "Exception.h"
#include <unistd.h>
#include "Debug.h"

namespace spades {
	StdStream::StdStream(FILE *f, bool ac):
	handle(f), autoClose(ac){
		SPADES_MARK_FUNCTION();
		
		if(!f)
			SPInvalidArgument("f");
	}
	
	StdStream::~StdStream(){
		SPADES_MARK_FUNCTION();
		
		if(autoClose)
			fclose(handle);
	}
	
	int StdStream::ReadByte() {
		SPADES_MARK_FUNCTION();
		
		int ret = fgetc(handle);
		if(ret == EOF){
			if(ferror(handle)){
				SPRaise("I/O error.");
			}else{
				return -1;
			}
		}else{
			return ret;
		}
	}
	
	size_t StdStream::Read(void *buf, size_t bytes){
		SPADES_MARK_FUNCTION_DEBUG();
		
		return fread(buf, 1, bytes, handle);
	}
	
	void StdStream::WriteByte(int byte) {
		SPADES_MARK_FUNCTION();
		
		if(fputc(byte, handle) != 0){
			SPRaise("I/O error.");
		}
	}
	
	void StdStream::Write(const void *buf, size_t bytes){
		SPADES_MARK_FUNCTION();
		
		if(fwrite(buf, 1, bytes, handle) < bytes){
			SPRaise("I/O error.");
		}
	}
	
	uint64_t StdStream::GetPosition() {
		SPADES_MARK_FUNCTION_DEBUG();
		return ftell(handle);
	}
	
	void StdStream::SetPosition(uint64_t pos){
		SPADES_MARK_FUNCTION();
		
		if(pos > 0x7fffffffULL){
			SPRaise("Currently StdStream doesn't support 64-bit offset.");
		}
		fseek(handle, (long)pos, SEEK_SET);
	}
	
	uint64_t StdStream::GetLength() {
		SPADES_MARK_FUNCTION();
		
		uint64_t opos = GetPosition();
		fseek(handle, 0, SEEK_END);
		uint64_t len = GetPosition();
		SetPosition(opos);
		return len;
	}
	
	void StdStream::SetLength(uint64_t len) {
		SPADES_MARK_FUNCTION_DEBUG();
		
		// this is not safe
		ftruncate(fileno(handle), len);
	}
	
	void StdStream::Flush() {
		fflush(handle);
	}
	
}
