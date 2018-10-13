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

#include <cstring>

#include <Core/Debug.h>
#include "Debug.h"
#include "DeflateStream.h"
#include "Exception.h"

namespace spades {
	static const size_t chunkSize = 16384;
	static const size_t bufferSize = 65536;
	DeflateStream::DeflateStream(IStream *stream, CompressMode mode, bool ac) {
		SPADES_MARK_FUNCTION();

		this->baseStream = stream;
		this->mode = mode;
		autoClose = ac;

		zstream.zalloc = Z_NULL;
		zstream.zfree = Z_NULL;
		zstream.opaque = Z_NULL;

		position = 0;

		int ret;
		if (mode == CompressModeCompress) {
			ret = deflateInit(&zstream, 5);
		} else if (mode == CompressModeDecompress) {
			ret = inflateInit(&zstream);
		} else {
			SPInvalidEnum("mode", mode);
		}

		if (ret != Z_OK) {
			SPRaise("Failed to initialize zlib deflator/inflator: %s", zError(ret));
		}

		valid = true;
		reachedEOF = false;
		bufferPos = 0;
	}
	DeflateStream::~DeflateStream() {
		SPADES_MARK_FUNCTION();
		if (valid) {
			if (mode == CompressModeCompress) {
				deflateEnd(&zstream);
			} else if (mode == CompressModeDecompress) {
				inflateEnd(&zstream);
			} else {
				SPUnreachable();
			}
		}
		if (autoClose) {
			delete baseStream;
		}
	}

#pragma mark - Deflate

	void DeflateStream::CompressBuffer() {
		SPADES_MARK_FUNCTION();
		SPAssert(mode == CompressModeCompress);
		char outputBuffer[chunkSize];

		if (!valid) {
			SPRaise("State is invalid");
		}

		zstream.avail_in = (unsigned int)buffer.size();
		zstream.next_in = (Bytef *)buffer.data();

		do {
			zstream.avail_out = chunkSize;
			zstream.next_out = (Bytef *)outputBuffer;
			int ret = deflate(&zstream, Z_NO_FLUSH);
			if (ret == Z_STREAM_ERROR) {
				valid = false;
				deflateEnd(&zstream);
				SPRaise("Error while deflating: %s", zError(ret));
			}

			int got = chunkSize - zstream.avail_out;
			baseStream->Write(outputBuffer, got);
		} while (zstream.avail_out == 0);

		SPAssert(zstream.avail_in == 0);
		std::vector<char>().swap(buffer);
	}

	void DeflateStream::DeflateEnd() {
		SPADES_MARK_FUNCTION();
		if (mode != CompressModeCompress) {
			SPRaise("DeflareEnd called when decompressing");
		}
		if (!valid) {
			SPRaise("State is invalid");
		}

		char outputBuffer[chunkSize];

		zstream.avail_in = 0;
		do {
			zstream.avail_out = chunkSize;
			zstream.next_out = (Bytef *)outputBuffer;
			int ret = deflate(&zstream, Z_FINISH);
			if (ret == Z_STREAM_ERROR) {
				valid = false;
				deflateEnd(&zstream);
				SPRaise("Error while deflating: %s", zError(ret));
			}

			int got = chunkSize - zstream.avail_out;
			baseStream->Write(outputBuffer, got);
		} while (zstream.avail_out == 0);

		deflateEnd(&zstream);
		valid = false;
	}

	void DeflateStream::WriteByte(int byte) {
		SPADES_MARK_FUNCTION();
		if (mode != CompressModeCompress) {
			SPRaise("Attempted to write when decompressing");
		}
		if (!valid) {
			SPRaise("State is invalid");
		}

		if (buffer.size() >= bufferSize) {
			CompressBuffer();
		}

		buffer.push_back((char)byte);
	}

