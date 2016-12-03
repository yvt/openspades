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

#include "IAudioStream.h"
#include "Debug.h"
#include "Exception.h"

namespace spades {
	uint64_t IAudioStream::GetNumSamples() { return GetLength() / (uint64_t)GetStride(); }
	int IAudioStream::GetStride() {
		SPADES_MARK_FUNCTION();

		int stride;
		switch (GetSampleFormat()) {
			case UnsignedByte: stride = 1; break;
			case SignedShort: stride = 2; break;
			case SingleFloat: stride = 4; break;
			default: SPInvalidEnum("GetSampleFormat", GetSampleFormat());
		}

		return stride * GetNumChannels();
	}
}
