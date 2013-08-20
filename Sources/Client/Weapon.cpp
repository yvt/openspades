//
//  Weapon.cpp
//  OpenSpades
//
//  Created by yvt on 7/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "Weapon.h"
#include "World.h"
#include "IWorldListener.h"
#include "../Core/Exception.h"
#include "../Core/Debug.h"

namespace spades {
	namespace client {
		Weapon::Weapon(World *w, Player *p):
		world(w),
		owner(p),
		time(0),
		shooting(false),
		reloading(false),
		nextShotTime(0.f),
		reloadEndTime(-100.f),
		reloadStartTime(-101.f),
		lastDryFire(false)
		{
			SPADES_MARK_FUNCTION();
		}
		
		Weapon::~Weapon(){
			
			SPADES_MARK_FUNCTION();
		}
		
		void Weapon::Restock() {
			stock = GetMaxStock();
		}
		
		void Weapon::Reset() {
			SPADES_MARK_FUNCTION();
			shooting = false;
			reloading = false;
			ammo = GetClipSize();
			stock = GetMaxStock();
			time = 0.f;
			nextShotTime = 0.f;
		}
		
		void Weapon::SetShooting(bool b){
			shooting = b;
		}
		
		bool Weapon::IsReadyToShoot()  {
			return ammo > 0 && time >= nextShotTime &&
			(!reloading || IsReloadSlow());
		}
		
		float Weapon::GetReloadProgress() {
			return (time - reloadStartTime) / (reloadEndTime - reloadStartTime);
		}
		
		float Weapon::TimeToNextFire() {
			return nextShotTime - time;
		}
		
		bool Weapon::FrameNext(float dt){
			SPADES_MARK_FUNCTION();
			
			bool fired = false;
			bool dryFire = false;
			if(shooting && (!reloading || IsReloadSlow())){
				// abort slow reload
				reloading = false;
				if(time >= nextShotTime && ammo > 0) {
					fired = true;
					ammo--;
					if(world->GetListener())
						world->GetListener()->PlayerFiredWeapon(owner);
					nextShotTime = time + GetDelay();
				}else if(time >= nextShotTime){
					dryFire = true;
				}
			}
			if(reloading){
				if(time >= reloadEndTime) {
					// reload done
					reloading = false;
					if(IsReloadSlow()){
						ammo++;
						stock--;
						Reload();
					}else{
						int newStock;
						newStock = std::max(0, stock - GetClipSize() + ammo);
						ammo += stock - newStock;
						stock = newStock;
					}
					
					
					if(world->GetListener())
						world->GetListener()->PlayerReloadedWeapon(owner);
				}
			}
			time += dt;
			
			if(dryFire && !lastDryFire){
				if(world->GetListener())
					world->GetListener()->PlayerDryFiredWeapon(owner);
			}
			lastDryFire = dryFire;
			return fired;
		}
		
		void Weapon::Reload() {
			SPADES_MARK_FUNCTION();
			
			if(reloading)
				return;
			if(ammo == GetClipSize())
				return;
			if(stock == 0)
				return;
			reloading = true;
			shooting = false;
			reloadStartTime = time;
			reloadEndTime = time + GetReloadTime();
			
			if(world->GetListener())
				world->GetListener()->PlayerReloadingWeapon(owner);
		}
		
		class RifleWeapon: public Weapon {
		public:
			RifleWeapon(World*w,Player*p):Weapon(w,p){}
			virtual std::string GetName() { return "Rifle"; }
			virtual float GetDelay() { return 0.5f; }
			virtual int GetClipSize() { return 10; }
			virtual int GetMaxStock() { return 50; }
			virtual float GetReloadTime() { return 2.5f; }
			virtual bool IsReloadSlow() { return false; }
			virtual WeaponType GetWeaponType() { return RIFLE_WEAPON; }
			virtual int GetDamage(HitType type, float distance) {
				switch(type){
					case HitTypeTorso: return 49;
					case HitTypeHead: return 100;
					case HitTypeArms: return 33;
					case HitTypeLegs: return 33;
					case HitTypeBlock: return 50;
				}
			}
			virtual Vector3 GetRecoil () {
				return MakeVector3(0.0001f, -0.05f, 0.f);
			}
			virtual float GetSpread() { return 0.006f; }
			virtual int GetPelletSize() { return 1; }
		};
		
		class SMGWeapon: public Weapon {
		public:
			SMGWeapon(World*w,Player*p):Weapon(w,p){}
			virtual std::string GetName() { return "SMG"; }
			virtual float GetDelay() { return 0.1f; }
			virtual int GetClipSize() { return 30; }
			virtual int GetMaxStock() { return 120; }
			virtual float GetReloadTime() { return 2.5f; }
			virtual bool IsReloadSlow() { return false; }
			virtual WeaponType GetWeaponType() { return SMG_WEAPON; }
			virtual int GetDamage(HitType type, float distance) {
				switch(type){
					case HitTypeTorso: return 29;
					case HitTypeHead: return 75;
					case HitTypeArms: return 18;
					case HitTypeLegs: return 18;
					case HitTypeBlock: return 25;
				}
			}
			virtual Vector3 GetRecoil () {
				return MakeVector3(0.00005f, -0.0125f, 0.f);
			}
			virtual float GetSpread() { return 0.012f; }
			virtual int GetPelletSize() { return 1; }
		};
		
		class ShotgunWeapon: public Weapon {
		public:
			ShotgunWeapon(World*w,Player*p):Weapon(w,p){}
			virtual std::string GetName() { return "Shotgun"; }
			virtual float GetDelay() { return 1.f; }
			virtual int GetClipSize() { return 6; }
			virtual int GetMaxStock() { return 48; }
			virtual float GetReloadTime() { return 0.5f; }
			virtual bool IsReloadSlow() { return true; }
			virtual WeaponType GetWeaponType() { return SHOTGUN_WEAPON; }
			virtual int GetDamage(HitType type, float distance) {
				switch(type){
					case HitTypeTorso: return 27;
					case HitTypeHead: return 37;
					case HitTypeArms: return 16;
					case HitTypeLegs: return 16;
					case HitTypeBlock:
						if(distance < 15.f)
							return 100;
						else
							return (int)(15.f / distance * 100.f);
				}
			}
			virtual Vector3 GetRecoil () {
				return MakeVector3(0.0002f, -0.1f, 0.f);
			}
			virtual float GetSpread() { return 0.024f; }
			virtual int GetPelletSize() { return 8; }
		};
		
		Weapon *Weapon::CreateWeapon(WeaponType type, Player *p) {
			SPADES_MARK_FUNCTION();
			
			switch(type){
				case RIFLE_WEAPON:
					return new RifleWeapon(p->GetWorld(), p);
				case SMG_WEAPON:
					return new SMGWeapon(p->GetWorld(), p);
				case SHOTGUN_WEAPON:
					return new ShotgunWeapon(p->GetWorld(), p);
				default:
					SPInvalidEnum("type", type);
			}
		}
		
	}
}
