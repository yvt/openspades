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

#include <string>
#include <vector>
#include <Core/ServerAddress.h>
#include <Core/Math.h>
#include "PhysicsConstants.h"
#include "Player.h"

struct _ENetHost;
struct _ENetPeer;
typedef _ENetHost ENetHost;
typedef _ENetPeer ENetPeer;


namespace spades {
	namespace client {
		class Client;
		class Player;
		enum NetClientStatus {
			NetClientStatusNotConnected = 0,
			NetClientStatusConnecting,
			NetClientStatusReceivingMap,
			NetClientStatusConnected
		};
		
		class World;
		class NetPacketReader;
		struct PlayerInput;
		struct WeaponInput;
		class Grenade;
		class NetClient {
			Client *client;
			NetClientStatus status;
			ENetHost *host;
			ENetPeer *peer;
			std::string statusString;
			unsigned int mapSize;
			std::vector<char> mapData;
			
			std::vector<Vector3> savedPlayerPos;
			std::vector<Vector3> savedPlayerFront;
			std::vector<int> savedPlayerTeam;
			
			struct PosRecord {
				float time;
				bool valid;
				Vector3 pos;
				
				PosRecord():valid(false){}
			};
			
			std::vector<PosRecord> playerPosRecords;
			
			std::vector<std::vector<char> > savedPackets;
			
			int timeToTryMapLoad;
			bool tryMapLoadOnPacketType;
			
			unsigned int lastPlayerInput;
			unsigned int lastWeaponInput;
			
			// used for some scripts including Arena by Yourself
			IntVector3 temporaryPlayerBlockColor;
			
			void Handle(NetPacketReader&);
			World *GetWorld();
			Player *GetPlayer(int);
			Player *GetPlayerOrNull(int);
			Player *GetLocalPlayer();
			Player *GetLocalPlayerOrNull();
			
			std::string DisconnectReasonString(uint32_t);
			
			void MapLoaded();
		public:
			NetClient(Client *);
			~NetClient();
			
			NetClientStatus GetStatus() {
				return status;
			}
			
			std::string GetStatusString() {
				return statusString;
			}
			
			void Connect(const ServerAddress& hostname);
			void Disconnect();
			
			void DoEvents(int timeout = 0);
		
			void SendJoin(int team, WeaponType,
						  std::string name, int kills);
			void SendPosition();
			void SendOrientation(Vector3);
			void SendPlayerInput(PlayerInput);
			void SendWeaponInput(WeaponInput);
			void SendBlockAction(IntVector3, BlockActionType);
			void SendBlockLine(IntVector3 v1, IntVector3 v2);
			void SendReload();
			void SendTool();
			void SendGrenade(Grenade *);
			void SendHeldBlockColor();
			void SendHit(int targetPlayerId,
						 HitType type);
			void SendChat(std::string, bool global);
			void SendWeaponChange(WeaponType);
			void SendTeamChange(int team);
			
		};
	}
}
