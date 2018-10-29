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

#include "Weapon.h"
#include "GameProperties.h"
#include "IWorldListener.h"
#include "World.h"
#include <Core/Debug.h>
#include <Core/Exception.h>

namespace spades {
	namespace client {
		Weapon::Weapon(World *w, Player *p)
		    : world(w),
		      owner(p),
		      time(0),
		      shooting(false),
			  shootingPreviously(false),
		      reloading(false),
		      nextShotTime(0.f),
		      reloadStartTime(-101.f),
		      reloadEndTime(-100.f),
		      lastDryFire(false) {
			SPADES_MARK_FUNCTION();
		}

		Weapon::~Weapon() { SPADES_MARK_FUNCTION(); }

		void Weapon::Restock() { stock = GetMaxStock(); }

		void Weapon::Reset() {
			SPADES_MARK_FUNCTION();
			shooting = false;
			reloading = false;
			ammo = GetClipSize();
			stock = GetMaxStock();
			time = 0.f;
			nextShotTime = 0.f;
		}

		void Weapon::SetShooting(bool b) { shooting = b; }

		bool Weapon::IsReadyToShoot() {
			return (ammo > 0 || !owner->IsLocalPlayer()) && time >= nextShotTime &&
			       (!reloading || IsReloadSlow());
		}

		float Weapon::GetReloadProgress() {
			return (time - reloadStartTime) / (reloadEndTime - reloadStartTime);
		}

		float Weapon::TimeToNextFire() { return nextShotTime - time; }

		bool Weapon::FrameNext(float dt) {
			SPADES_MARK_FUNCTION();

			bool ownerIsLocalPlayer = owner->IsLocalPlayer();

			bool fired = false;
			bool dryFire = false;
			if (shooting && (!reloading || IsReloadSlow())) {
				// abort slow reload
				reloading = false;

				if (!shootingPreviously) {
					nextShotTime = std::max(nextShotTime, time);
				}

				// Automatic operation of weapon.
				if (time >= nextShotTime && (ammo > 0 || !ownerIsLocalPlayer)) {
					fired = true;

					// Consume an ammo.
					if (ammo > 0) {
						ammo--;
					}

					if (world->GetListener()) {
						world->GetListener()->PlayerFiredWeapon(owner);
					}
					nextShotTime += GetDelay();
				} else if (time >= nextShotTime) {
					dryFire = true;
				}
				shootingPreviously = true;
			} else {
				shootingPreviously = false;
			}
			if (reloading) {
				if (time >= reloadEndTime) {
					// A reload was completed (non-slow-loading weapon), or a single shell was
					// loaded (slow-loading weapon).
					//
					// For local players, the number of bullets/shells loaded onto the magazine is
					// sent by the server. However, we still calculate it by ourselves for slow
					// -loading weapons so the reloading animation and the number displayed on the
					// screen is synchronized. (This is especially important on a slow connection.)
					//
					// For remote players, the server does not send any information regarding the
					// number of bullets/shells loaded or held as stock. This is problematic for
					// slow-loading weapons because we can't tell how many times the reloading
					// animation has to be repeated. For now, we just assume a remote player has
					// an infinite supply of ammo, but a limited number of bullets in a clip.
					reloading = false;
					if (IsReloadSlow()) {
						if (ammo < GetClipSize() && (stock > 0 || !ownerIsLocalPlayer)) {
							ammo++;
							stock--;
						}
						slowReloadLeftCount--;
						if (slowReloadLeftCount > 0)
							Reload(false);
						else if (world->GetListener())
							world->GetListener()->PlayerReloadedWeapon(owner);
					} else {
						if (!ownerIsLocalPlayer) {
							ammo = GetClipSize();
						}
						if (world->GetListener())
							world->GetListener()->PlayerReloadedWeapon(owner);
					}
				}
			}
			time += dt;

			if (dryFire && !lastDryFire) {
				if (world->GetListener())
					world->GetListener()->PlayerDryFiredWeapon(owner);
			}
			lastDryFire = dryFire;
			return fired;
		}

		void Weapon::ReloadDone(int ammo, int stock) {
			SPADES_MARK_FUNCTION_DEBUG();
			this->ammo = ammo;
			this->stock = stock;
		}

		void Weapon::AbortReload() {
			SPADES_MARK_FUNCTION_DEBUG();
			reloading = false;
		}

