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

#include <math.h>
#include <string.h>
#include <vector>

#include <enet/enet.h>

#include "CTFGameMode.h"
#include "Client.h"
#include "GameMap.h"
#include "Grenade.h"
#include "NetClient.h"
#include "Player.h"
#include "TCGameMode.h"
#include "World.h"
#include "GameProperties.h"
#include <Core/CP437.h>
#include <Core/Debug.h>
#include <Core/Debug.h>
#include <Core/DeflateStream.h>
#include <Core/Exception.h>
#include <Core/Math.h>
#include <Core/MemoryStream.h>
#include <Core/Settings.h>
#include <Core/Strings.h>
#include <Core/TMPUtils.h>

DEFINE_SPADES_SETTING(cg_unicode, "1");

namespace spades {
	namespace client {

		namespace {
			const char UtfSign = -1;

			enum { BLUE_FLAG = 0, GREEN_FLAG = 1, BLUE_BASE = 2, GREEN_BASE = 3 };
			enum PacketType {
				PacketTypePositionData = 0,
				PacketTypeOrientationData = 1,
				PacketTypeWorldUpdate = 2,
				PacketTypeInputData = 3,
				PacketTypeWeaponInput = 4,
				PacketTypeHitPacket = 5, // C2S
				PacketTypeSetHP = 5,     // S2C
				PacketTypeGrenadePacket = 6,
				PacketTypeSetTool = 7,
				PacketTypeSetColour = 8,
				PacketTypeExistingPlayer = 9,
				PacketTypeShortPlayerData = 10,
				PacketTypeMoveObject = 11,
				PacketTypeCreatePlayer = 12,
				PacketTypeBlockAction = 13,
				PacketTypeBlockLine = 14,
				PacketTypeStateData = 15,
				PacketTypeKillAction = 16,
				PacketTypeChatMessage = 17,
				PacketTypeMapStart = 18,         // S2C
				PacketTypeMapChunk = 19,         // S2C
				PacketTypePlayerLeft = 20,       // S2P
				PacketTypeTerritoryCapture = 21, // S2P
				PacketTypeProgressBar = 22,
				PacketTypeIntelCapture = 23,    // S2P
				PacketTypeIntelPickup = 24,     // S2P
				PacketTypeIntelDrop = 25,       // S2P
				PacketTypeRestock = 26,         // S2P
				PacketTypeFogColour = 27,       // S2C
				PacketTypeWeaponReload = 28,    // C2S2P
				PacketTypeChangeTeam = 29,      // C2S2P
				PacketTypeChangeWeapon = 30,    // C2S2P
				PacketTypeHandShakeInit = 31,   // S2C
				PacketTypeHandShakeReturn = 32, // C2S
				PacketTypeVersionGet = 33,      // S2C
				PacketTypeVersionSend = 34,     // C2S

			};

			enum class VersionInfoPropertyId : std::uint8_t {
				ApplicationNameAndVersion = 0,
				UserLocale = 1,
				ClientFeatureFlags1 = 2
			};

			enum class ClientFeatureFlags1 : std::uint32_t { None = 0, SupportsUnicode = 1 << 0 };

			ClientFeatureFlags1 operator|(ClientFeatureFlags1 a, ClientFeatureFlags1 b) {
				return (ClientFeatureFlags1)((uint32_t)a | (uint32_t)b);
			}
			ClientFeatureFlags1 &operator|=(ClientFeatureFlags1 &a, ClientFeatureFlags1 b) {
				return a = a | b;
			}

			std::string EncodeString(std::string str) {
				auto str2 = CP437::Encode(str, -1);
				if (!cg_unicode) {
					// ignore fallbacks
					return str2;
				}
				if (str2.find(-1) != std::string::npos) {
					// some fallbacks; always use UTF8
					str.insert(0, &UtfSign, 1);
				} else {
					str = str2;
				}
				return str;
			}

			std::string DecodeString(std::string s) {
				if (s.size() > 0 && s[0] == UtfSign) {
					return s.substr(1);
				}
				return CP437::Decode(s);
			}
		}

		class NetPacketReader {
			std::vector<char> data;
			size_t pos;

		public:
			NetPacketReader(ENetPacket *packet) {
				SPADES_MARK_FUNCTION();

				data.resize(packet->dataLength);
				memcpy(data.data(), packet->data, packet->dataLength);
				enet_packet_destroy(packet);
				pos = 1;
			}

			NetPacketReader(const std::vector<char> inData) {
				data = inData;
				pos = 1;
			}

			PacketType GetType() { return (PacketType)data[0]; }

			uint32_t ReadInt() {
				SPADES_MARK_FUNCTION();

				uint32_t value = 0;
				if (pos + 4 > data.size()) {
					SPRaise("Received packet truncated");
				}
				value |= ((uint32_t)(uint8_t)data[pos++]);
				value |= ((uint32_t)(uint8_t)data[pos++]) << 8;
				value |= ((uint32_t)(uint8_t)data[pos++]) << 16;
				value |= ((uint32_t)(uint8_t)data[pos++]) << 24;
				return value;
			}

			uint16_t ReadShort() {
				SPADES_MARK_FUNCTION();

				uint32_t value = 0;
				if (pos + 2 > data.size()) {
					SPRaise("Received packet truncated");
				}
				value |= ((uint32_t)(uint8_t)data[pos++]);
				value |= ((uint32_t)(uint8_t)data[pos++]) << 8;
				return (uint16_t)value;
			}

			uint8_t ReadByte() {
				SPADES_MARK_FUNCTION();

				if (pos >= data.size()) {
					SPRaise("Received packet truncated");
				}
				return (uint8_t)data[pos++];
			}

			float ReadFloat() {
				SPADES_MARK_FUNCTION();
				union {
					float f;
					uint32_t v;
				};
				v = ReadInt();
				return f;
			}

			IntVector3 ReadIntColor() {
				SPADES_MARK_FUNCTION();
				IntVector3 col;
				col.z = ReadByte();
				col.y = ReadByte();
				col.x = ReadByte();
				return col;
			}

			Vector3 ReadFloatColor() {
				SPADES_MARK_FUNCTION();
				Vector3 col;
				col.z = ReadByte() / 255.f;
				col.y = ReadByte() / 255.f;
				col.x = ReadByte() / 255.f;
				return col;
			}

			std::size_t GetNumRemainingBytes() { return data.size() - pos; }

			std::vector<char> GetData() { return data; }

			std::string ReadData(size_t siz) {
				if (pos + siz > data.size()) {
					SPRaise("Received packet truncated");
				}
				std::string s = std::string(data.data() + pos, siz);
				pos += siz;
				return s;
			}
			std::string ReadRemainingData() {
				return std::string(data.data() + pos, data.size() - pos);
			}

			std::string ReadString(size_t siz) {
				// convert to C string once so that
				// null-chars are removed
				std::string s = ReadData(siz).c_str();
				s = DecodeString(s);
				return s;
			}
			std::string ReadRemainingString() {
				// convert to C string once so that
				// null-chars are removed
				std::string s = ReadRemainingData().c_str();
				s = DecodeString(s);
				return s;
			}

			void DumpDebug() {
#if 1
				char buf[1024];
				std::string str;
				sprintf(buf, "Packet 0x%02x [len=%d]", (int)GetType(), (int)data.size());
				str = buf;
				int bytes = (int)data.size();
				if (bytes > 64) {
					bytes = 64;
				}
				for (int i = 0; i < bytes; i++) {
					sprintf(buf, " %02x", (unsigned int)(unsigned char)data[i]);
					str += buf;
				}

				SPLog("%s", str.c_str());
#endif
			}
		};

		class NetPacketWriter {
			std::vector<char> data;

