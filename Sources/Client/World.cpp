//
//  World.cpp
//  OpenSpades
//
//  Created by yvt on 7/14/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "World.h"
#include "GameMap.h"
#include "GameMapWrapper.h"
#include "../Core/IStream.h"
#include "../Core/FileManager.h"
#include "../Core/Debug.h"
#include "Player.h"
#include "Weapon.h"
#include "Grenade.h"
#include "../Core/Debug.h"
#include "IGameMode.h"
#include <math.h>
#include <stdlib.h>
#include "IWorldListener.h"

namespace spades {
	namespace client {
		
		World::World(){
			SPADES_MARK_FUNCTION();
			
			listener = NULL;
			
			map = NULL;
			mapWrapper = NULL;
			/*
			IStream *stream = FileManager::OpenForReading("Maps/burbs.vxl");
			map = GameMap::Load(stream);
			delete stream;
			
			mapWrapper = new GameMapWrapper(map);
			mapWrapper->Rebuild();*/
			
			localPlayerIndex = -1;
			for(int i = 0; i < 32; i++){
				players.push_back(NULL);
				playerPersistents.push_back(PlayerPersistent());
			}
			/*
			Player *pl = new Player(this, 0, WeaponType::SMG_WEAPON,
									0, MakeVector3(256,256,5),
									"Deuce", IntVector3::Make(0,255,0));
		
			pl->SetOrientation(MakeVector3(0, 1, 0));
			
			
			players[0] = pl;*/
			
			localPlayerIndex = 0;
			
			time = 0.f;
			mode = NULL;
			
		}
		World::~World() {
			SPADES_MARK_FUNCTION();
			
			for(std::list<Grenade *>::iterator it = grenades.begin();
				it != grenades.end(); it++)
				delete *it;
			for(size_t i = 0; i < players.size(); i++)
				if(players[i])
					delete players[i];
			if(mode){
				delete mode;
			}
			if(map){
				delete mapWrapper;
				delete map;
			}
		}
		
		void World::Advance(float dt) {
			SPADES_MARK_FUNCTION();
			
			for(size_t i = 0; i < players.size(); i++)
				if(players[i])
					players[i]->Update(dt);
			
			std::vector<std::list<Grenade *>::iterator> removedGrenades;
			for(std::list<Grenade *>::iterator it = grenades.begin();
				it != grenades.end(); it++){
				Grenade *g = *it;
				if(g->Update(dt)){
					removedGrenades.push_back(it);
				}
			}
			for(size_t i = 0; i < removedGrenades.size(); i++)
				grenades.erase(removedGrenades[i]);
			
			time += dt;
		}
		
		void World::SetMap(spades::client::GameMap *newMap){
			if(map){
				delete map;
				delete mapWrapper;
			}
			
			map = newMap;
			if(map){
				mapWrapper = new GameMapWrapper(map);
				mapWrapper->Rebuild();
			}
		}
		
		void World::AddGrenade(spades::client::Grenade *g){
			SPADES_MARK_FUNCTION_DEBUG();
			
			grenades.push_back(g);
		}
		
		std::vector<Grenade *> World::GetAllGrenades() {
			SPADES_MARK_FUNCTION_DEBUG();
			
			std::vector<Grenade *> g;
			for(std::list<Grenade *>::iterator it = grenades.begin();
				it != grenades.end(); it++){
				g.push_back(*it);
			}
			return g;
		}
		
		void World::SetPlayer(int i,
							  spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			SPAssert(i >= 0);
			SPAssert( i < (int)players.size());
			if(players[i] == p)
				return;
			if(players[i])
				delete players[i];
			players[i] = p;
		}
		
		void World::SetMode(spades::client::IGameMode *m) {
			if(mode == m)
				return;
			if(mode)
				delete mode;
			mode = m;
		}
		
		void World::CreateBlock(spades::IntVector3 pos,
								spades::IntVector3 color) {
			if(map->IsSolid(pos.x, pos.y, pos.z))
				return;
			mapWrapper->AddBlock(pos.x, pos.y, pos.z,
								 color.x |
								 (color.y << 8) |
								 (color.z << 16) |
								 (100UL << 24));
		}
		void World::DestroyBlock(std::vector<spades::IntVector3> pos){
			std::vector<CellPos> cells;
			for(size_t i = 0; i < pos.size(); i++){
				const IntVector3& p = pos[i];
				if(p.z >= 62 || p.z < 0 || p.x < 0 || p.y < 0 ||
				   p.x >= map->Width() || p.y >= map->Height())
					continue;
				if(!map->IsSolid(p.x, p.y, p.z))
					continue;
				cells.push_back(CellPos(p.x,p.y,p.z));
			}
			
			cells = mapWrapper->RemoveBlocks(cells);
			
			for(size_t i =0 ; i < cells.size(); i++){
				CellPos& p = cells[i];
				map->Set(p.x, p.y, p.z, false, 0);
			}
			
			std::vector<IntVector3> cells2;
			for(size_t i =0 ; i < cells.size(); i++){
				cells2.push_back(IntVector3::Make(cells[i].x,
												  cells[i].y,
												  cells[i].z));
			}
			
			if(listener)
				listener->BlocksFell(cells2);
		}
		
		World::PlayerPersistent& World::GetPlayerPersistent(int index) {
			SPAssert(index >= 0);
			SPAssert(index < players.size());
			return playerPersistents[index];
		}
		
