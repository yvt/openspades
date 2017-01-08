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

#include "ParticleSpriteEntity.h"

namespace spades {
	namespace client {
		class IImage;
		class IRenderer;
		class SmokeSpriteEntity : public ParticleSpriteEntity {
		public:
			enum class Type { Steady, Explosion };

		private:
			float frame;
			float fps;
			Type type;
			static IImage *GetSequence(int i, IRenderer *r, Type);

		public:
			SmokeSpriteEntity(Client *cli, Vector4 color, float fps, Type type = Type::Steady);

			static void Preload(IRenderer *);

			bool Update(float dt) override;
		};
	}
}