		public:
			NetPacketWriter(PacketType type) { data.push_back(type); }

			void Write(uint8_t v) {
				SPADES_MARK_FUNCTION_DEBUG();
				data.push_back(v);
			}
			void Write(uint16_t v) {
				SPADES_MARK_FUNCTION_DEBUG();
				data.push_back((char)(v));
				data.push_back((char)(v >> 8));
			}
			void Write(uint32_t v) {
				SPADES_MARK_FUNCTION_DEBUG();
				data.push_back((char)(v));
				data.push_back((char)(v >> 8));
				data.push_back((char)(v >> 16));
				data.push_back((char)(v >> 24));
			}
			void Write(float v) {
				SPADES_MARK_FUNCTION_DEBUG();
				union {
					float f;
					uint32_t i;
				};
				f = v;
				Write(i);
			}
			void WriteColor(IntVector3 v) {
				Write((uint8_t)v.z);
				Write((uint8_t)v.y);
				Write((uint8_t)v.x);
			}

			void Write(std::string str) {
				str = EncodeString(str);
				data.insert(data.end(), str.begin(), str.end());
			}

			void Write(std::string str, size_t fillLen) {
				str = EncodeString(str);
				Write(str.substr(0, fillLen));
				size_t sz = str.size();
				while (sz < fillLen) {
					Write((uint8_t)0);
					sz++;
				}
			}

			std::size_t GetPosition() { return data.size(); }

			void Update(std::size_t position, std::uint8_t newValue) {
				SPADES_MARK_FUNCTION_DEBUG();

				if (position >= data.size()) {
					SPRaise("Invalid write (%d should be less than %d)", (int)position,
					        (int)data.size());
				}

				data[position] = static_cast<char>(newValue);
			}

			void Update(std::size_t position, std::uint32_t newValue) {
				SPADES_MARK_FUNCTION_DEBUG();

				if (position + 4 > data.size()) {
					SPRaise("Invalid write (%d should be less than or equal to %d)",
					        (int)(position + 4), (int)data.size());
				}

				// Assuming the target platform is little endian and supports
				// unaligned memory access...
				*reinterpret_cast<std::uint32_t *>(data.data() + position) = newValue;
			}

			ENetPacket *CreatePacket(int flag = ENET_PACKET_FLAG_RELIABLE) {
				return enet_packet_create(data.data(), data.size(), flag);
			}
		};

		NetClient::NetClient(Client *c) : client(c), host(nullptr), peer(nullptr) {
			SPADES_MARK_FUNCTION();

			enet_initialize();
			SPLog("ENet initialized");

			host = enet_host_create(NULL, 1, 1, 100000, 100000);
			SPLog("ENet host created");
			if (!host) {
				SPRaise("Failed to create ENet host");
			}

			if (enet_host_compress_with_range_coder(host) < 0)
				SPRaise("Failed to enable ENet Range coder.");

			SPLog("ENet Range Coder Enabled");

			peer = NULL;
			status = NetClientStatusNotConnected;

			lastPlayerInput = 0;
			lastWeaponInput = 0;

			savedPlayerPos.resize(128);
			savedPlayerFront.resize(128);
			savedPlayerTeam.resize(128);
			playerPosRecords.resize(128);

			std::fill(savedPlayerTeam.begin(), savedPlayerTeam.end(), -1);

			bandwidthMonitor.reset(new BandwidthMonitor(host));
		}
		NetClient::~NetClient() {
			SPADES_MARK_FUNCTION();

			Disconnect();
			if (host)
				enet_host_destroy(host);
			bandwidthMonitor.reset();
			SPLog("ENet host destroyed");
		}

		void NetClient::Connect(const ServerAddress &hostname) {
			SPADES_MARK_FUNCTION();

			Disconnect();
			SPAssert(status == NetClientStatusNotConnected);

			switch (hostname.GetProtocolVersion()) {
				case ProtocolVersion::v075:
					SPLog("Using Ace of Spades 0.75 protocol");
					protocolVersion = 3;
					break;
				case ProtocolVersion::v076:
					SPLog("Using Ace of Spades 0.76 protocol");
					protocolVersion = 4;
					break;
				default: SPRaise("Invalid ProtocolVersion"); break;
			}

			ENetAddress addr = hostname.GetENetAddress();
			SPLog("Connecting to %u:%u", (unsigned int)addr.host, (unsigned int)addr.port);

			savedPackets.clear();

			peer = enet_host_connect(host, &addr, 1, protocolVersion);
			if (peer == NULL) {
				SPRaise("Failed to create ENet peer");
			}

			properties.reset(new GameProperties(hostname.GetProtocolVersion()));

			status = NetClientStatusConnecting;
			statusString = _Tr("NetClient", "Connecting to the server");
			timeToTryMapLoad = 0;
		}

		void NetClient::Disconnect() {
			SPADES_MARK_FUNCTION();

			if (!peer)
				return;
			enet_peer_disconnect(peer, 0);

			status = NetClientStatusNotConnected;
			statusString = _Tr("NetClient", "Not connected");

			savedPackets.clear();

			ENetEvent event;
			SPLog("Waiting for graceful disconnection");
			while (enet_host_service(host, &event, 1000) > 0) {
				switch (event.type) {
					case ENET_EVENT_TYPE_RECEIVE: enet_packet_destroy(event.packet); break;
					case ENET_EVENT_TYPE_DISCONNECT:
						// disconnected safely
						// FIXME: release peer
						enet_peer_reset(peer);
						peer = NULL;
						return;
					default:;
						// discard
				}
			}

			SPLog("Connection terminated");
			enet_peer_reset(peer);
			// FXIME: release peer
			peer = NULL;
		}

		int NetClient::GetPing() {
			SPADES_MARK_FUNCTION();

			if (status == NetClientStatusNotConnected)
				return -1;

			auto rtt = peer->roundTripTime;
			if (rtt == 0)
				return -1;
			return static_cast<int>(rtt);
		}

