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

#include <cstdint>
#include <cstdio>
#include <string>

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
		virtual void Write(const std::string &);

		virtual uint64_t GetPosition();
		virtual void SetPosition(uint64_t);

		virtual uint64_t GetLength();
		virtual void SetLength(uint64_t);

		virtual void Flush() {}

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
		StreamHandle(const StreamHandle &);
		~StreamHandle();
		spades::StreamHandle &operator=(const StreamHandle &);
		void Reset();
		IStream *operator->() const;
		operator IStream *() const;
	};
}
