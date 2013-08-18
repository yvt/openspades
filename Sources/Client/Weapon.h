//
//  Weapon.h
//  OpenSpades
//
//  Created by yvt on 7/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "Player.h"
#include "PhysicsConstants.h"

namespace spades {
	namespace client {
		class World;
		class Player;
		
		
		
		
		
		class Weapon {
			World *world;
			Player *owner;
			float time;
			bool shooting;
			bool reloading;
			float nextShotTime;
			float reloadEndTime;
			
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
			
			static Weapon *CreateWeapon(WeaponType index,
										Player *);
		
			void Restock();
			void Reset();
			void SetShooting(bool);
			
			/** @return true when fired. */
			bool FrameNext(float);
			
			void Reload();
			
			bool IsShooting() const {return shooting;}
			bool IsReloading() const { return reloading; }
			int GetAmmo() { return ammo; }
			int GetStock() { return stock;}
			
			bool IsReadyToShoot() ;
		};
	}
}
