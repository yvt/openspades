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

#include "PhysicsConstants.h"
#include "Player.h"

namespace spades {
	namespace client {
		class World;
		class Player;
		struct GameProperties;

		class Weapon {
			World *world;
			Player *owner;
			float time;
			bool shooting;
			bool shootingPreviously;
			bool reloading;
			float nextShotTime;
			float reloadStartTime;
			float reloadEndTime;

			int slowReloadLeftCount;

			bool lastDryFire;

			int ammo;
			int stock;

		public:
			Weapon(World *, Player *);
			virtual ~Weapon();
			virtual std::string GetName() = 0;
			virtual float GetDelay() = 0;
			virtual int GetClipSize() = 0;
			virtual int GetMaxStock() = 0;
			virtual float GetReloadTime() = 0;
			virtual bool IsReloadSlow() = 0;
			virtual int GetDamage(HitType, float distance) = 0;
			virtual WeaponType GetWeaponType() = 0;

			virtual Vector3 GetRecoil() = 0;
			virtual float GetSpread() = 0;

			virtual int GetPelletSize() = 0;

			static Weapon *CreateWeapon(WeaponType index, Player *, const GameProperties &);

			void Restock();
			void Reset();
			void SetShooting(bool);

			/** @return true when fired. */
			bool FrameNext(float);

			/**
			 * @param initial `true` if the reload was initiated by the user.
			 */
			void Reload(bool initial = true);
			void AbortReload();

			bool IsShooting() const { return shooting; }
			bool IsReloading() const { return reloading; }
			int GetAmmo() { return ammo; }
			int GetStock() { return stock; }

			// for local player
			void ReloadDone(int ammo, int stock);

			void ForceReloadDone();

			float GetReloadProgress();
			float TimeToNextFire();

			bool IsReadyToShoot();
		};
	}
}
