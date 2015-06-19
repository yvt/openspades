/*
 Copyright (c) 2013 OpenSpades Developers
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

#include "Client.h"
#include <cstdlib>
#include <Core/Strings.h>

#include "IAudioDevice.h"
#include "IAudioChunk.h"

#include "World.h"
#include "IGameMode.h"
#include "TCGameMode.h"
#include "CTFGameMode.h"
#include "GameMap.h"

#include "ChatWindow.h"
#include "ClientUI.h"
#include "CenterMessageView.h"

#include "NetClient.h"

namespace spades {
	namespace client {
		
#pragma mark - Server Packet Handlers
		
		void Client::LocalPlayerCreated(){
			followPos = world->GetLocalPlayer()->GetEye();
			weapInput = WeaponInput();
			playerInput = PlayerInput();
			keypadInput = KeypadInput();
			
			toolRaiseState = .0f;
		}
		
		void Client::JoinedGame() {
			// note: local player doesn't exist yet now
			
			// tune for spectate mode
			followingPlayerId = world->GetLocalPlayerIndex();
			followPos = MakeVector3(256, 256, 30);
			followVel = MakeVector3(0, 0, 0);
		}
		
		void Client::PlayerCreatedBlock(spades::client::Player *p){
			SPADES_MARK_FUNCTION();
			
			if(!IsMuted()) {
				Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Weapons/Block/Build.wav");
				audioDevice->Play(c, p->GetEye() + p->GetFront(),
								  AudioParam());
			}
		}
		
		void Client::TeamCapturedTerritory(int teamId,
										   int terId) {
			TCGameMode::Territory *ter = static_cast<TCGameMode *>(world->GetMode())->GetTerritory(terId);
			int old = ter->ownerTeamId;
			std::string msg;
			std::string teamName = chatWindow->TeamColorMessage(world->GetTeam(teamId).name,
																teamId);
			if(old < 2){
				std::string otherTeam = chatWindow->TeamColorMessage(world->GetTeam(old).name,
																	 old);
				msg = _Tr("Client", "{0} captured {1}'s territory", teamName, otherTeam);
			}else{
				msg = _Tr("Client", "{0} captured an neutral territory", teamName);
			}
			chatWindow->AddMessage(msg);
			
			teamName = world->GetTeam(teamId).name;
			if(old < 2){
				std::string otherTeam = world->GetTeam(old).name;
				msg = _Tr("Client", "{0} captured {1}'s Territory", teamName, otherTeam);
			}else{
				msg = _Tr("Client", "{0} captured an Neutral Territory", teamName);
			}
			NetLog("%s", msg.c_str());
			centerMessageView->AddMessage(msg);
			
			if(world->GetLocalPlayer() && !IsMuted()){
				if(teamId == world->GetLocalPlayer()->GetTeamId()){
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/TC/YourTeamCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}else{
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/TC/EnemyCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}
			}
			
		}
		
		void Client::PlayerCapturedIntel(spades::client::Player *p){
			std::string msg;
			{
				std::string holderName = chatWindow->TeamColorMessage(p->GetName(), p->GetTeamId());
				
				std::string otherTeamName = chatWindow->TeamColorMessage(world->GetTeam(1 - p->GetTeamId()).name,
																		 1 - p->GetTeamId());
				msg = _Tr("Client", "{0} captured {1}'s intel", holderName, otherTeamName);
				chatWindow->AddMessage(msg);
			}
			
			{
				std::string holderName = p->GetName();
				std::string otherTeamName = world->GetTeam(1 - p->GetTeamId()).name;
				msg = _Tr("Client", "{0} captured {1}'s Intel.", holderName, otherTeamName);
				NetLog("%s", msg.c_str());
				centerMessageView->AddMessage(msg);
			}
			
			if(world->GetLocalPlayer() && !IsMuted()){
				if(p->GetTeamId() == world->GetLocalPlayer()->GetTeamId()){
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/CTF/YourTeamCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}else{
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/CTF/EnemyCaptured.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}
			}
		}
		
		void Client::PlayerPickedIntel(spades::client::Player *p) {
			std::string msg;
			{
				std::string holderName = chatWindow->TeamColorMessage(p->GetName(), p->GetTeamId());
				std::string otherTeamName = chatWindow->TeamColorMessage(world->GetTeam(1 - p->GetTeamId()).name,
																		 1 - p->GetTeamId());
				msg = _Tr("Client", "{0} picked up {1}'s intel", holderName, otherTeamName);
				chatWindow->AddMessage(msg);
			}
			
			{
				std::string holderName = p->GetName();
				std::string otherTeamName = world->GetTeam(1 - p->GetTeamId()).name;
				msg = _Tr("Client", "{0} picked up {1}'s Intel.", holderName, otherTeamName);
				NetLog("%s", msg.c_str());
				centerMessageView->AddMessage(msg);
			}
			
			if(!IsMuted()) {
				Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/CTF/PickedUp.wav");
				audioDevice->PlayLocal(chunk, AudioParam());
			}
		}
		
		void Client::PlayerDropIntel(spades::client::Player *p) {
			std::string msg;
			{
				std::string holderName = chatWindow->TeamColorMessage(p->GetName(), p->GetTeamId());
				std::string otherTeamName = chatWindow->TeamColorMessage(world->GetTeam(1 - p->GetTeamId()).name,
																		 1 - p->GetTeamId());
				msg = _Tr("Client", "{0} dropped {1}'s intel", holderName, otherTeamName);
				chatWindow->AddMessage(msg);
			}
			
			{
				std::string holderName = p->GetName();
				std::string otherTeamName = world->GetTeam(1 - p->GetTeamId()).name;
				msg = _Tr("Client", "{0} dropped {1}'s Intel", holderName, otherTeamName);
				NetLog("%s", msg.c_str());
				centerMessageView->AddMessage(msg);
			}
		}
		
		void Client::PlayerDestroyedBlockWithWeaponOrTool(spades::IntVector3 blk){
			Vector3 origin = {blk.x + .5f, blk.y + .5f, blk.z + .5f};
			
			if(!map->IsSolid(blk.x, blk.y, blk.z))
				return;;
			
			Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Misc/BlockDestroy.wav");
			if(!IsMuted()){
				audioDevice->Play(c, origin,
								  AudioParam());
			}
			
			uint32_t col = map->GetColor(blk.x, blk.y, blk.z);
			IntVector3 colV = {(uint8_t)col,
				(uint8_t)(col >> 8), (uint8_t)(col >> 16)};
			
			EmitBlockDestroyFragments(blk, colV);
		}
		
		void Client::PlayerDiggedBlock(spades::IntVector3 blk){
			Vector3 origin = {blk.x + .5f, blk.y + .5f, blk.z + .5f};
			
			Handle<IAudioChunk> c = audioDevice->RegisterSound("Sounds/Misc/BlockDestroy.wav");
			if(!IsMuted()){
				audioDevice->Play(c, origin,
								  AudioParam());
			}
			
			for(int z = blk.z - 1 ; z <= blk.z + 1; z++){
				if(z < 0 || z > 61)
					continue;
				if(!map->IsSolid(blk.x, blk.y, z))
					continue;
				uint32_t col = map->GetColor(blk.x, blk.y, z);
				IntVector3 colV = {(uint8_t)col,
					(uint8_t)(col >> 8), (uint8_t)(col >> 16)};
				
				EmitBlockDestroyFragments(IntVector3::Make(blk.x, blk.y, z), colV);
			}
		}
		
		void Client::PlayerLeaving(spades::client::Player *p) {
			{
				std::string msg;
				msg = _Tr("Client", "Player {0} has left", chatWindow->TeamColorMessage(p->GetName(), p->GetTeamId()));
				chatWindow->AddMessage(msg);
			}
			{
				std::string msg;
				msg = _Tr("Client", "Player {0} has left", p->GetName());
				
				auto col = p->GetTeamId() < 2 ?
				world->GetTeam(p->GetTeamId()).color :
				IntVector3::Make(255, 255, 255);
				
				NetLog("%s", msg.c_str());
				scriptedUI->RecordChatLog(msg,
										  MakeVector4(col.x / 255.f, col.y / 255.f,
													  col.z / 255.f, 0.8f));
			}
		}
		
		void Client::PlayerJoinedTeam(spades::client::Player *p) {
			{
				std::string msg;
				msg = _Tr("Client", "{0} joined {1} team", p->GetName(),
						  chatWindow->TeamColorMessage(world->GetTeam(p->GetTeamId()).name,
													   p->GetTeamId()));
				chatWindow->AddMessage(msg);
			}
			{
				std::string msg;
				msg = _Tr("Client", "{0} joined {1} team", p->GetName(),
						  world->GetTeam(p->GetTeamId()).name);
				
				auto col = p->GetTeamId() < 2 ?
				world->GetTeam(p->GetTeamId()).color :
				IntVector3::Make(255, 255, 255);
				
				NetLog("%s", msg.c_str());
				scriptedUI->RecordChatLog(msg,
										  MakeVector4(col.x / 255.f, col.y / 255.f,
													  col.z / 255.f, 0.8f));
			}
		}
		
		void Client::GrenadeDestroyedBlock(spades::IntVector3 blk){
			for(int x = blk.x - 1; x <= blk.x + 1; x++)
				for(int y = blk.y - 1; y <= blk.y + 1; y++)
					for(int z = blk.z - 1 ; z <= blk.z + 1; z++){
						if(z < 0 || z > 61 || x < 0 || x >= 512 ||
						   y < 0 || y >= 512)
							continue;
						if(!map->IsSolid(x, y, z))
							continue;
						uint32_t col = map->GetColor(x, y, z);
						IntVector3 colV = {(uint8_t)col,
							(uint8_t)(col >> 8), (uint8_t)(col >> 16)};
						
						EmitBlockDestroyFragments(IntVector3::Make(x, y, z), colV);
					}
		}
		
		
		void Client::TeamWon(int teamId){
			std::string msg;
			msg = chatWindow->TeamColorMessage(world->GetTeam(teamId).name,
											   teamId);
			msg = _Tr("Client", "{0} wins!", msg);
			chatWindow->AddMessage(msg);
			
			msg = world->GetTeam(teamId).name;
			msg = _Tr("Client", "{0} Wins!", msg);
			NetLog("%s", msg.c_str());
			centerMessageView->AddMessage(msg);
			
			scriptedUI->RecordChatLog(msg, MakeVector4(1.f, 1.f, 1.f, 0.8f));
			
			if(world->GetLocalPlayer()){
				if(teamId == world->GetLocalPlayer()->GetTeamId()){
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/Win.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}else{
					Handle<IAudioChunk> chunk = audioDevice->RegisterSound("Sounds/Feedback/Lose.wav");
					audioDevice->PlayLocal(chunk, AudioParam());
				}
			}
		}
		
	}
}