		void NetClient::DoEvents(int timeout) {
			SPADES_MARK_FUNCTION();

			if (status == NetClientStatusNotConnected)
				return;

			if (bandwidthMonitor)
				bandwidthMonitor->Update();

			ENetEvent event;
			while (enet_host_service(host, &event, timeout) > 0) {
				if (event.type == ENET_EVENT_TYPE_DISCONNECT) {
					if (GetWorld()) {
						client->SetWorld(NULL);
					}

					enet_peer_reset(peer);
					peer = NULL;
					status = NetClientStatusNotConnected;

					SPLog("Disconnected (data = 0x%08x)", (unsigned int)event.data);
					statusString = "Disconnected: " + DisconnectReasonString(event.data);
					SPRaise("Disconnected: %s", DisconnectReasonString(event.data).c_str());
				}

				stmp::optional<NetPacketReader> readerOrNone;

				if (event.type == ENET_EVENT_TYPE_RECEIVE) {
					readerOrNone.reset(event.packet);
					auto &reader = readerOrNone.value();

					try {
						if (HandleHandshakePacket(reader)) {
							continue;
						}
					} catch (const std::exception &ex) {
						int type = reader.GetType();
						reader.DumpDebug();
						SPRaise("Exception while handling packet type 0x%08x:\n%s", type,
						        ex.what());
					}
				}

				if (status == NetClientStatusConnecting) {
					if (event.type == ENET_EVENT_TYPE_CONNECT) {
						statusString = _Tr("NetClient", "Awaiting for state");
					} else if (event.type == ENET_EVENT_TYPE_RECEIVE) {
						auto &reader = readerOrNone.value();
						reader.DumpDebug();
						if (reader.GetType() != PacketTypeMapStart) {
							SPRaise("Unexpected packet: %d", (int)reader.GetType());
						}

						mapSize = reader.ReadInt();
						status = NetClientStatusReceivingMap;
						statusString = _Tr("NetClient", "Loading snapshot");
						timeToTryMapLoad = 30;
						tryMapLoadOnPacketType = true;
					}
				} else if (status == NetClientStatusReceivingMap) {
					if (event.type == ENET_EVENT_TYPE_RECEIVE) {
						auto &reader = readerOrNone.value();

						if (reader.GetType() == PacketTypeMapChunk) {
							std::vector<char> dt = reader.GetData();
							dt.erase(dt.begin());
							mapData.insert(mapData.end(), dt.begin(), dt.end());

							timeToTryMapLoad = 200;

							statusString = _Tr("NetClient", "Loading snapshot ({0}/{1})",
							                   mapData.size(), mapSize);

							if (mapSize == mapData.size()) {
								status = NetClientStatusConnected;
								statusString = _Tr("NetClient", "Connected");

								try {
									MapLoaded();
								} catch (const std::exception &ex) {
									if (strstr(ex.what(), "File truncated") ||
									    strstr(ex.what(), "EOF reached")) {
										SPLog("Map decoder returned error. Maybe we will get more "
										      "data...:\n%s",
										      ex.what());
										// hack: more data to load...
										status = NetClientStatusReceivingMap;
										statusString = _Tr("NetClient", "Still loading...");
									} else {
										Disconnect();
										statusString = _Tr("NetClient", "Error");
										throw;
									}

								} catch (...) {
									Disconnect();
									statusString = _Tr("NetClient", "Error");
									throw;
								}
							}

						} else {
							reader.DumpDebug();

							// On pyspades and derivative servers the actual size of the map data
							// cannot be known in beforehand, so we have to find the end of the data
							// by one of other means. One indicator for this would be a packet of a
							// type other than MapChunk, which usually marks the end of map data
							// transfer.
							//
							// However, we can't rely on this heuristics entirely because there are
							// several occasions where the server would send non-MapChunk packets
							// during map loading sequence, for example:
							//
							//  - World update packets (WorldUpdate, ExistingPlayer, and
							//    CreatePlayer) for the current round. We must store such packets
							//	  temporarily and process them later when a `World` is created.
							//
							//  - Leftover reload packet from the previous round. This happens when
							//    you initiate the reload action and a map change occurs before it
							// 	  is completed. In pyspades, sending a reload packet is implemented
							// 	  by registering a callback function to the Twisted reactor. This
							//    callback function sends a reload packet, but it does not check if
							//    the current game round is finished, nor is it unregistered on a
							//    map change.
							//
							//    Such a reload packet would not (and should not) have any effect on
							//    the current round. Also, an attempt to process it would result in
							//    an "invalid player ID" exception, so we simply drop it during
							//    map load sequence.
							//
							if (reader.GetType() == PacketTypeWeaponReload) {
								// Drop reload packets
							} else if (reader.GetType() != PacketTypeWorldUpdate &&
							           reader.GetType() != PacketTypeExistingPlayer &&
							           reader.GetType() != PacketTypeCreatePlayer &&
							           tryMapLoadOnPacketType) {
								status = NetClientStatusConnected;
								statusString = _Tr("NetClient", "Connected");

								try {
									MapLoaded();
								} catch (const std::exception &ex) {
									tryMapLoadOnPacketType = false;
									if (strstr(ex.what(), "File truncated") ||
									    strstr(ex.what(), "EOF reached")) {
										SPLog("Map decoder returned error. Maybe we will get more "
										      "data...:\n%s",
										      ex.what());
										// hack: more data to load...
										status = NetClientStatusReceivingMap;
										statusString = _Tr("NetClient", "Still loading...");
										goto stillLoading;
									} else {
										Disconnect();
										statusString = _Tr("NetClient", "Error");
										throw;
									}
								} catch (...) {
									Disconnect();
									statusString = _Tr("NetClient", "Error");
									throw;
								}
								HandleGamePacket(reader);
							} else {
							stillLoading:
								savedPackets.push_back(reader.GetData());
							}

							// HandleGamePacket(reader);
						}
					}
				} else if (status == NetClientStatusConnected) {
					if (event.type == ENET_EVENT_TYPE_RECEIVE) {
						auto &reader = readerOrNone.value();
						// reader.DumpDebug();
						try {
							HandleGamePacket(reader);
						} catch (const std::exception &ex) {
							int type = reader.GetType();
							reader.DumpDebug();
							SPRaise("Exception while handling packet type 0x%08x:\n%s", type,
							        ex.what());
						}
					}
				}
			}

			if (status == NetClientStatusReceivingMap) {
				if (timeToTryMapLoad > 0) {
					timeToTryMapLoad--;
					if (timeToTryMapLoad == 0) {
						try {
							MapLoaded();
						} catch (const std::exception &ex) {
							if ((strstr(ex.what(), "File truncated") ||
							     strstr(ex.what(), "EOF reached")) &&
							    savedPackets.size() < 400) {
								// hack: more data to load...
								SPLog(
								  "Map decoder returned error. Maybe we will get more data...:\n%s",
								  ex.what());
								status = NetClientStatusReceivingMap;
								statusString = _Tr("NetClient", "Still loading...");
								timeToTryMapLoad = 200;
							} else {
								Disconnect();
								statusString = _Tr("NetClient", "Error");
								throw;
							}
						} catch (...) {
							Disconnect();
							statusString = _Tr("NetClient", "Error");
							throw;
						}
					}
				}
			}
		}

		World *NetClient::GetWorld() { return client->GetWorld(); }

		Player *NetClient::GetPlayerOrNull(int pId) {
			SPADES_MARK_FUNCTION();
			if (!GetWorld())
				SPRaise("Invalid Player ID %d: No world", pId);
			if (pId < 0 || pId >= GetWorld()->GetNumPlayerSlots())
				return NULL;
			return GetWorld()->GetPlayer(pId);
		}
		Player *NetClient::GetPlayer(int pId) {
			SPADES_MARK_FUNCTION();
			if (!GetWorld())
				SPRaise("Invalid Player ID %d: No world", pId);
			if (pId < 0 || pId >= GetWorld()->GetNumPlayerSlots())
				SPRaise("Invalid Player ID %d: Out of range", pId);
			if (!GetWorld()->GetPlayer(pId))
				SPRaise("Invalid Player ID %d: Doesn't exist", pId);
			return GetWorld()->GetPlayer(pId);
		}

		Player *NetClient::GetLocalPlayer() {
			SPADES_MARK_FUNCTION();
			if (!GetWorld())
				SPRaise("Failed to get local player: no world");
			if (!GetWorld()->GetLocalPlayer())
				SPRaise("Failed to get local player: no local player");
			return GetWorld()->GetLocalPlayer();
		}

		Player *NetClient::GetLocalPlayerOrNull() {
			SPADES_MARK_FUNCTION();
			if (!GetWorld())
				SPRaise("Failed to get local player: no world");
			return GetWorld()->GetLocalPlayer();
		}
		PlayerInput ParsePlayerInput(uint8_t bits) {
			PlayerInput inp;
			inp.moveForward = (bits & (1)) != 0;
			inp.moveBackward = (bits & (1 << 1)) != 0;
			inp.moveLeft = (bits & (1 << 2)) != 0;
			inp.moveRight = (bits & (1 << 3)) != 0;
			inp.jump = (bits & (1 << 4)) != 0;
			inp.crouch = (bits & (1 << 5)) != 0;
			inp.sneak = (bits & (1 << 6)) != 0;
			inp.sprint = (bits & (1 << 7)) != 0;
			return inp;
		}

