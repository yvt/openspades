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

#include "ILocalEntity.h"
#include "IModel.h"
#include <Core/Math.h>

namespace spades {
	namespace client {
		class IRenderer;
		class Client;
		class IAudioChunk;
		class GunCasing : public ILocalEntity {
			Client *client;
			IRenderer *renderer;
			IModel *model;
			Matrix4 matrix;
			Vector3 rotAxis;
			Vector3 vel;

			IAudioChunk *dropSound;
			IAudioChunk *waterSound;

			bool onGround;
			IntVector3 groundPos;
			float groundTime;
			float rotSpeed;

		public:
			GunCasing(Client *client, IModel *model, IAudioChunk *dropSound,
			          IAudioChunk *waterSound, Vector3 pos, Vector3 dir, Vector3 flyDir);
			~GunCasing();
			bool Update(float dt) override;
			void Render3D() override;
		};
	}
}