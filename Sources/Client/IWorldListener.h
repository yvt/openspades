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
#include "PhysicsConstants.h"

namespace spades {
	namespace client {
		class Player;
		class Grenade;

		enum class BuildFailureReason { InsufficientBlocks, InvalidPosition };

		class IWorldListener {
		public:
			virtual void PlayerObjectSet(int playerId) = 0;
			virtual void PlayerMadeFootstep(Player *) = 0;
			virtual void PlayerJumped(Player *) = 0;
			virtual void PlayerLanded(Player *, bool hurt) = 0;
			virtual void PlayerFiredWeapon(Player *) = 0;
			virtual void PlayerDryFiredWeapon(Player *) = 0;
			virtual void PlayerReloadingWeapon(Player *) = 0;
			virtual void PlayerReloadedWeapon(Player *) = 0;
			virtual void PlayerChangedTool(Player *) = 0;
			virtual void PlayerThrownGrenade(Player *, Grenade *) = 0;
			virtual void PlayerMissedSpade(Player *) = 0;
			virtual void PlayerHitBlockWithSpade(Player *, Vector3 hitPos, IntVector3 blockPos,
			                                     IntVector3 normal) = 0;
			virtual void PlayerKilledPlayer(Player *killer, Player *victim, KillType) = 0;
			virtual void PlayerRestocked(Player *) = 0;

			virtual void BulletHitPlayer(Player *hurtPlayer, HitType, Vector3 hitPos,
			                             Player *by) = 0;
			virtual void BulletHitBlock(Vector3 hitPos, IntVector3 blockPos, IntVector3 normal) = 0;
			virtual void AddBulletTracer(Player *player, Vector3 muzzlePos, Vector3 hitPos) = 0;

			virtual void GrenadeExploded(Grenade *) = 0;
			virtual void GrenadeBounced(Grenade *) = 0;
			virtual void GrenadeDroppedIntoWater(Grenade *) = 0;

			virtual void BlocksFell(std::vector<IntVector3>) = 0;

			virtual void LocalPlayerPulledGrenadePin() = 0;
			virtual void LocalPlayerBlockAction(IntVector3, BlockActionType type) = 0;
			virtual void LocalPlayerCreatedLineBlock(IntVector3, IntVector3) = 0;
			virtual void LocalPlayerHurt(HurtType type, bool sourceGiven, Vector3 source) = 0;
			virtual void LocalPlayerBuildError(BuildFailureReason reason) = 0;
		};
	}
}
