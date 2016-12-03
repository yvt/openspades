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

#include "../Core/Math.h"

namespace spades {
	namespace client {
		extern int palette[32][3];
		class Client;
		class IRenderer;
		class IImage;
		class MapView {
			Client *client;
			IRenderer *renderer;

			int scaleMode;
			float actualScale;
			float lastScale; // used for animation

			float zoomState;
			bool zoomed;

			bool largeMap;

			AABB2 inRect;
			AABB2 outRect;

			void DrawIcon(Vector3 pos, IImage *img, float rotation);

		public:
			MapView(Client *, bool largeMap);
			~MapView();

			void Update(float dt);
			void SwitchScale();
			bool ToggleZoom();

			void Draw();
		};
	}
}