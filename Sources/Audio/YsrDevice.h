//
//  YsrDevice.h
//  OpenSpades
//
//  Created by Tomoaki Kawada on 12/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <Client/IAudioDevice.h>
#include <map>
#include <memory>
#include <array>

namespace spades {
	namespace audio {
		
		class YsrDriver;
		class YsrAudioChunk;
		
		class YsrDevice: public client::IAudioDevice {
			std::shared_ptr<YsrDriver> driver;
			client::GameMap *gameMap;
			
			int roomHistoryPos;
			enum { RoomHistorySize = 128 };
			std::array<float, RoomHistorySize> roomHistory;
			std::array<float, RoomHistorySize> roomFeedbackHistory;
			
			std::map<std::string, YsrAudioChunk *> chunks;
			YsrAudioChunk *CreateChunk(const char *name);
		public:
			YsrDevice();
			virtual ~YsrDevice();
			
			virtual client::IAudioChunk *RegisterSound(const char *name);
			
			virtual void SetGameMap(client::GameMap *);
			
			virtual void Play(client::IAudioChunk *, const Vector3& origin, const client::AudioParam&);
			virtual void PlayLocal(client::IAudioChunk *, const Vector3& origin, const client::AudioParam&);
			virtual void PlayLocal(client::IAudioChunk *, const client::AudioParam&);
			
			virtual void Respatialize(const Vector3& eye,
									  const Vector3& front,
									  const Vector3& up);
		};
	}
}
