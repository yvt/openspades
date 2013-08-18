//
//  NetClient.h
//  OpenSpades
//
//  Created by yvt on 7/16/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once 

#include <enet/enet.h>
#include <string>
#include <vector>
#include "../Core/Math.h"
#include "PhysicsConstants.h"
#include "Player.h"

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
			
			std::vector<std::vector<char> > savedPackets;
			
			int timeToTryMapLoad;
			bool tryMapLoadOnPacketType;
			
			// used for some scripts including Arena by Yourself
			IntVector3 temporaryPlayerBlockColor;
			
			void Handle(NetPacketReader&);
			World *GetWorld();
			Player *GetPlayer(int);
			Player *GetPlayerOrNull(int);
			Player *GetLocalPlayer();
			Player *GetLocalPlayerOrNull();
			
			std::string DisconnectReasonString(enet_uint32);
			
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
			
			void Connect(std::string hostname);
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