		std::vector<IntVector3> World::CubeLine(spades::IntVector3 v1,
												spades::IntVector3 v2,
												int maxLength) {
			SPADES_MARK_FUNCTION_DEBUG();
			
			IntVector3 c = v1;
			IntVector3 d = v2 - v1;
			long ixi, iyi, izi, dx, dy, dz, dxi, dyi, dzi;
			std::vector<IntVector3> ret;
			
			int VSID = map->Width();
			SPAssert(VSID == map->Height());
			
			int MAXZDIM = map->Depth();
			
			if (d.x < 0) ixi = -1;
			else ixi = 1;
			if (d.y < 0) iyi = -1;
			else iyi = 1;
			if (d.z < 0) izi = -1;
			else izi = 1;
			
			if ((abs(d.x) >= abs(d.y)) && (abs(d.x) >= abs(d.z)))
			{
				dxi = 1024; dx = 512;
				dyi = (long)(!d.y ? 0x3fffffff/VSID : abs(d.x*1024/d.y));
				dy = dyi/2;
				dzi = (long)(!d.z ? 0x3fffffff/VSID : abs(d.x*1024/d.z));
				dz = dzi/2;
			}
			else if (abs(d.y) >= abs(d.z))
			{
				dyi = 1024; dy = 512;
				dxi = (long)(!d.x ? 0x3fffffff/VSID : abs(d.y*1024/d.x));
				dx = dxi/2;
				dzi = (long)(!d.z ? 0x3fffffff/VSID : abs(d.y*1024/d.z));
				dz = dzi/2;
			}
			else
			{
				dzi = 1024; dz = 512;
				dxi = (long)(!d.x ? 0x3fffffff/VSID : abs(d.z*1024/d.x));
				dx = dxi/2;
				dyi = (long)(!d.y ? 0x3fffffff/VSID : abs(d.z*1024/d.y));
				dy = dyi/2;
			}
			if (ixi >= 0) dx = dxi-dx;
			if (iyi >= 0) dy = dyi-dy;
			if (izi >= 0) dz = dzi-dz;
			
			while (1)
			{
				ret.push_back(c);
				
				if(ret.size() == (size_t)maxLength)
					break;
				
				if(c.x == v2.x &&
				   c.y == v2.y &&
				   c.z == v2.z)
					break;
				
				if ((dz <= dx) && (dz <= dy))
				{
					c.z += izi;
					if (c.z < 0 || c.z >= MAXZDIM)
						break;
					dz += dzi;
				}
				else
				{
					if (dx < dy)
					{
						c.x += ixi;
						if ((unsigned long)c.x >= VSID)
							break;
						dx += dxi;
					}
					else
					{
						c.y += iyi;
						if ((unsigned long)c.y >= VSID)
							break;
						dy += dyi;
					}
				}
			}
			
			return ret;
		}
		
		World::WeaponRayCastResult World::WeaponRayCast(spades::Vector3 startPos,
														spades::Vector3 dir,
														Player *exclude) {
			WeaponRayCastResult result;
			Player *hitPlayer = NULL;
			float hitPlayerDistance = 0.f;
			int hitFlag = 0;
			
			for(int i = 0; i < (int)players.size(); i++){
				Player *p = players[i];
				if(p == NULL || p == exclude)
					continue;
				if(p->GetTeamId() >= 2 || !p->IsAlive())
					continue;
				if(!p->RayCastApprox(startPos, dir))
					continue;
				
				Player::HitBoxes hb = p->GetHitBoxes();
				Vector3 hitPos;
				
				if(hb.head.RayCast(startPos, dir, &hitPos)) {
					float dist = (hitPos - startPos).GetLength();
					if(hitPlayer == NULL ||
					   dist < hitPlayerDistance){
						if(hitPlayer != p){
							hitPlayer = p;
							hitFlag = 0;
						}
						hitPlayerDistance = dist;
						hitFlag |= 1; // head
					}
				}
				if(hb.torso.RayCast(startPos, dir, &hitPos)) {
					float dist = (hitPos - startPos).GetLength();
					if(hitPlayer == NULL ||
					   dist < hitPlayerDistance){
						if(hitPlayer != p){
							hitPlayer = p;
							hitFlag = 0;
						}
						hitPlayerDistance = dist;
						hitFlag |= 2; // torso
					}
				}
				for(int j = 0; j < 3 ;j++){
					if(hb.limbs[j].RayCast(startPos, dir, &hitPos)) {
						float dist = (hitPos - startPos).GetLength();
						if(hitPlayer == NULL ||
						   dist < hitPlayerDistance){
							if(hitPlayer != p){
								hitPlayer = p;
								hitFlag = 0;
							}
							hitPlayerDistance = dist;
							if(j == 2)
								hitFlag |= 8; // arms
							else
								hitFlag |= 4; // leg
						}
					}
				}
			}
			
			// map raycast
			GameMap::RayCastResult res2;
			res2 = map->CastRay2(startPos, dir, 256);
			
			if(res2.hit && (hitPlayer == NULL ||
							(res2.hitPos - startPos).GetLength() <
							hitPlayerDistance)){
				result.hit = true;
				result.startSolid = res2.startSolid;
				result.player = NULL;
				result.blockPos = res2.hitBlock;
				result.hitPos = res2.hitPos;
			}else if(hitPlayer) {
				result.hit = true;
				result.startSolid = false; // FIXME: startSolid for player
				result.player = hitPlayer;
				result.hitPos = startPos + dir * hitPlayerDistance;
			}else{
				result.hit = false;
			}
			
			return result;
		}
		
	}
}
