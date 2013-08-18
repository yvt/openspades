//
//  IWorldListener.h
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Core/Math.h"
#include "PhysicsConstants.h"

namespace spades {
	namespace client {
		class Player;
		class Grenade;
		class IWorldListener{
		public:
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
			virtual void PlayerHitPlayerWithSpade(Player *) = 0;
			virtual void PlayerHitBlockWithSpade(Player *,
												 Vector3 hitPos,
												 IntVector3 blockPos,
												 IntVector3 normal) = 0;
			virtual void PlayerKilledPlayer(Player *killer,
											Player *victim,
											KillType) = 0;
			virtual void PlayerRestocked(Player *) = 0;
			
			virtual void BulletHitPlayer(Player *hurtPlayer, HitType,
										 Vector3 hitPos,
										 Player *by) = 0;
			virtual void BulletHitBlock(Vector3 hitPos,
										IntVector3 blockPos,
										IntVector3 normal) = 0;
			
			virtual void GrenadeExploded(Grenade *) = 0;
			virtual void GrenadeBounced(Grenade *) = 0;
			virtual void GrenadeDroppedIntoWater(Grenade *) = 0;
			
			virtual void BlocksFell(std::vector<IntVector3>) = 0;
			
			virtual void LocalPlayerPulledGrenadePin() = 0;
			virtual void LocalPlayerBlockAction(IntVector3, BlockActionType type) = 0;
			virtual void LocalPlayerCreatedLineBlock(IntVector3, IntVector3) = 0;
			virtual void LocalPlayerHurt(HurtType type, bool sourceGiven,
										 Vector3 source) = 0;
		};
	}
}