		void Weapon::Reload(bool initial) {
			SPADES_MARK_FUNCTION();

			bool ownerIsLocalPlayer = owner->IsLocalPlayer();

			if (reloading)
				return;

			// Is the clip already full?
			if (ammo >= GetClipSize())
				return;
			
			if (ownerIsLocalPlayer) {
				if (stock == 0)
					return;
				if (IsReloadSlow() && ammo > 0 && shooting)
					return;
			}

			if (initial)
				slowReloadLeftCount = stock - std::max(0, stock - GetClipSize() + ammo);
			reloading = true;
			shooting = false;
			reloadStartTime = time;
			reloadEndTime = time + GetReloadTime();

			if (world->GetListener())
				world->GetListener()->PlayerReloadingWeapon(owner);
		}

		void Weapon::ForceReloadDone() {
			int newStock;
			newStock = std::max(0, stock - GetClipSize() + ammo);
			ammo += stock - newStock;
			stock = newStock;
		}

		class RifleWeapon3 : public Weapon {
		public:
			RifleWeapon3(World *w, Player *p) : Weapon(w, p) {}
			std::string GetName() override { return "Rifle"; }
			float GetDelay() override { return 0.5f; }
			int GetClipSize() override { return 10; }
			int GetMaxStock() override { return 50; }
			float GetReloadTime() override { return 2.5f; }
			bool IsReloadSlow() override { return false; }
			WeaponType GetWeaponType() override { return RIFLE_WEAPON; }
			int GetDamage(HitType type, float distance) override {
				switch (type) {
					case HitTypeTorso: return 49;
					case HitTypeHead: return 100;
					case HitTypeArms: return 33;
					case HitTypeLegs: return 33;
					case HitTypeBlock: return 50;
					default: SPAssert(false); return 0;
				}
			}
			Vector3 GetRecoil() override {
				return MakeVector3(0.025f, 0.05f, 0.f); // measured
			}
			float GetSpread() override { return 0.012f; } // measured (standing, crouched)
			int GetPelletSize() override { return 1; }
		};

		class SMGWeapon3 : public Weapon {
		public:
			SMGWeapon3(World *w, Player *p) : Weapon(w, p) {}
			std::string GetName() override { return "SMG"; }
			float GetDelay() override { return 0.11f; }
			int GetClipSize() override { return 30; }
			int GetMaxStock() override { return 120; }
			float GetReloadTime() override { return 2.5f; }
			bool IsReloadSlow() override { return false; }
			WeaponType GetWeaponType() override { return SMG_WEAPON; }
			int GetDamage(HitType type, float distance) override {
				switch (type) {
					case HitTypeTorso: return 29;
					case HitTypeHead: return 75;
					case HitTypeArms: return 18;
					case HitTypeLegs: return 18;
					case HitTypeBlock: return 35;
					default: SPAssert(false); return 0;
				}
			}
			Vector3 GetRecoil() override {
				return MakeVector3(0.01f, 0.0125f, 0.f); // measured
			}
			float GetSpread() override { return 0.025f; } // measured (standing, crouched)
			int GetPelletSize() override { return 1; }
		};

		class ShotgunWeapon3 : public Weapon {
		public:
			ShotgunWeapon3(World *w, Player *p) : Weapon(w, p) {}
			std::string GetName() override { return "Shotgun"; }
			float GetDelay() override { return 1.f; }
			int GetClipSize() override { return 6; }
			int GetMaxStock() override { return 48; }
			float GetReloadTime() override { return 0.5f; }
			bool IsReloadSlow() override { return true; }
			WeaponType GetWeaponType() override { return SHOTGUN_WEAPON; }
			int GetDamage(HitType type, float distance) override {
				switch (type) {
					case HitTypeTorso: return 27;
					case HitTypeHead: return 37;
					case HitTypeArms: return 16;
					case HitTypeLegs: return 16;
					case HitTypeBlock:
						// Actually, you cast a hit per pallet. This value is a guess, by the way.
						// --GM
						return 34;
					default: SPAssert(false); return 0;
				}
			}
			Vector3 GetRecoil() override {
				return MakeVector3(0.05f, 0.1f, 0.f); // measured
			}
			float GetSpread() override { return 0.024f; }
			int GetPelletSize() override { return 8; }
		};

