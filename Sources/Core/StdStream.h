//
//  StdStream.h
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <stdio.h>
#include "IStream.h"

namespace spades {
	class StdStream: public IStream {
		FILE *handle;
		bool autoClose;
		StdStream(const StdStream&){}
		void operator =(const StdStream&){}
	public:
		StdStream(FILE *f, bool autoClose = false);
		virtual ~StdStream();
		
		virtual int ReadByte();
		virtual size_t Read(void *, size_t bytes);
		
		virtual void WriteByte(int);
		virtual void Write(const void *, size_t bytes);
		
		virtual uint64_t GetPosition();
		virtual void SetPosition(uint64_t);
		
		virtual uint64_t GetLength();
		virtual void SetLength(uint64_t);
		
		virtual void Flush();
	};
}