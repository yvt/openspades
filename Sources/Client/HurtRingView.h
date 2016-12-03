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

#include <list>

#include <Core/Math.h>

namespace spades {
	namespace client {
		class IRenderer;
		class Client;
		class IImage;

		class HurtRingView {
			Client *client;
			IRenderer *renderer;
			IImage *image;

			struct Item {
				Vector3 dir;
				float fade;
			};

			std::list<Item> items;

		public:
			HurtRingView(Client *);
			~HurtRingView();

			void ClearAll();
			void Add(Vector3);

			void Update(float dt);
			void Draw();
		};
	}
}
