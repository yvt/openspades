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

#include "ParticleSpriteEntity.h"
#include <Core/Debug.h>
#include "GameMap.h"
#include "World.h"

namespace spades {
	namespace client {
		ParticleSpriteEntity::ParticleSpriteEntity(Client *cli, IImage *image, Vector4 color)
		    : image(image), color(color) {
			position = MakeVector3(0, 0, 0);
			velocity = MakeVector3(0, 0, 0);
			radius = 1.f;
			radiusVelocity = 0.f;
			angle = 0.f;
			rotationVelocity = 0.f;
			velocityDamp = 1;
			gravityScale = 1.f;
			lifetime = 1.f;
			radiusDamp = 1.f;
			time = 0.f;
			fadeInDuration = .1f;
			fadeOutDuration = .5f;
			additive = false;
			blockHitAction = Delete;

			if (image != NULL)
				image->AddRef();

			renderer = cli->GetRenderer();
			if (cli->GetWorld())
				map = cli->GetWorld()->GetMap();
			else
				map = NULL;
		}

		ParticleSpriteEntity::~ParticleSpriteEntity() {
			if (image != NULL) {
				image->Release();
			}
		}

		void ParticleSpriteEntity::SetLifeTime(float lifeTime, float fadeIn, float fadeOut) {
			lifetime = lifeTime;
			fadeInDuration = fadeIn;
			fadeOutDuration = fadeOut;
		}
		void ParticleSpriteEntity::SetTrajectory(Vector3 pos, Vector3 vel, float damp, float grav) {
			position = pos;
			velocity = vel;
			velocityDamp = damp;
			gravityScale = grav;
		}

		void ParticleSpriteEntity::SetRotation(float initialAngle, float angleVelocity) {
			angle = initialAngle;
			rotationVelocity = angleVelocity;
		}

		void ParticleSpriteEntity::SetRadius(float initialRadius, float radiusVelocity,
		                                     float damp) {
			radius = initialRadius;
			this->radiusVelocity = radiusVelocity;
			radiusDamp = damp;
		}

		bool ParticleSpriteEntity::Update(float dt) {
			SPADES_MARK_FUNCTION_DEBUG();
			Vector3 lastPos = position;

			time += dt;
			if (time > lifetime)
				return false;

			position += velocity * dt;
			velocity.z += 32.f * dt * gravityScale;

			// TODO: control clip action
			if (blockHitAction != Ignore && map) {
				if (map->ClipWorld(position.x, position.y, position.z)) {
					if (blockHitAction == Delete) {
						return false;
					} else {
						IntVector3 lp2 = lastPos.Floor();
						IntVector3 lp = position.Floor();
						if (lp.z != lp2.z && ((lp.x == lp2.x && lp.y == lp2.y) ||
						                      !map->ClipWorld(lp.x, lp.y, lp2.z)))
							velocity.z = -velocity.z;
						else if (lp.x != lp2.x && ((lp.y == lp2.y && lp.z == lp2.z) ||
						                           !map->ClipWorld(lp2.x, lp.y, lp.z)))
							velocity.x = -velocity.x;
						else if (lp.y != lp2.y && ((lp.x == lp2.x && lp.z == lp2.z) ||
						                           !map->ClipWorld(lp.x, lp2.y, lp.z)))
							velocity.y = -velocity.y;
						velocity *= .36f;
						position = lastPos;
					}
				}
			}

			// radius
			if (radiusVelocity != 0.f)
				radius += radiusVelocity * dt;

			if (rotationVelocity != 0.f)
				angle += rotationVelocity * dt;

			if (velocityDamp != 1.f)
				velocity *= powf(velocityDamp, dt);

			if (radiusDamp != 1.f)
				radiusVelocity *= powf(radiusDamp, dt);

			return true;
		}

		void ParticleSpriteEntity::Render3D() {
			SPADES_MARK_FUNCTION_DEBUG();

			float fade = 1.f;
			if (time < fadeInDuration) {
				fade *= time / fadeInDuration;
			}
			if (time > lifetime - fadeOutDuration) {
				fade *= (lifetime - time) / fadeOutDuration;
			}

			Vector4 col = color;
			col.w *= fade;

			// premultiplied alpha!
			col.x *= col.w;
			col.y *= col.w;
			col.z *= col.w;

			if (additive)
				col.w = 0.f;

			renderer->SetColorAlphaPremultiplied(col);
			renderer->AddSprite(image, position, radius, angle);
		}
		void ParticleSpriteEntity::SetImage(IImage *img) {
			if (img == image)
				return;
			if (image != NULL)
				image->Release();
			image = img;
			if (image != NULL)
				image->AddRef();
		}
	}
}
