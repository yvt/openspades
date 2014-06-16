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
#include "NetworkPlayer.h"
#include "NetworkClient.h"
#include <Game/World.h>
#include <Game/Player.h>
#include "Host.h"
#include <Server/Shared.h>

namespace spades { namespace ngclient {

	NetworkPlayer::NetworkPlayer(NetworkClient& net,
								 game::Player& p):
	net(net),
	player(p) {
		AddPlayerEntityListeners();
	}
	
	NetworkPlayer::~NetworkPlayer() {
		auto *e = GetEntity();
		if (e) {
			e->RemoveListener(this);
		}
	}
	
	void NetworkPlayer::Update() {
		auto *e = GetEntity();
		if (!e) return;
		
		protocol::ClientSideEntityUpdatePacket p;
		protocol::EntityUpdateItem item;
		item.entityId = !e->GetId();
		item.trajectory = e->GetTrajectory();
		if (shouldUpdateInput) {
			item.playerInput = e->GetPlayerInput();
		}
		
	}
	
	game::PlayerEntity *NetworkPlayer::GetEntity() {
		return dynamic_cast<game::PlayerEntity *>
		(net.world->FindEntity(*player.GetId()));
	}
	
	void NetworkPlayer::AddPlayerEntityListeners() {
		auto *ent = GetEntity();
		if (!ent) return;
		ent->AddListener(this);
	}
	
	void NetworkPlayer::PlayerInputUpdated(game::PlayerEntity &) {
		shouldUpdateInput = true;
	}
	
} }