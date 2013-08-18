//
//  ALDevice.h
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Client/IAudioDevice.h"
#include <map>

namespace spades {
	namespace audio {
		
		class ALAudioChunk;
		
		class ALDevice: public client::IAudioDevice {
			bool useEAX;
			class Internal;
			Internal *d;
			
			std::map<std::string, ALAudioChunk *> chunks;
			ALAudioChunk *CreateChunk(const char *name);
		public:
			ALDevice();
			virtual ~ALDevice();
			
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
