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
#include <string>

#include <opusfile.h>

#include "OpusAudioStream.h"

#include "Debug.h"
#include "Exception.h"
#include <Core/Strings.h>

namespace spades {

	namespace {
		std::string stringifyOpusErrorCode(int error) {
			const char *errorMessage = nullptr;
			switch (error) {
				case OP_EREAD: errorMessage = "OP_EREAD"; break;
				case OP_EFAULT: errorMessage = "OP_EFAULT"; break;
				case OP_EIMPL: errorMessage = "OP_EIMPL"; break;
				case OP_EINVAL: errorMessage = "OP_EINVAL"; break;
				case OP_ENOTFORMAT: errorMessage = "OP_ENOTFORMAT"; break;
				case OP_EBADHEADER: errorMessage = "OP_EBADHEADER"; break;
				case OP_EVERSION: errorMessage = "OP_EVERSION"; break;
				case OP_EBADLINK: errorMessage = "OP_EBADLINK"; break;
				case OP_EBADTIMESTAMP: errorMessage = "OP_EBADTIMESTAMP"; break;
			}
			if (errorMessage) {
				return errorMessage;
			} else {
				return Format("{0}", error);
			}
		}
	}

	OpusAudioStream::OpusAudioStream(IStream *s, bool ac) : subsamplePosition{0} {
		SPADES_MARK_FUNCTION();

		stream = s;
		autoClose = ac;

		opusCallback.reset(new OpusFileCallbacks());
		opusCallback->read = [](void *stream, unsigned char *ptr, int nbytes) -> int {
			auto &self = *reinterpret_cast<OpusAudioStream *>(stream);
			return static_cast<int>(self.stream->Read(ptr, static_cast<int>(nbytes)));
		};
		opusCallback->seek = [](void *stream, opus_int64 offset, int whence) -> int {
			auto &self = *reinterpret_cast<OpusAudioStream *>(stream);
			switch (whence) {
				case SEEK_CUR:
					self.stream->SetPosition(
					  static_cast<std::uint64_t>(offset + self.stream->GetPosition()));
					break;
				case SEEK_SET: self.stream->SetPosition(static_cast<std::uint64_t>(offset)); break;
				case SEEK_END:
					self.stream->SetPosition(
					  static_cast<std::uint64_t>(offset + self.stream->GetLength()));
					break;
			}
			return 0;
		};
		opusCallback->tell = [](void *stream) -> opus_int64 {
			auto &self = *reinterpret_cast<OpusAudioStream *>(stream);
			return static_cast<opus_int64>(self.stream->GetPosition());
		};
		opusCallback->close = nullptr;

		int result = 0;
		opusFile = op_open_callbacks(this, opusCallback.get(), nullptr, 0, &result);

		if (result) {
			SPRaise("op_open_callbacks failed with error code %s",
			        stringifyOpusErrorCode(result).c_str());
		}

		channels = op_channel_count(opusFile, 0);
		currentSample.resize(channels);

		SetPosition(0);
	}

	OpusAudioStream::~OpusAudioStream() {
		SPADES_MARK_FUNCTION();

		op_free(opusFile);

		if (autoClose)
			delete stream;
	}

	uint64_t OpusAudioStream::GetLength() { return op_pcm_total(opusFile, 0) * channels * 4; }

	int OpusAudioStream::GetSamplingFrequency() { return 48000; }

	int OpusAudioStream::GetNumChannels() { return channels; }

	OpusAudioStream::SampleFormat OpusAudioStream::GetSampleFormat() {
		return SampleFormat::SingleFloat;
	}

	int OpusAudioStream::ReadByte() {
		SPADES_MARK_FUNCTION();

		if (subsamplePosition == 0) {
			if (op_pcm_tell(opusFile) >= op_pcm_total(opusFile, 0)) {
				return -1;
			}
			int result = op_read_float(opusFile, currentSample.data(), channels, nullptr);
			if (result < 0) {
				SPRaise("op_read_float failed with error code %s",
				        stringifyOpusErrorCode(result).c_str());
			} else if (result != 1) {
				SPRaise("op_read_float returned %d (expected: 1)", result);
			}
		}

		int ret = reinterpret_cast<const uint8_t *>(currentSample.data())[subsamplePosition];

		if ((++subsamplePosition) == channels * 4) {
			subsamplePosition = 0;
		}

		return ret;
	}

	size_t OpusAudioStream::Read(void *data, size_t bytes) {
		SPADES_MARK_FUNCTION();

		uint64_t maxLen = GetLength() - GetPosition();
		if ((uint64_t)bytes > maxLen)
			bytes = (size_t)maxLen;

		uint8_t *retBytes = reinterpret_cast<uint8_t *>(data);
		uint64_t remainingBytes = bytes;

		if (subsamplePosition) {
			uint64_t copied =
			  std::min(remainingBytes, static_cast<uint64_t>(channels * 4 - subsamplePosition));
			std::memcpy(retBytes,
			            reinterpret_cast<uint8_t *>(currentSample.data()) + subsamplePosition,
			            static_cast<size_t>(copied));
			subsamplePosition += static_cast<int>(copied);
			remainingBytes -= copied;
			retBytes += copied;

			if (subsamplePosition == channels * 4) {
				subsamplePosition = 0;
			}
		}

		while (remainingBytes >= channels * 4) {
			uint64_t numSamples = remainingBytes / (channels * 4);

			// 64-bit sample count might doesn't fit in int which op_read_float's third parameter
			// accepts
			numSamples = std::min<uint64_t>(numSamples, 0x1000000);

			int result = op_read_float(opusFile, reinterpret_cast<float *>(retBytes),
			                           static_cast<int>(numSamples * channels), nullptr);

			if (result < 0) {
				SPRaise("op_read_float failed with error code %s",
				        stringifyOpusErrorCode(result).c_str());
			} else if (result == 0) {
				SPRaise("op_read_float returned 0 (expected: > 0)");
			}

			numSamples = static_cast<uint64_t>(result);

			retBytes += numSamples * (channels * 4);
			remainingBytes -= numSamples * (channels * 4);
		}

		if (remainingBytes) {
			int result = op_read_float(opusFile, currentSample.data(), channels, nullptr);
			if (result < 0) {
				SPRaise("op_read_float failed with error code %s",
				        stringifyOpusErrorCode(result).c_str());
			} else if (result != 1) {
				SPRaise("op_read_float returned %d (expected: 1)", result);
			}

			std::memcpy(retBytes, reinterpret_cast<const uint8_t *>(currentSample.data()),
			            static_cast<size_t>(remainingBytes));

			subsamplePosition = static_cast<int>(remainingBytes);
		}

		return bytes;
	}

	uint64_t OpusAudioStream::GetPosition() {
		if (subsamplePosition) {
			return (op_pcm_tell(opusFile) - 1) * channels * 4 + subsamplePosition;
		} else {
			return op_pcm_tell(opusFile) * channels * 4;
		}
	}

	void OpusAudioStream::SetPosition(uint64_t pos) {
		pos = std::min(pos, GetLength());

		op_pcm_seek(opusFile, static_cast<opus_int64>(pos / (channels * 4)));

		subsamplePosition = static_cast<int>(pos % (channels * 4));
		if (subsamplePosition) {
			int result = op_read_float(opusFile, currentSample.data(), channels, nullptr);
			if (result < 0) {
				SPRaise("op_read_float failed with error code %s",
				        stringifyOpusErrorCode(result).c_str());
			} else if (result != 1) {
				SPRaise("op_read_float returned %d (expected: 1)", result);
			}
		}
	}
}
