/*
 Copyright (c) 2016 yvt

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

#include <tuple>
#include <regex>

#include "AudioStream.h"

#include "WavAudioStream.h"
#include "OpusAudioStream.h"
#include <Core/FileManager.h>
#include <Core/Exception.h>

namespace spades {
	namespace {
		std::regex const wavRegex{".*\\.wav", std::regex::icase};
		std::regex const opusRegex{".*\\.(?:opus|ogg)", std::regex::icase};

		using CodecInfo = std::tuple<std::string, IAudioStream *(*)(IStream *stream, bool autoClose), std::regex const &>;
		CodecInfo g_codecs[] = {
			CodecInfo {"WAV Decoder", [] (IStream *stream, bool autoClose) -> IAudioStream * {
				return new WavAudioStream(stream, autoClose);
			}, wavRegex},
			CodecInfo {"Opus Decoder", [] (IStream *stream, bool autoClose) -> IAudioStream * {
				return new OpusAudioStream(stream, autoClose);
			}, opusRegex}
		};
	}

	IAudioStream *OpenAudioStream(const std::string &fileName) {
		std::string errMsg;
		for (const auto &codec: g_codecs) {
			if (!std::regex_match(fileName, std::get<2>(codec))) {
				continue;
			}

			// give it a try.
			// open error shouldn't be handled here
			auto stream = FileManager::OpenForReading(fileName.c_str());
			try {
				auto parsedStream = std::get<1>(codec)(stream.get(), true);

				// The ownership of `stream` moves to `parsedStream` if the load
				// succeeds
				stream.release();

				return parsedStream;
			} catch (const std::exception &ex) {
				errMsg += std::get<0>(codec);
				errMsg += ":\n";
				errMsg += ex.what();
				errMsg += "\n\n";
			}
		}

		if (errMsg.empty()) {
			SPRaise("Audio codec not found for filename: %s", fileName.c_str());
		} else {
			SPRaise("No audio codec could load file successfully: %s\n%s\n", fileName.c_str(),
					errMsg.c_str());
		}
	}
}
