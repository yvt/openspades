//
//  IAudioStream.cpp
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "IAudioStream.h"
#include "Exception.h"
#include "Debug.h"

namespace spades {
	uint64_t IAudioStream::GetNumSamples() {
		return GetLength() / (uint64_t)GetStride();
	}
	int IAudioStream::GetStride() {
		SPADES_MARK_FUNCTION();
		
		int stride;
		switch(GetSampleFormat()){
			case UnsignedByte: stride = 1; break;
			case SignedShort: stride = 2; break;
			case SingleFloat: stride = 4; break;
			default:
				SPInvalidEnum("GetSampleFormat", GetSampleFormat());
		}
		
		return stride * GetNumChannels();
	}
}
