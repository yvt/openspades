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

#include <algorithm>

#include <Core/Debug.h>
#include "Debug.h"
#include "Exception.h"
#include "IStream.h"

namespace spades {
	IStream::~IStream() {}
	int IStream::ReadByte() { SPUnsupported(); }

	size_t IStream::Read(void *out, size_t bytes) {
		SPADES_MARK_FUNCTION();

		char *buf = reinterpret_cast<char *>(out);
		size_t read = 0;
		while (bytes--) {
			int b = ReadByte();
			if (b == -1)
				break;
			*(buf++) = (char)b;
			read++;
		}
		return read;
	}

	std::string IStream::Read(size_t maxBytes) {
		SPADES_MARK_FUNCTION();

		std::string str;
		char buf[2048];
		size_t readBytes;
		while ((readBytes = Read(buf, std::min((size_t)2048, maxBytes))) > 0) {
			str.append(buf, readBytes);
			maxBytes -= readBytes;
		}
		return str;
	}

	std::string IStream::ReadAllBytes() {
		SPADES_MARK_FUNCTION();

		return Read(0x7fffffffUL);
	}

	void IStream::WriteByte(int) {
		SPADES_MARK_FUNCTION();

		SPUnsupported();
	}

	void IStream::Write(const void *inp, size_t bytes) {
		SPADES_MARK_FUNCTION();

		const unsigned char *buf = reinterpret_cast<const unsigned char *>(inp);
		while (bytes--) {
			WriteByte((int)*(buf++));
		}
	}

	void IStream::Write(const std::string &str) {
		SPADES_MARK_FUNCTION();

		Write(str.data(), str.size());
	}

	uint64_t IStream::GetPosition() { SPUnsupported(); }

	void IStream::SetPosition(uint64_t pos) { SPUnsupported(); }

	uint64_t IStream::GetLength() { SPUnsupported(); }

	void IStream::SetLength(uint64_t) { SPUnsupported(); }

	uint16_t IStream::ReadLittleShort() {
		SPADES_MARK_FUNCTION();

		// TODO: big endian support
		uint16_t data;
		if (Read(&data, 2) < 2)
			SPRaise("Failed to read 2 bytes");
		return data;
	}

	uint32_t IStream::ReadLittleInt() {
		SPADES_MARK_FUNCTION();

		// TODO: big endian support
		uint32_t data;
		if (Read(&data, 4) < 4)
			SPRaise("Failed to read 4 bytes");
		return data;
	}

	StreamHandle::StreamHandle() : o(NULL) {}

	StreamHandle::StreamHandle(IStream *stream) {
		SPADES_MARK_FUNCTION();
		if (!stream)
			SPInvalidArgument("stream");
		o = new SharedStream(stream);
	}

	StreamHandle::StreamHandle(const StreamHandle &handle) : o(handle.o) {
		SPADES_MARK_FUNCTION_DEBUG();
		o->Retain();
	}

	StreamHandle::~StreamHandle() {
		SPADES_MARK_FUNCTION();
		Reset();
	}

	spades::StreamHandle &StreamHandle::operator=(const spades::StreamHandle &h) {
		SPADES_MARK_FUNCTION();
		if (o != h.o) {
			SharedStream *old = o;
			o = h.o;
			o->Retain();
			old->Release();
		}
		return *this;
	}

	IStream *StreamHandle::operator->() const {
		SPAssert(o);
		return o->stream;
	}

	StreamHandle::operator class spades::IStream *() const {
		SPAssert(o);
		return o->stream;
	}

	void StreamHandle::Reset() {
		if (o) {
			o->Release();
			o = NULL;
		}
	}

	StreamHandle::SharedStream::SharedStream(IStream *s) : stream(s), refCount(1) {}

	StreamHandle::SharedStream::~SharedStream() { delete stream; }

	void StreamHandle::SharedStream::Retain() { refCount++; }

	void StreamHandle::SharedStream::Release() {
		SPAssert(refCount > 0);
		refCount--;
		if (refCount == 0)
			delete this;
	}
}