		class RifleWeapon4 : public Weapon {
		public:
			RifleWeapon4(World *w, Player *p) : Weapon(w, p) {}
			std::string GetName() override { return "Rifle"; }
			float GetDelay() override { return 0.6f; }
			int GetClipSize() override { return 8; }
			int GetMaxStock() override { return 48; }
			float GetReloadTime() override { return 2.5f; }
			bool IsReloadSlow() override { return false; }
			WeaponType GetWeaponType() override { return RIFLE_WEAPON; }
			int GetDamage(HitType type, float distance) override {
				switch (type) {
					// These are the 0.75 damage values.
					// To be honest, we don't need this information, as the server decides the
					// damage.
					// EXCEPT for blocks, that is.
					// --GM
					case HitTypeTorso: return 49;
					case HitTypeHead: return 100;
					case HitTypeArms: return 33;
					case HitTypeLegs: return 33;
					case HitTypeBlock: return 50;
					default: SPAssert(false); return 0;
				}
			}
			Vector3 GetRecoil() override {
				// FIXME: needs to measured
				return MakeVector3(0.0001f, 0.075f, 0.f);
			}
			float GetSpread() override { return 0.004f; }
			int GetPelletSize() override { return 1; }
		};

		class SMGWeapon4 : public Weapon {
		public:
			SMGWeapon4(World *w, Player *p) : Weapon(w, p) {}
			std::string GetName() override { return "SMG"; }
			float GetDelay() override { return 0.1f; }
			int GetClipSize() override { return 30; }
			int GetMaxStock() override { return 150; }
			float GetReloadTime() override { return 2.5f; }
			bool IsReloadSlow() override { return false; }
			WeaponType GetWeaponType() override { return SMG_WEAPON; }
			int GetDamage(HitType type, float distance) override {
				switch (type) {
					case HitTypeTorso: return 29;
					case HitTypeHead: return 75;
					case HitTypeArms: return 18;
					case HitTypeLegs: return 18;
					case HitTypeBlock: return 34;
					default: SPAssert(false); return 0;
				}
			}
			Vector3 GetRecoil() override {
				// FIXME: needs to measured
				return MakeVector3(0.00005f, 0.0125f, 0.f);
			}
			float GetSpread() override { return 0.012f; }
			int GetPelletSize() override { return 1; }
		};

		class ShotgunWeapon4 : public Weapon {
		public:
			ShotgunWeapon4(World *w, Player *p) : Weapon(w, p) {}
			std::string GetName() override { return "Shotgun"; }
			float GetDelay() override { return 0.8f; }
			int GetClipSize() override { return 8; }
			int GetMaxStock() override { return 48; }
			float GetReloadTime() override { return 0.4f; }
			bool IsReloadSlow() override { return true; }
			WeaponType GetWeaponType() override { return SHOTGUN_WEAPON; }
			int GetDamage(HitType type, float distance) override {
				switch (type) {
					case HitTypeTorso: return 27;
					case HitTypeHead: return 37;
					case HitTypeArms: return 16;
					case HitTypeLegs: return 16;
					case HitTypeBlock: return 34;
					default: SPAssert(false); return 0;
				}
			}
			Vector3 GetRecoil() override {
				// FIXME: needs to measured
				return MakeVector3(0.0002f, 0.075f, 0.f);
			}
			float GetSpread() override { return 0.036f; }
			int GetPelletSize() override { return 8; }
		};

		Weapon *Weapon::CreateWeapon(WeaponType type, Player *p, const GameProperties &gp) {
			SPADES_MARK_FUNCTION();

			switch (gp.protocolVersion) {
				case ProtocolVersion::v075:
					switch (type) {
						case RIFLE_WEAPON: return new RifleWeapon3(p->GetWorld(), p);
						case SMG_WEAPON: return new SMGWeapon3(p->GetWorld(), p);
						case SHOTGUN_WEAPON: return new ShotgunWeapon3(p->GetWorld(), p);
						default: SPInvalidEnum("type", type);
					}
				case ProtocolVersion::v076:
					switch (type) {
						case RIFLE_WEAPON: return new RifleWeapon4(p->GetWorld(), p);
						case SMG_WEAPON: return new SMGWeapon4(p->GetWorld(), p);
						case SHOTGUN_WEAPON: return new ShotgunWeapon4(p->GetWorld(), p);
						default: SPInvalidEnum("type", type);
					}
                default:
                    SPInvalidEnum("protocolVersion", gp.protocolVersion);
			}
		}
	}
}
