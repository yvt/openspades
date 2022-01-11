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

#include "FallingBlock.h"
#include "Client.h"
#include "GameMap.h"
#include "IModel.h"
#include "IRenderer.h"
#include "ParticleSpriteEntity.h"
#include "SmokeSpriteEntity.h"
#include "World.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <limits.h>

namespace spades {
	namespace client {
		FallingBlock::FallingBlock(Client *client, std::vector<IntVector3> blocks)
		    : client(client) {
			// stubbed
		}

		FallingBlock::~FallingBlock() {
			// stubbed
		}

		bool FallingBlock::Update(float dt) {
			// stubbed
		}

		void FallingBlock::Render3D() {
			// stubbed
		}
	} // namespace client
} // namespace spades
