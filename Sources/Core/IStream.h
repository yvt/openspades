//
//  IStream.h
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <stdio.h>
#include <string>
#include <stdint.h>

namespace spades {
	/** operation mode for data-compression streams. */
	enum CompressMode {
		/** Compresses and writes to the base stream. */
		CompressModeCompress = 0,
		
		/** Decompresses the base stream. */
		CompressModeDecompress
	};
	
	class IStream {
	protected:
		IStream() {}
	public:
		virtual ~IStream();
		/** reads one byte and return in range [0, 255]. 
		 * -1 if EOF reached. */
		virtual int ReadByte();
		virtual size_t Read(void *, size_t bytes);
		virtual std::string Read(size_t maxBytes);
		
		virtual void WriteByte(int);
		virtual void Write(const void *, size_t bytes);
		virtual void Write(const std::string&);
		
		virtual uint64_t GetPosition();
		virtual void SetPosition(uint64_t);
		
		virtual uint64_t GetLength();
		virtual void SetLength(uint64_t);
		
		uint16_t ReadLittleShort();
		uint32_t ReadLittleInt();
		
		// utilities
		virtual std::string ReadAllBytes();
	};
	
	/** makes management of stream lifetime easier. 
	 * don't create multiple StreamHandles with the same IStream. */
	class StreamHandle {
		struct SharedStream {
			IStream *stream;
			int refCount;
			SharedStream(IStream *);
			~SharedStream();
			void Retain();
			void Release();
		};
		SharedStream *o;
	public:
		StreamHandle();
		StreamHandle(IStream *);
		StreamHandle(const StreamHandle&);
		~StreamHandle();
		void operator =(const StreamHandle&);
		void Reset();
		IStream *operator ->() const;
		operator IStream *() const;
	};
}
