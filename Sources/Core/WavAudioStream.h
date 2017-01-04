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

#include <map>
#include <string>

#include "IAudioStream.h"

namespace spades {
	class WavAudioStream : public IAudioStream {
		struct RiffChunkInfo {
			uint64_t dataPosition;
			std::string name;
			uint64_t length;
		};

		std::map<std::string, RiffChunkInfo> chunks;

		IStream *stream;
		bool autoClose;

		int channels;
		int rate;
		SampleFormat sampleFormat;

		const RiffChunkInfo *dataChunk;
		uint64_t startPos, endPos;

		RiffChunkInfo ReadChunkInfo();
		const RiffChunkInfo &GetChunk(const std::string &);

	public:
		WavAudioStream(IStream *, bool autoClose);
		~WavAudioStream();

		uint64_t GetLength() override;
		int GetSamplingFrequency() override;
		SampleFormat GetSampleFormat() override;
		int GetNumChannels() override;

		int ReadByte() override;
		size_t Read(void *, size_t bytes) override;

		uint64_t GetPosition() override;
		void SetPosition(uint64_t) override;
	};
}