		WeaponInput ParseWeaponInput(uint8_t bits) {
			WeaponInput inp;
			inp.primary = ((bits & (1)) != 0);
			inp.secondary = ((bits & (1 << 1)) != 0);
			return inp;
		}

		std::string NetClient::DisconnectReasonString(enet_uint32 num) {
			switch (num) {
				case 1: return _Tr("NetClient", "You are banned from this server.");
				case 2:
					// FIXME: this number seems to be used when per-IP connection limit was
					// exceeded.
					//        we need to check other usages
					return _Tr("NetClient", "You were kicked from this server.");
				case 3: return _Tr("NetClient", "Incompatible client protocol version.");
				case 4: return _Tr("NetClient", "Server full");
				case 10: return _Tr("NetClient", "You were kicked from this server.");
				default: return _Tr("NetClient", "Unknown Reason");
			}
		}

		bool NetClient::HandleHandshakePacket(spades::client::NetPacketReader &reader) {
			SPADES_MARK_FUNCTION();

			switch (reader.GetType()) {
				case PacketTypeHandShakeInit: SendHandShakeValid(reader.ReadInt()); return true;
				case PacketTypeVersionGet: {
					if (reader.GetNumRemainingBytes() > 0) {
						// Enhanced variant
						std::set<std::uint8_t> propertyIds;
						while (reader.GetNumRemainingBytes()) {
							propertyIds.insert(reader.ReadByte());
						}
						SendVersionEnhanced(propertyIds);
					} else {
						// Simple variant
						SendVersion();
					}
				}
					return true;
				default: return false;
			}
		}

