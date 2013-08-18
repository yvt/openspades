//
//  Player.h
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
		class World;
		class Weapon;
		
		struct PlayerInput {
			bool moveForward: 1;
			bool moveBackward: 1;
			bool moveLeft: 1;
			bool moveRight: 1;
			bool jump: 1;
			bool crouch: 1;
			bool sneak: 1;
			bool sprint: 1;
			
			PlayerInput():
			moveForward(false),
			moveBackward(false),
			moveLeft(false),
			moveRight(false),
			jump(false),
			crouch(false),
			sneak(false),
			sprint(false){
				
			}
		};
		
		struct WeaponInput {
			bool primary: 1;
			bool secondary: 1;
			WeaponInput():
			primary(false),
			secondary(false){}
		};
		
		
		
		class Player{
		public:
			enum ToolType {
				ToolSpade = 0,
				ToolBlock,
				ToolWeapon,
				ToolGrenade
			};
			struct HitBoxes{
				OBB3 torso;
				OBB3 limbs[3];
				OBB3 head;
			};
		private:
			World *world;
			
			Vector3 position;
			Vector3 velocity;
			Vector3 orientation;
			Vector3 eye;
			PlayerInput input;
			WeaponInput weapInput;
			bool airborne;
			bool wade;
			ToolType tool;
			
			Weapon *weapon;
			int playerId;
			int teamId;
			IntVector3 color; // obsolete
			
			int health;
			int grenades;
			int blockStocks;
			IntVector3 blockColor;
			
			// for making footsteps
			float moveDistance;
			int moveSteps;
			
			bool lastJump;
			
			float lastClimbTime;
			float lastJumpTime;
			
			// tools
			float nextSpadeTime;
			float nextDigTime;
			float nextGrenadeTime;
			float nextBlockTime;
			bool holdingGrenade;
			float grenadeTime;
			bool blockCursorActive;
			bool blockCursorDragging;
			IntVector3 blockCursorPos;
			IntVector3 blockCursorDragPos;
			
			float respawnTime;
			
			void RepositionPlayer(const Vector3&);
			void MovePlayer(float fsynctics);
			void BoxClipMove(float fsynctics);
			
			void UseSpade();
			void DigWithSpade();
			void FireWeapon();
			void ThrowGrenade();
		public:
			
			Player(World *, int playerId,
				   WeaponType weapon, int teamId,
				   Vector3 position,
				   IntVector3 color);
			
			~Player();
			
			int GetId() {return playerId; }
			Weapon *GetWeapon() { return weapon; }
			int GetTeamId() { return teamId;}
			std::string GetName();
			IntVector3 GetColor();
			IntVector3 GetBlockColor() { return blockColor; }
			ToolType GetTool() { return tool; }
			
			PlayerInput GetInput() { return input; }
			WeaponInput GetWeaponInput() { return weapInput; }
			void SetInput(PlayerInput);
			void SetWeaponInput(WeaponInput);
			void SetTool(ToolType);
			void SetHeldBlockColor(IntVector3);
			bool IsBlockCursorActive();
			bool IsBlockCursorDragging();
			IntVector3 GetBlockCursorPos(){return blockCursorPos;}
			IntVector3 GetBlockCursorDragPos(){return blockCursorDragPos;}
			bool IsReadyToUseTool();
			
			// ammo counts
			int GetNumBlocks() { return blockStocks;}
			int GetNumGrenades() { return grenades; }
			void Reload();
			void Restock();
			void GotBlock();
			
			bool IsToolWeapon() {
				return tool == ToolWeapon;
			}
			
			void SetPosition(const Vector3&);
			void SetOrientation(const Vector3&);
			void Turn(float longitude, float latitude);
			
			void SetHP(int hp, HurtType, Vector3);
			
			void SetWeaponType(WeaponType weap);
			void SetTeam(int);
			void UsedBlocks(int c) { blockStocks = std::max(blockStocks - c, 0); }
			
			/** makes player's health 0. */
			void KilledBy(KillType, Player *killer, int respawnTime);
			
			bool IsAlive();
			/** @return world time to respawn */
			float GetRespawnTime() { return respawnTime;}
			/** Returns player's health (local player only) */
			int GetHealth() { return health; }
			
			Vector3 GetPosition() { return position; }
			Vector3 GetFront();
			Vector3 GetFront2D();
			Vector3 GetRight();
			Vector3 GetLeft();
			Vector3 GetUp();
			Vector3 GetEye() { return eye; }
			Vector3 GetOrigin(); // actually not origin at all!
			Vector3 GetVelocty() { return velocity; }
			int GetMoveSteps() {return moveSteps;}
			
			World *GetWorld() { return world; }
			
			bool GetWade();
			bool IsOnGroundOrWade();
			
			void Update(float dt);
			bool TryUncrouch(bool move); // ??
			
			float GetToolPrimaryDelay();
			float GetToolSecondaryDelay();
			
			float GetSpadeAnimationProgress();
			float GetDigAnimationProgress();
			
			float GetWalkAnimationProgress();
			
			bool IsCookingGrenade();
			float GetGrenadeCookTime();
			
			// hit tests
			HitBoxes GetHitBoxes();
			
			/** Does approximated ray casting.
			 * @param dir normalized direction vector.
			 * @return true if ray may hit the player. */
			bool RayCastApprox(Vector3 start, Vector3 dir);
		};
	}
}