	void DeflateStream::Write(const void *data, size_t bytes) {
		SPADES_MARK_FUNCTION();
		if (mode != CompressModeCompress) {
			SPRaise("Attempted to write when decompressing");
		}
		if (!valid) {
			SPRaise("State is invalid");
		}

		if (buffer.size() >= bufferSize) {
			CompressBuffer();
		}

		const char *dt = reinterpret_cast<const char *>(data);
		buffer.insert(buffer.end(), dt, dt + bytes);
	}

#pragma mark - inflateEnd
	void DeflateStream::FillBuffer() {
		SPAssert(bufferPos >= buffer.size());
		buffer.clear();

		SPADES_MARK_FUNCTION();
		SPAssert(mode == CompressModeDecompress);
		char inputBuffer[chunkSize];
		char outputBuffer[chunkSize];
#ifndef NDEBUG
		uLong oval = zstream.total_out;
#endif

		if (reachedEOF)
			return;
		if (!valid) {
			SPRaise("State is invalid");
		}

		while (buffer.size() < bufferSize) {

			size_t readSize;
			readSize = chunkSize - nextbuffer.size();
			for (size_t i = 0; i < nextbuffer.size(); i++)
				inputBuffer[i] = nextbuffer[i];
			readSize = baseStream->Read(inputBuffer + nextbuffer.size(), readSize);
			readSize += nextbuffer.size();
			zstream.avail_in = (unsigned int)readSize;
			zstream.next_in = (Bytef *)inputBuffer;

			do {
				zstream.avail_out = chunkSize;
				zstream.next_out = (Bytef *)outputBuffer;
				int ret = inflate(&zstream, Z_NO_FLUSH);
				if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR ||
				    ret == Z_MEM_ERROR) {
					valid = false;
					inflateEnd(&zstream);
					SPRaise("Error while inflating: %s", zError(ret));
				}
				if (ret == Z_STREAM_END && zstream.avail_out != 0) {
					reachedEOF = true;
				}
				int got = chunkSize - zstream.avail_out;
				buffer.insert(buffer.end(), outputBuffer, outputBuffer + got);
			} while (zstream.avail_out == 0 && !reachedEOF);

			if (reachedEOF)
				break;
			else {
				if (readSize == 0) {
					valid = false;
					inflateEnd(&zstream);
					SPRaise("EOF reached while reading compressed data");
				}
				nextbuffer.resize(zstream.avail_in);
				for (size_t i = 0; i < zstream.avail_in; i++)
					nextbuffer[i] = zstream.next_in[i];
			}
		}

		SPAssert(buffer.size() == zstream.total_out - oval);
		bufferPos = 0;
	}

	int DeflateStream::ReadByte() {
		SPADES_MARK_FUNCTION();
		if (bufferPos >= buffer.size()) {
			FillBuffer();
		}
		if (bufferPos >= buffer.size()) {
			SPAssert(reachedEOF);
			return -1;
		}
		position++;
		return (unsigned char)buffer[bufferPos++];
	}

	size_t DeflateStream::Read(void *data, size_t bytes) {
		SPADES_MARK_FUNCTION();
		size_t readBytes = 0;
		while (bytes > 0) {
			if (bufferPos >= buffer.size()) {
				FillBuffer();
				if (reachedEOF && bufferPos >= buffer.size())
					break;
			}

			size_t copySize = bytes;
			if (copySize > (buffer.size() - bufferPos))
				copySize = buffer.size() - bufferPos;

			memcpy(data, buffer.data() + bufferPos, copySize);

			reinterpret_cast<char *&>(data) += copySize;
			SPAssert(bytes >= copySize);
			bytes -= copySize;
			readBytes += copySize;
			bufferPos += copySize;
			position += copySize;
		}
		return readBytes;
	}

#pragma mark - Seek

	uint64_t DeflateStream::GetPosition() { return position; }
	void DeflateStream::SetPosition(uint64_t pos) {
		SPADES_MARK_FUNCTION();
		SPRaise("Invalid operation");
	}

	uint64_t DeflateStream::GetLength() {
		SPADES_MARK_FUNCTION();
		if (mode == CompressModeCompress) {
			return position;
		} else {
			SPRaise("Cannot retrieve uncompressed data length");
		}
	}

	void DeflateStream::SetLength(uint64_t) {
		SPADES_MARK_FUNCTION();
		SPRaise("Invalid operation");
	}
}
