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

#include <cstdint>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "PhysicsConstants.h"
#include "Player.h"
#include <Core/Debug.h>
#include <Core/Math.h>
#include <Core/ServerAddress.h>
#include <Core/Stopwatch.h>
#include <Core/VersionInfo.h>
#include <OpenSpades.h>

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

		enum NetExtensionType {
			ExtensionType128Player = 192,
			ExtensionTypeMessageTypes = 193,
			ExtensionTypeKickReason = 194,
		};

		class World;
		class NetPacketReader;
		struct PlayerInput;
		struct WeaponInput;
		class Grenade;
		struct GameProperties;
		class GameMapLoader;

		class NetClient {
			Client *client;
			NetClientStatus status;
			ENetHost *host;
			ENetPeer *peer;
			std::string statusString;

			class MapDownloadMonitor {
				Stopwatch sw;
				unsigned int numBytesDownloaded;
				GameMapLoader &mapLoader;
				bool receivedFirstByte;

			public:
				MapDownloadMonitor(GameMapLoader &);

				void AccumulateBytes(unsigned int);
				std::string GetDisplayedText();
			};

			/** Only valid in the `NetClientStatusReceivingMap` state */
			std::unique_ptr<GameMapLoader> mapLoader;
			/** Only valid in the `NetClientStatusReceivingMap` state */
			std::unique_ptr<MapDownloadMonitor> mapLoadMonitor;

			std::shared_ptr<GameProperties> properties;

			int protocolVersion;
			/** Extensions supported by both client and server (map of extension id → version) */
			std::unordered_map<uint8_t, uint8_t> extensions;
			/** Extensions implemented in this client (map of extension id → version) */
			std::unordered_map<uint8_t, uint8_t> implementedExtensions{
			  {ExtensionType128Player, 1},
			};

			class BandwidthMonitor {
				ENetHost *host;
				Stopwatch sw;
				double lastDown;
				double lastUp;

			public:
				BandwidthMonitor(ENetHost *);
				double GetDownlinkBps() { return lastDown * 8.; }
				double GetUplinkBps() { return lastUp * 8.; }
				void Update();
			};

			std::unique_ptr<BandwidthMonitor> bandwidthMonitor;

			std::vector<Vector3> savedPlayerPos;
			std::vector<Vector3> savedPlayerFront;
			std::vector<int> savedPlayerTeam;

			struct PosRecord {
				float time;
				bool valid;
				Vector3 pos;

				PosRecord() : valid(false) {}
			};

			std::vector<PosRecord> playerPosRecords;

			std::vector<std::vector<char>> savedPackets;

			int timeToTryMapLoad;
			bool tryMapLoadOnPacketType;

			unsigned int lastPlayerInput;
			unsigned int lastWeaponInput;

			// used for some scripts including Arena by Yourself
			IntVector3 temporaryPlayerBlockColor;

			bool HandleHandshakePackets(NetPacketReader &);
			void HandleExtensionPacket(NetPacketReader &);
			void HandleGamePacket(NetPacketReader &);
			stmp::optional<World &> GetWorld();
			Player &GetPlayer(int);
			stmp::optional<Player &> GetPlayerOrNull(int);
			Player &GetLocalPlayer();
			stmp::optional<Player &> GetLocalPlayerOrNull();

			std::string DisconnectReasonString(uint32_t);

			void MapLoaded();

			void SendVersion();
			void SendVersionEnhanced(const std::set<std::uint8_t> &propertyIds);
			void SendSupportedExtensions();

		public:
			NetClient(Client *);
			~NetClient();

			NetClientStatus GetStatus() { return status; }

			std::string GetStatusString();

			/**
			 * Gets how much portion of the map has completed loading.
			 * `GetStatus()` must be `NetClientStatusReceivingMap`.
			 *
			 * @return A value in range `[0, 1]`.
			 */
			float GetMapReceivingProgress();

			/**
			 * Return a non-null reference to `GameProperties` for this connection.
			 * Must be the connected state.
			 */
			std::shared_ptr<GameProperties> &GetGameProperties() {
				SPAssert(properties);
				return properties;
			}

			void Connect(const ServerAddress &hostname);
			void Disconnect();

			int GetPing();

			void DoEvents(int timeout = 0);

			void SendJoin(int team, WeaponType, std::string name, int kills);
			void SendPosition();
			void SendOrientation(Vector3);
			void SendPlayerInput(PlayerInput);
			void SendWeaponInput(WeaponInput);
			void SendBlockAction(IntVector3, BlockActionType);
			void SendBlockLine(IntVector3 v1, IntVector3 v2);
			void SendReload();
			void SendTool();
			void SendGrenade(const Grenade &);
			void SendHeldBlockColor();
			void SendHit(int targetPlayerId, HitType type);
			void SendChat(std::string, bool global);
			void SendWeaponChange(WeaponType);
			void SendTeamChange(int team);
			void SendHandShakeValid(int challenge);

			double GetDownlinkBps() { return bandwidthMonitor->GetDownlinkBps(); }
			double GetUplinkBps() { return bandwidthMonitor->GetUplinkBps(); }
		};
	} // namespace client
} // namespace spades
