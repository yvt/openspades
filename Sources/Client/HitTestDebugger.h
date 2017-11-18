/*
 Copyright (c) 2013 yvt
 based on code of pysnip (c) Mathias Kaerlev 2011-2012.

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

#include <array>
#include <map>
#include <vector>

#include <Core/Math.h>
#include <Core/RefCountedObject.h>

namespace spades {
	namespace client {
		class IRenderer;
		class World;

		/** HitTestDebugger is used to debug hit detection issues. */
		class HitTestDebugger {
			class Port;

			Handle<IRenderer> renderer;
			World *world; // weak ref
			Handle<Port> port;

		public:
			HitTestDebugger(World *world);
			~HitTestDebugger();
			HitTestDebugger(const HitTestDebugger &) = delete;
			void operator=(const HitTestDebugger &) = delete;

			struct PlayerHit {
				int numHeadHits;
				std::array<int, 3> numLimbHits;
				int numTorsoHits;
				PlayerHit() : numHeadHits(0), numLimbHits({{0, 0, 0}}), numTorsoHits(0) {}
			};

			/** Save hit detection debug image */
			void SaveImage(const std::map<int, PlayerHit> &hits,
			               const std::vector<Vector3> &bullets);
		};
	}
}
