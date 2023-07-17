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

#include "Client.h"
#include "ILocalEntity.h"
#include <Core/Math.h>

namespace spades {
	namespace client {
		class IImage;

		enum class BlockHitAction { Delete, Ignore, BounceWeak };

		class ParticleSpriteEntity : public ILocalEntity {
		private:
			IRenderer &renderer;
			Handle<GameMap> map;

			Handle<IImage> image;
			Vector4 color;
			bool additive;
			BlockHitAction blockHitAction;

			Vector3 position, velocity;    // unit/sec
			float radius, radiusVelocity;  // unit/sec
			float angle, rotationVelocity; // radian/sec

			float velocityDamp;
			float radiusDamp;
			float gravityScale;

			float lifetime, time;
			float fadeInDuration;
			float fadeOutDuration;

		public:
			ParticleSpriteEntity(Client &client, Handle<IImage> image, Vector4 color);

			~ParticleSpriteEntity();

			bool Update(float dt) override;
			void Render3D() override;

			void SetAdditive(bool b) { additive = b; }

			void SetLifeTime(float lifeTime, float fadeIn, float fadeOut);

			void SetTrajectory(Vector3 initialPosition, Vector3 initialVelocity,
			                   float velocityDamp = 1.f, float gravityScale = 1.f);

			void SetRotation(float initialAngle, float angleVelocity = 0.f);

			void SetRadius(float initialRadius, float radiusVelocity = 0.f, float radiusDamp = 1.f);

			void SetBlockHitAction(BlockHitAction act) { blockHitAction = act; }

			void SetImage(Handle<IImage> img);
			void SetColor(Vector4 col) { color = col; }

			IRenderer &GetRenderer() { return renderer; }
		};
	} // namespace client
} // namespace spades
