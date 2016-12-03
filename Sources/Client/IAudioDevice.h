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

#pragma once

#include <Core/Math.h>
#include <Core/RefCountedObject.h>

namespace spades {
	namespace client {
		class IAudioChunk;
		class GameMap;

		struct AudioParam {
			float volume;
			float pitch;
			float referenceDistance;

			AudioParam() {
				volume = 1.f;
				pitch = 1.f;
				referenceDistance = 1.f;
			}
		};

		class IAudioDevice : public RefCountedObject {
		public:
			virtual ~IAudioDevice() {}

			virtual IAudioChunk *RegisterSound(const char *name) = 0;

			virtual void SetGameMap(GameMap *) = 0;

			virtual void Play(IAudioChunk *, const Vector3 &origin, const AudioParam &) = 0;
			virtual void PlayLocal(IAudioChunk *, const Vector3 &origin, const AudioParam &) = 0;
			virtual void PlayLocal(IAudioChunk *, const AudioParam &) = 0;

			virtual void Respatialize(const Vector3 &eye, const Vector3 &front,
			                          const Vector3 &up) = 0;
		};
	}
}