		void NetClient::HandleGamePacket(spades::client::NetPacketReader &reader) {
			SPADES_MARK_FUNCTION();

			switch (reader.GetType()) {
				case PacketTypePositionData: {
					Player *p = GetLocalPlayer();
					Vector3 pos;
					if (reader.GetData().size() < 12) {
						// sometimes 00 00 00 00 packet is sent.
						// ignore this now
						break;
					}
					pos.x = reader.ReadFloat();
					pos.y = reader.ReadFloat();
					pos.z = reader.ReadFloat();
					p->SetPosition(pos);
				} break;
				case PacketTypeOrientationData: {
					Player *p = GetLocalPlayer();
					Vector3 pos;
					pos.x = reader.ReadFloat();
					pos.y = reader.ReadFloat();
					pos.z = reader.ReadFloat();
					p->SetOrientation(pos);
				} break;
				case PacketTypeWorldUpdate: {
					// reader.DumpDebug();
					int bytesPerEntry = 24;
					if (protocolVersion == 4)
						bytesPerEntry++;

					client->MarkWorldUpdate();

					int entries = static_cast<int>(reader.GetData().size() / bytesPerEntry);
					for (int i = 0; i < entries; i++) {
						int idx = i;
						if (protocolVersion == 4) {
							idx = reader.ReadByte();
							if (idx < 0 || idx >= properties->GetMaxNumPlayerSlots()) {
								SPRaise("Invalid player number %d received with WorldUpdate", idx);
							}
						}
						Vector3 pos, front;
						pos.x = reader.ReadFloat();
						pos.y = reader.ReadFloat();
						pos.z = reader.ReadFloat();
						front.x = reader.ReadFloat();
						front.y = reader.ReadFloat();
						front.z = reader.ReadFloat();

						savedPlayerPos[idx] = pos;
						savedPlayerFront[idx] = front;
						if (pos.x != 0.f || pos.y != 0.f || pos.z != 0.f || front.x != 0.f ||
						    front.y != 0.f || front.z != 0.f) {
							Player *p;
							SPAssert(!std::isnan(pos.x));
							SPAssert(!std::isnan(pos.y));
							SPAssert(!std::isnan(pos.z));
							SPAssert(!std::isnan(front.x));
							SPAssert(!std::isnan(front.y));
							SPAssert(!std::isnan(front.z));
							SPAssert(front.GetLength() < 40.f);
							if (GetWorld()) {
								p = GetWorld()->GetPlayer(idx);
								if (p) {
									if (p != GetWorld()->GetLocalPlayer()) {
										p->SetPosition(pos);
										p->SetOrientation(front);

										PosRecord &rec = playerPosRecords[idx];
										if (rec.valid) {
											float timespan = GetWorld()->GetTime() - rec.time;
											timespan = std::max(0.16f, timespan);
											Vector3 vel = (pos - rec.pos) / timespan;
											vel *= 1.f / 32.f;
											p->SetVelocity(vel);
										}

										rec.valid = true;
										rec.pos = pos;
										rec.time = GetWorld()->GetTime();
									}
								}
							}
						}
					}
					SPAssert(reader.ReadRemainingData().empty());
				} break;
				case PacketTypeInputData:
					if (!GetWorld())
						break;
					{
						int pId = reader.ReadByte();
						Player *p = GetPlayer(pId);

						PlayerInput inp = ParsePlayerInput(reader.ReadByte());

						if (GetWorld()->GetLocalPlayer() == p) {
							// handle "/fly" jump
							if (inp.jump) {
								if (!p) {
									SPRaise("Local player is null");
								}
								p->ForceJump();
							}
							break;
						}

						p->SetInput(inp);
					}
					break;

				case PacketTypeWeaponInput:
					if (!GetWorld())
						break;
					{
						int pId = reader.ReadByte();
						Player *p = GetPlayer(pId);

						WeaponInput inp = ParseWeaponInput(reader.ReadByte());

						if (GetWorld()->GetLocalPlayer() == p)
							break;

						p->SetWeaponInput(inp);
					}
					break;

				// Hit Packet is Client-to-Server!
				case PacketTypeSetHP: {
					Player *p = GetLocalPlayer();
					int hp = reader.ReadByte();
					int type = reader.ReadByte(); // 0=fall, 1=weap
					Vector3 hurtPos;
					hurtPos.x = reader.ReadFloat();
					hurtPos.y = reader.ReadFloat();
					hurtPos.z = reader.ReadFloat();
					p->SetHP(hp, type ? HurtTypeWeapon : HurtTypeFall, hurtPos);
				} break;

				case PacketTypeGrenadePacket:
					if (!GetWorld())
						break;
					{
						reader.ReadByte(); // skip player Id
						// Player *p = GetPlayerOrNull(reader.ReadByte());
						float fuseLen = reader.ReadFloat();
						Vector3 pos, vel;
						pos.x = reader.ReadFloat();
						pos.y = reader.ReadFloat();
						pos.z = reader.ReadFloat();
						vel.x = reader.ReadFloat();
						vel.y = reader.ReadFloat();
						vel.z = reader.ReadFloat();
						/* blockpower mode may emit local player's grenade
						if(p == GetLocalPlayerOrNull()){
						    // local player's grenade is already
						    // emit by Player
						    break;
						}*/

						Grenade *g = new Grenade(GetWorld(), pos, vel, fuseLen);
						GetWorld()->AddGrenade(g);
					}
					break;

				case PacketTypeSetTool: {
					Player *p = GetPlayer(reader.ReadByte());
					int tool = reader.ReadByte();
					switch (tool) {
						case 0: p->SetTool(Player::ToolSpade); break;
						case 1: p->SetTool(Player::ToolBlock); break;
						case 2: p->SetTool(Player::ToolWeapon); break;
						case 3: p->SetTool(Player::ToolGrenade); break;
						default: SPRaise("Received invalid tool type: %d", tool);
					}
				} break;
				case PacketTypeSetColour: {
					Player *p = GetPlayerOrNull(reader.ReadByte());
					IntVector3 col = reader.ReadIntColor();
					if (p)
						p->SetHeldBlockColor(col);
					else
						temporaryPlayerBlockColor = col;
				} break;
				case PacketTypeExistingPlayer:
					if (!GetWorld())
						break;
					{
						int pId = reader.ReadByte();
						int team = reader.ReadByte();
						int weapon = reader.ReadByte();
						int tool = reader.ReadByte();
						int kills = reader.ReadInt();
						IntVector3 color = reader.ReadIntColor();
						std::string name = reader.ReadRemainingString();
						// TODO: decode name?

						WeaponType wType;
						switch (weapon) {
							case 0: wType = RIFLE_WEAPON; break;
							case 1: wType = SMG_WEAPON; break;
							case 2: wType = SHOTGUN_WEAPON; break;
							default: SPRaise("Received invalid weapon: %d", weapon);
						}

						Player *p = new Player(GetWorld(), pId, wType, team, savedPlayerPos[pId],
						                       GetWorld()->GetTeam(team).color);
						p->SetHeldBlockColor(color);
						// p->SetOrientation(savedPlayerFront[pId]);
						GetWorld()->SetPlayer(pId, p);

						switch (tool) {
							case 0: p->SetTool(Player::ToolSpade); break;
							case 1: p->SetTool(Player::ToolBlock); break;
							case 2: p->SetTool(Player::ToolWeapon); break;
							case 3: p->SetTool(Player::ToolGrenade); break;
							default: SPRaise("Received invalid tool type: %d", tool);
						}

						World::PlayerPersistent &pers = GetWorld()->GetPlayerPersistent(pId);
						pers.name = name;
						pers.kills = kills;

						savedPlayerTeam[pId] = team;
					}
					break;
				case PacketTypeShortPlayerData: SPRaise("Unexpected: received Short Player Data");
				case PacketTypeMoveObject:
					if (!GetWorld())
						SPRaise("No world");
					{
						uint8_t type = reader.ReadByte();
						uint8_t state = reader.ReadByte();
						Vector3 pos;
						pos.x = reader.ReadFloat();
						pos.y = reader.ReadFloat();
						pos.z = reader.ReadFloat();

						IGameMode *mode = GetWorld()->GetMode();
						if (mode && IGameMode::m_CTF == mode->ModeType()) {
							CTFGameMode *ctf = static_cast<CTFGameMode *>(mode);
							switch (type) {
								case BLUE_BASE: ctf->GetTeam(0).basePos = pos; break;
								case BLUE_FLAG: ctf->GetTeam(0).flagPos = pos; break;
								case GREEN_BASE: ctf->GetTeam(1).basePos = pos; break;
								case GREEN_FLAG: ctf->GetTeam(1).flagPos = pos; break;
							}
						} else if (mode && IGameMode::m_TC == mode->ModeType()) {
							TCGameMode *tc = static_cast<TCGameMode *>(mode);
							if (type >= tc->GetNumTerritories()) {
								SPRaise("Invalid territory id specified: %d (max = %d)", (int)type,
								        tc->GetNumTerritories() - 1);
							}

							if (state > 2) {
								SPRaise("Invalid state %d specified for territory owner.",
								        (int)state);
							}

							TCGameMode::Territory *t = tc->GetTerritory(type);
							t->pos = pos;
							t->ownerTeamId = state; /*
							 t->progressBasePos = 0.f;
							 t->progressRate = 0.f;
							 t->progressStartTime = 0.f;
							 t->capturingTeamId = -1;*/
						}
					}
					break;
				case PacketTypeCreatePlayer: {
					if (!GetWorld())
						SPRaise("No world");
					int pId = reader.ReadByte();
					int weapon = reader.ReadByte();
					int team = reader.ReadByte();
					Vector3 pos;
					pos.x = reader.ReadFloat();
					pos.y = reader.ReadFloat();
					pos.z = reader.ReadFloat() - 2.f;
					std::string name = reader.ReadRemainingString();
					// TODO: decode name?

					if (pId < 0 || pId >= properties->GetMaxNumPlayerSlots()) {
						SPLog("Ignoring invalid player number %d (bug in pyspades?: %s)", pId,
						      name.c_str());
						break;
					}
					WeaponType wType;
					switch (weapon) {
						case 0: wType = RIFLE_WEAPON; break;
						case 1: wType = SMG_WEAPON; break;
						case 2: wType = SHOTGUN_WEAPON; break;
						default: SPRaise("Received invalid weapon: %d", weapon);
					}

					Player *p = new Player(GetWorld(), pId, wType, team, savedPlayerPos[pId],

					                       GetWorld()->GetTeam(team).color);
					p->SetPosition(pos);
					GetWorld()->SetPlayer(pId, p);

					World::PlayerPersistent &pers = GetWorld()->GetPlayerPersistent(pId);

					if (!name.empty()) // sometimes becomes empty
						pers.name = name;

					playerPosRecords[pId].valid = false;

					if (pId == GetWorld()->GetLocalPlayerIndex()) {
						client->LocalPlayerCreated();
						lastPlayerInput = 0xffffffff;
						lastWeaponInput = 0xffffffff;
						SendHeldBlockColor(); // ensure block color synchronized
					} else {
						if (team < 2 && pId < (int)playerPosRecords.size()) {
							PosRecord &rec = playerPosRecords[pId];

							rec.valid = true;
							rec.pos = pos;
							rec.time = GetWorld()->GetTime();
						}
						if (savedPlayerTeam[pId] != team) {

							client->PlayerJoinedTeam(p);

							savedPlayerTeam[pId] = team;
						}
					}
					client->PlayerSpawned(p);

				} break;
				case PacketTypeBlockAction: {
					Player *p = GetPlayerOrNull(reader.ReadByte());
					int action = reader.ReadByte();
					IntVector3 pos;
					pos.x = reader.ReadInt();
					pos.y = reader.ReadInt();
					pos.z = reader.ReadInt();

					std::vector<IntVector3> cells;
					if (action == 0) {
						bool replace = GetWorld()->GetMap()->IsSolidWrapped(pos.x, pos.y, pos.z);
						if (!p) {
							GetWorld()->CreateBlock(pos, temporaryPlayerBlockColor);
						} else {
							GetWorld()->CreateBlock(pos, p->GetBlockColor());
							client->PlayerCreatedBlock(p);
							if (!replace) {
								p->UsedBlocks(1);
							}
						}
					} else if (action == 1) {
						cells.push_back(pos);
						client->PlayerDestroyedBlockWithWeaponOrTool(pos);
						GetWorld()->DestroyBlock(cells);

						if (p && p->GetTool() == Player::ToolSpade) {
							p->GotBlock();
						}
					} else if (action == 2) {
						// dig
						client->PlayerDiggedBlock(pos);
						for (int z = -1; z <= 1; z++)
							cells.push_back(IntVector3::Make(pos.x, pos.y, pos.z + z));
						GetWorld()->DestroyBlock(cells);
					} else if (action == 3) {
						// grenade
						client->GrenadeDestroyedBlock(pos);
						for (int x = -1; x <= 1; x++)
							for (int y = -1; y <= 1; y++)
								for (int z = -1; z <= 1; z++)
									cells.push_back(
									  IntVector3::Make(pos.x + x, pos.y + y, pos.z + z));
						GetWorld()->DestroyBlock(cells);
					}
				} break;
				case PacketTypeBlockLine: {
					Player *p = GetPlayerOrNull(reader.ReadByte());
					IntVector3 pos1, pos2;
					pos1.x = reader.ReadInt();
					pos1.y = reader.ReadInt();
					pos1.z = reader.ReadInt();
					pos2.x = reader.ReadInt();
					pos2.y = reader.ReadInt();
					pos2.z = reader.ReadInt();

					IntVector3 col = p ? p->GetBlockColor() : temporaryPlayerBlockColor;
					std::vector<IntVector3> cells;
					cells = GetWorld()->CubeLine(pos1, pos2, 50);

					for (size_t i = 0; i < cells.size(); i++) {
						if (!GetWorld()->GetMap()->IsSolid(cells[i].x, cells[i].y, cells[i].z)) {
							GetWorld()->CreateBlock(cells[i], col);
						}
					}

					if (p) {
						p->UsedBlocks(static_cast<int>(cells.size()));
						client->PlayerCreatedBlock(p);
					}
				} break;
				case PacketTypeStateData:
					if (!GetWorld())
						break;
					{
						// receives my player info.
						int pId = reader.ReadByte();
						IntVector3 fogColor = reader.ReadIntColor();
						IntVector3 teamColors[2];
						teamColors[0] = reader.ReadIntColor();
						teamColors[1] = reader.ReadIntColor();

						std::string teamNames[2];
						teamNames[0] = reader.ReadString(10);
						teamNames[1] = reader.ReadString(10);

						World::Team &t1 = GetWorld()->GetTeam(0);
						World::Team &t2 = GetWorld()->GetTeam(1);
						t1.color = teamColors[0];
						t2.color = teamColors[1];
						t1.name = teamNames[0];
						t2.name = teamNames[1];

						GetWorld()->SetFogColor(fogColor);
						GetWorld()->SetLocalPlayerIndex(pId);

						int mode = reader.ReadByte();
						if (mode == 0) {
							// CTF
							CTFGameMode *mode = new CTFGameMode();
							try {
								CTFGameMode::Team &mt1 = mode->GetTeam(0);
								CTFGameMode::Team &mt2 = mode->GetTeam(1);

								mt1.score = reader.ReadByte();
								mt2.score = reader.ReadByte();
								mode->SetCaptureLimit(reader.ReadByte());

								int intelFlags = reader.ReadByte();
								mt1.hasIntel = (intelFlags & 1) != 0;
								mt2.hasIntel = (intelFlags & 2) != 0;

								if (mt2.hasIntel) {
									mt1.carrier = reader.ReadByte();
									reader.ReadData(11);
								} else {
									mt1.flagPos.x = reader.ReadFloat();
									mt1.flagPos.y = reader.ReadFloat();
									mt1.flagPos.z = reader.ReadFloat();
								}

								if (mt1.hasIntel) {
									mt2.carrier = reader.ReadByte();
									reader.ReadData(11);
								} else {
									mt2.flagPos.x = reader.ReadFloat();
									mt2.flagPos.y = reader.ReadFloat();
									mt2.flagPos.z = reader.ReadFloat();
								}

								mt1.basePos.x = reader.ReadFloat();
								mt1.basePos.y = reader.ReadFloat();
								mt1.basePos.z = reader.ReadFloat();

								mt2.basePos.x = reader.ReadFloat();
								mt2.basePos.y = reader.ReadFloat();
								mt2.basePos.z = reader.ReadFloat();

								GetWorld()->SetMode(mode);
							} catch (...) {
								delete mode;
								throw;
							}
						} else {
							// TC
							TCGameMode *mode = new TCGameMode(GetWorld());
							try {
								int numTer = reader.ReadByte();

								for (int i = 0; i < numTer; i++) {
									TCGameMode::Territory ter;
									ter.pos.x = reader.ReadFloat();
									ter.pos.y = reader.ReadFloat();
									ter.pos.z = reader.ReadFloat();

									int state = reader.ReadByte();
									ter.ownerTeamId = state;
									ter.progressBasePos = 0.f;
									ter.progressStartTime = 0.f;
									ter.progressRate = 0.f;
									ter.capturingTeamId = -1;
									ter.mode = mode;
									mode->AddTerritory(ter);
								}

								GetWorld()->SetMode(mode);
							} catch (...) {
								delete mode;
								throw;
							}
						}
						client->JoinedGame();
					}
					break;
				case PacketTypeKillAction: {
					Player *p = GetPlayer(reader.ReadByte());
					Player *killer = GetPlayer(reader.ReadByte());
					int kt = reader.ReadByte();
					KillType type;
					switch (kt) {
						case 0: type = KillTypeWeapon; break;
						case 1: type = KillTypeHeadshot; break;
						case 2: type = KillTypeMelee; break;
						case 3: type = KillTypeGrenade; break;
						case 4: type = KillTypeFall; break;
						case 5: type = KillTypeTeamChange; break;
						case 6: type = KillTypeClassChange; break;
						default: SPInvalidEnum("kt", kt);
					}

					int respawnTime = reader.ReadByte();
					switch (type) {
						case KillTypeFall:
						case KillTypeClassChange:
						case KillTypeTeamChange: killer = p; break;
						default: break;
					}
					p->KilledBy(type, killer, respawnTime);
					if (p != killer) {
						GetWorld()->GetPlayerPersistent(killer->GetId()).kills += 1;
					}
				} break;
				case PacketTypeChatMessage: {
					// might be wrong player id for server message
					uint8_t pId = reader.ReadByte();
					Player *p = GetPlayerOrNull(pId);
					int type = reader.ReadByte();
					std::string txt = reader.ReadRemainingString();
					if (p) {
						switch (type) {
							case 0: // all
								client->PlayerSentChatMessage(p, true, txt);
								break;
							case 1: // team
								client->PlayerSentChatMessage(p, false, txt);
								break;
							case 2: // system???
								client->ServerSentMessage(txt);
						}
					} else {
						client->ServerSentMessage(txt);

						// Speculate the best game properties based on the server generated
						// messages
						properties->HandleServerMessage(txt);
					}
				} break;
				case PacketTypeMapStart: {
					// next map!
					client->SetWorld(NULL);
					mapSize = reader.ReadInt();
					status = NetClientStatusReceivingMap;
					statusString = _Tr("NetClient", "Loading snapshot");
				} break;
				case PacketTypeMapChunk: SPRaise("Unexpected: received Map Chunk while game");
				case PacketTypePlayerLeft: {
					Player *p = GetPlayer(reader.ReadByte());

					client->PlayerLeaving(p);
					GetWorld()->GetPlayerPersistent(p->GetId()).kills = 0;

					savedPlayerTeam[p->GetId()] = -1;
					playerPosRecords[p->GetId()].valid = false;
					GetWorld()->SetPlayer(p->GetId(), NULL);
					// TODO: message
				} break;
				case PacketTypeTerritoryCapture: {
					int territoryId = reader.ReadByte();
					bool winning = reader.ReadByte() != 0;
					int state = reader.ReadByte();

					IGameMode *mode = GetWorld()->GetMode();
					if (mode == NULL)
						break;
					if (mode->ModeType() != IGameMode::m_TC) {
						SPRaise("Received PacketTypeTerritoryCapture in non-TC gamemode");
					}
					TCGameMode *tc = static_cast<TCGameMode *>(mode);

					if (territoryId >= tc->GetNumTerritories()) {
						SPRaise("Invalid territory id %d specified (max = %d)", territoryId,
						        tc->GetNumTerritories() - 1);
					}

					client->TeamCapturedTerritory(state, territoryId);

					TCGameMode::Territory *t = tc->GetTerritory(territoryId);

					t->ownerTeamId = state;
					t->progressBasePos = 0.f;
					t->progressRate = 0.f;
					t->progressStartTime = 0.f;
					t->capturingTeamId = -1;

					if (winning)
						client->TeamWon(state);
				} break;
				case PacketTypeProgressBar: {
					int territoryId = reader.ReadByte();
					int capturingTeam = reader.ReadByte();
					int rate = (int8_t)reader.ReadByte();
					float progress = reader.ReadFloat();

					IGameMode *mode = GetWorld()->GetMode();
					if (mode == NULL)
						break;
					if (mode->ModeType() != IGameMode::m_TC) {
						SPRaise("Received PacketTypeProgressBar in non-TC gamemode");
					}
					TCGameMode *tc = static_cast<TCGameMode *>(mode);

					if (territoryId >= tc->GetNumTerritories()) {
						SPRaise("Invalid territory id %d specified (max = %d)", territoryId,
						        tc->GetNumTerritories() - 1);
					}

					if (progress < -0.1f || progress > 1.1f)
						SPRaise("Progress value out of range(%f)", progress);

					TCGameMode::Territory *t = tc->GetTerritory(territoryId);

					t->progressBasePos = progress;
					t->progressRate = (float)rate * TC_CAPTURE_RATE;
					t->progressStartTime = GetWorld()->GetTime();
					t->capturingTeamId = capturingTeam;
				} break;
				case PacketTypeIntelCapture: {
					if (!GetWorld())
						SPRaise("No world");
					IGameMode *mode = GetWorld()->GetMode();
					if (mode == NULL)
						break;
					if (mode->ModeType() != IGameMode::m_CTF) {
						SPRaise("Received PacketTypeIntelCapture in non-TC gamemode");
					}
					CTFGameMode *ctf = static_cast<CTFGameMode *>(mode);
					Player *p = GetPlayer(reader.ReadByte());
					client->PlayerCapturedIntel(p);
					GetWorld()->GetPlayerPersistent(p->GetId()).kills += 10;
					ctf->GetTeam(p->GetTeamId()).hasIntel = false;
					ctf->GetTeam(p->GetTeamId()).score++;

					bool winning = reader.ReadByte() != 0;
					if (winning)
						client->TeamWon(p->GetTeamId());
				} break;
				case PacketTypeIntelPickup: {
					Player *p = GetPlayer(reader.ReadByte());
					IGameMode *mode = GetWorld()->GetMode();
					if (mode == NULL)
						break;
					if (mode->ModeType() != IGameMode::m_CTF) {
						SPRaise("Received PacketTypeIntelPickup in non-TC gamemode");
					}
					CTFGameMode *ctf = static_cast<CTFGameMode *>(mode);
					CTFGameMode::Team &team = ctf->GetTeam(p->GetTeamId());
					team.hasIntel = true;
					team.carrier = p->GetId();
					client->PlayerPickedIntel(p);
				} break;
				case PacketTypeIntelDrop: {
					Player *p = GetPlayer(reader.ReadByte());
					IGameMode *mode = GetWorld()->GetMode();
					if (mode == NULL)
						break;
					if (mode->ModeType() != IGameMode::m_CTF) {
						SPRaise("Received PacketTypeIntelPickup in non-TC gamemode");
					}
					CTFGameMode *ctf = static_cast<CTFGameMode *>(mode);
					CTFGameMode::Team &team = ctf->GetTeam(p->GetTeamId());
					team.hasIntel = false;

					Vector3 pos;
					pos.x = reader.ReadFloat();
					pos.y = reader.ReadFloat();
					pos.z = reader.ReadFloat();

					ctf->GetTeam(1 - p->GetTeamId()).flagPos = pos;

					client->PlayerDropIntel(p);
				} break;
				case PacketTypeRestock: {
					Player *p = GetLocalPlayer(); // GetPlayer(reader.ReadByte());
					p->Restock();
				} break;
				case PacketTypeFogColour: {
					if (GetWorld()) {
						reader.ReadByte(); // skip
						GetWorld()->SetFogColor(reader.ReadIntColor());
					}
				} break;
				case PacketTypeWeaponReload: {
					Player *p = GetPlayer(reader.ReadByte());
					if (p != GetLocalPlayerOrNull())
						p->Reload();
					else {
						int clip = reader.ReadByte();
						int reserve = reader.ReadByte();
						if (clip < 255 && reserve < 255 && p) {
							p->ReloadDone(clip, reserve);
						}
					}
					// FIXME: use of "clip ammo" and "reserve ammo"?
				} break;
				case PacketTypeChangeTeam: {
					Player *p = GetPlayer(reader.ReadByte());
					int team = reader.ReadByte();
					if (team < 0 || team > 2)
						SPRaise("Received invalid team: %d", team);
					p->SetTeam(team);
				}
				case PacketTypeChangeWeapon: {
					reader.ReadByte();
					WeaponType wType;
					int weapon = reader.ReadByte();
					switch (weapon) {
						case 0: wType = RIFLE_WEAPON; break;
						case 1: wType = SMG_WEAPON; break;
						case 2: wType = SHOTGUN_WEAPON; break;
						default: SPRaise("Received invalid weapon: %d", weapon);
					}
					// maybe this command is intended to change local player's
					// weapon...
					// p->SetWeaponType(wType);
				} break;
				default:
					printf("WARNING: dropped packet %d\n", (int)reader.GetType());
					reader.DumpDebug();
			}
		}

