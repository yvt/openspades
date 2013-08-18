//
//  IAudioStream.h
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IStream.h"

namespace spades {
	class IAudioStream: public IStream {
	public:
		enum SampleFormat {
			UnsignedByte,
			SignedShort,
			SingleFloat
		};
		virtual int GetSamplingFrequency() = 0;
		virtual SampleFormat GetSampleFormat() = 0;
		virtual int GetNumChannels() = 0;
		uint64_t GetNumSamples();
		int GetStride();
	};
}
