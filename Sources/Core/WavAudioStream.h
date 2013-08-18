//
//  WavAudioStream.h
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "IAudioStream.h"
#include <string>
#include <map>

namespace spades {
	class WavAudioStream: public IAudioStream {
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
		const RiffChunkInfo& GetChunk(const std::string&);
	public:
		WavAudioStream(IStream *, bool autoClose);
		virtual ~WavAudioStream();
		
		virtual uint64_t GetLength();
		virtual int GetSamplingFrequency();
		virtual SampleFormat GetSampleFormat();
		virtual int GetNumChannels();
		
		virtual int ReadByte();
		virtual size_t Read(void *, size_t bytes);
		
		virtual uint64_t GetPosition();
		virtual void SetPosition(uint64_t);
		
	};
}