		void NetClient::SendVersionEnhanced(const std::set<std::uint8_t> &propertyIds) {
			NetPacketWriter wri(PacketTypeExistingPlayer);
			wri.Write((uint8_t)'x');

			for (std::uint8_t propertyId : propertyIds) {
				wri.Write(propertyId);

				auto lengthLabel = wri.GetPosition();
				wri.Write((uint8_t)0); // dummy data for "Payload Length"

				auto beginLabel = wri.GetPosition();
				switch (static_cast<VersionInfoPropertyId>(propertyId)) {
					case VersionInfoPropertyId::ApplicationNameAndVersion:
						wri.Write((uint8_t)OpenSpades_VERSION_MAJOR);
						wri.Write((uint8_t)OpenSpades_VERSION_MINOR);
						wri.Write((uint8_t)OpenSpades_VERSION_REVISION);
						wri.Write("OpenSpades");
						break;
					case VersionInfoPropertyId::UserLocale:
						wri.Write(GetCurrentLocaleAndRegion());
						break;
					case VersionInfoPropertyId::ClientFeatureFlags1: {
						auto flags = ClientFeatureFlags1::None;

						if (cg_unicode) {
							flags |= ClientFeatureFlags1::SupportsUnicode;
						}

						wri.Write(static_cast<uint32_t>(flags));
					} break;
					default:
						// Just return empty payload for an unrecognized property
						break;
				}

				wri.Update(lengthLabel, (uint8_t)(wri.GetPosition() - beginLabel));
			}
		}

