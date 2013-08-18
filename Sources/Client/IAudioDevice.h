//
//  IAudioDevice.h
//  OpenSpades
//
//  Created by yvt on 7/11/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"

namespace spades {
	namespace client {
		class IAudioChunk;
		class GameMap;
		
		struct AudioParam {
			float volume;
			float pitch;
			float referenceDistance;
			
			AudioParam(){
				volume = 1.f;
				pitch = 1.f;
				referenceDistance = 1.f;
			}
		};
		
		class IAudioDevice {
		public:
			virtual ~IAudioDevice() {}
			
			virtual IAudioChunk *RegisterSound(const char *name) = 0;
			
			virtual void SetGameMap(GameMap *) = 0;
			
			virtual void Play(IAudioChunk *, const Vector3& origin, const AudioParam&) = 0;
			virtual void PlayLocal(IAudioChunk *, const Vector3& origin, const AudioParam&) = 0;
			virtual void PlayLocal(IAudioChunk *, const AudioParam&) = 0;
			
			virtual void Respatialize(const Vector3& eye,
									  const Vector3& front,
									  const Vector3& up) = 0;
		};
	}
}
