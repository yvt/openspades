/*
 Copyright (c) 2013 OpenSpades Developers
 
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

#pragma once

#include <Client/IAudioDevice.h>
#include <map>
#include <memory>
#include <array>

namespace spades {
	namespace audio {
				
		class NullDevice: public client::IAudioDevice {
		protected:
			virtual ~NullDevice();
		public:
			NullDevice();
			
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