		void NetClient::SendJoin(int team, WeaponType weapType, std::string name, int kills) {
			SPADES_MARK_FUNCTION();
			int weapId;
			switch (weapType) {
				case RIFLE_WEAPON: weapId = 0; break;
				case SMG_WEAPON: weapId = 1; break;
				case SHOTGUN_WEAPON: weapId = 2; break;
				default: SPInvalidEnum("weapType", weapType);
			}

			NetPacketWriter wri(PacketTypeExistingPlayer);
			wri.Write((uint8_t)GetWorld()->GetLocalPlayerIndex());
			wri.Write((uint8_t)team);
			wri.Write((uint8_t)weapId);
			wri.Write((uint8_t)2); // TODO: change tool
			wri.Write((uint32_t)kills);
			wri.WriteColor(GetWorld()->GetTeam(team).color);
			wri.Write(name, 16);
			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendPosition() {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypePositionData);
			// wri.Write((uint8_t)pId);
			Player *p = GetLocalPlayer();
			Vector3 v = p->GetPosition();
			wri.Write(v.x);
			wri.Write(v.y);
			wri.Write(v.z);
			enet_peer_send(peer, 0, wri.CreatePacket());
			// printf("> (%f %f %f)\n", v.x, v.y, v.z);
		}

		void NetClient::SendOrientation(spades::Vector3 v) {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeOrientationData);
			// wri.Write((uint8_t)pId);
			wri.Write(v.x);
			wri.Write(v.y);
			wri.Write(v.z);
			enet_peer_send(peer, 0, wri.CreatePacket());
			// printf("> (%f %f %f)\n", v.x, v.y, v.z);
		}

		void NetClient::SendPlayerInput(PlayerInput inp) {
			SPADES_MARK_FUNCTION();

			uint8_t bits = 0;
			if (inp.moveForward)
				bits |= 1 << 0;
			if (inp.moveBackward)
				bits |= 1 << 1;
			if (inp.moveLeft)
				bits |= 1 << 2;
			if (inp.moveRight)
				bits |= 1 << 3;
			if (inp.jump)
				bits |= 1 << 4;
			if (inp.crouch)
				bits |= 1 << 5;
			if (inp.sneak)
				bits |= 1 << 6;
			if (inp.sprint)
				bits |= 1 << 7;

			if ((unsigned int)bits == lastPlayerInput)
				return;
			lastPlayerInput = bits;

			NetPacketWriter wri(PacketTypeInputData);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());
			wri.Write(bits);

			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendWeaponInput(WeaponInput inp) {
			SPADES_MARK_FUNCTION();
			uint8_t bits = 0;
			if (inp.primary)
				bits |= 1 << 0;
			if (inp.secondary)
				bits |= 1 << 1;

			if ((unsigned int)bits == lastWeaponInput)
				return;
			lastWeaponInput = bits;

			NetPacketWriter wri(PacketTypeWeaponInput);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());
			wri.Write(bits);

			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendBlockAction(spades::IntVector3 v, BlockActionType type) {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeBlockAction);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());

			switch (type) {
				case BlockActionCreate: wri.Write((uint8_t)0); break;
				case BlockActionTool: wri.Write((uint8_t)1); break;
				case BlockActionDig: wri.Write((uint8_t)2); break;
				case BlockActionGrenade: wri.Write((uint8_t)3); break;
				default: SPInvalidEnum("type", type);
			}

			wri.Write((uint32_t)v.x);
			wri.Write((uint32_t)v.y);
			wri.Write((uint32_t)v.z);

			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendBlockLine(spades::IntVector3 v1, spades::IntVector3 v2) {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeBlockLine);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());

			wri.Write((uint32_t)v1.x);
			wri.Write((uint32_t)v1.y);
			wri.Write((uint32_t)v1.z);
			wri.Write((uint32_t)v2.x);
			wri.Write((uint32_t)v2.y);
			wri.Write((uint32_t)v2.z);

			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendReload() {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeWeaponReload);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());

			// these value should be 255, or
			// NetClient will think reload was done when
			// it receives echoed WeaponReload packet
			wri.Write((uint8_t)255); // clip_ammo; not used?
			wri.Write((uint8_t)255); // reserve_ammo; not used?

			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendHeldBlockColor() {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeSetColour);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());
			IntVector3 v = GetLocalPlayer()->GetBlockColor();
			wri.WriteColor(v);
			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendTool() {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeSetTool);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());
			switch (GetLocalPlayer()->GetTool()) {
				case Player::ToolSpade: wri.Write((uint8_t)0); break;
				case Player::ToolBlock: wri.Write((uint8_t)1); break;
				case Player::ToolWeapon: wri.Write((uint8_t)2); break;
				case Player::ToolGrenade: wri.Write((uint8_t)3); break;
				default: SPInvalidEnum("tool", GetLocalPlayer()->GetTool());
			}

			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendGrenade(spades::client::Grenade *g) {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeGrenadePacket);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());

			wri.Write(g->GetFuse());

			Vector3 v = g->GetPosition();
			wri.Write(v.x);
			wri.Write(v.y);
			wri.Write(v.z);

			v = g->GetVelocity();
			wri.Write(v.x);
			wri.Write(v.y);
			wri.Write(v.z);
			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendHit(int targetPlayerId, HitType type) {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeHitPacket);
			wri.Write((uint8_t)targetPlayerId);

			switch (type) {
				case HitTypeTorso: wri.Write((uint8_t)0); break;
				case HitTypeHead: wri.Write((uint8_t)1); break;
				case HitTypeArms: wri.Write((uint8_t)2); break;
				case HitTypeLegs: wri.Write((uint8_t)3); break;
				case HitTypeMelee: wri.Write((uint8_t)4); break;
				default: SPInvalidEnum("type", type);
			}
			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendChat(std::string text, bool global) {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeChatMessage);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());
			wri.Write((uint8_t)(global ? 0 : 1));
			wri.Write(text);
			wri.Write((uint8_t)0);
			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendWeaponChange(WeaponType wt) {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeChangeWeapon);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());
			switch (wt) {
				case RIFLE_WEAPON: wri.Write((uint8_t)0); break;
				case SMG_WEAPON: wri.Write((uint8_t)1); break;
				case SHOTGUN_WEAPON: wri.Write((uint8_t)2); break;
			}
			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendTeamChange(int team) {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeChangeTeam);
			wri.Write((uint8_t)GetLocalPlayer()->GetId());
			wri.Write((uint8_t)team);
			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendHandShakeValid(int challenge) {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeHandShakeReturn);
			wri.Write((uint32_t)challenge);
			SPLog("Sending hand shake back.");
			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::SendVersion() {
			SPADES_MARK_FUNCTION();
			NetPacketWriter wri(PacketTypeVersionSend);
			wri.Write((uint8_t)'o');
			wri.Write((uint8_t)OpenSpades_VERSION_MAJOR);
			wri.Write((uint8_t)OpenSpades_VERSION_MINOR);
			wri.Write((uint8_t)OpenSpades_VERSION_REVISION);
			wri.Write(VersionInfo::GetVersionInfo());
			SPLog("Sending version back.");
			enet_peer_send(peer, 0, wri.CreatePacket());
		}

		void NetClient::MapLoaded() {
			SPADES_MARK_FUNCTION();
			MemoryStream compressed(mapData.data(), mapData.size());
			DeflateStream inflate(&compressed, CompressModeDecompress, false);
			GameMap *map;
			map = GameMap::Load(&inflate);

			SPLog("Map decoding succeeded.");

			// now initialize world
			World *w = new World(properties);
			w->SetMap(map);
			map->Release();
			SPLog("World initialized.");

			client->SetWorld(w);

			mapData.clear();

			SPAssert(GetWorld());

			SPLog("World loaded. Processing saved packets (%d)...", (int)savedPackets.size());

			for (size_t i = 0; i < playerPosRecords.size(); i++)
				playerPosRecords[i].valid = false;
			std::fill(savedPlayerTeam.begin(), savedPlayerTeam.end(), -1);

			// do saved packets
			try {
				for (size_t i = 0; i < savedPackets.size(); i++) {
					NetPacketReader r(savedPackets[i]);
					HandleGamePacket(r);
				}
				savedPackets.clear();
				SPLog("Done.");
			} catch (...) {
				savedPackets.clear();
				throw;
			}
		}

		NetClient::BandwidthMonitor::BandwidthMonitor(ENetHost *host)
		    : host(host), lastDown(0.0), lastUp(0.0) {
			sw.Reset();
		}

		void NetClient::BandwidthMonitor::Update() {
			if (sw.GetTime() > 0.5) {
				lastUp = host->totalSentData / sw.GetTime();
				lastDown = host->totalReceivedData / sw.GetTime();
				host->totalSentData = 0;
				host->totalReceivedData = 0;
				sw.Reset();
			}
		}
	}
}
