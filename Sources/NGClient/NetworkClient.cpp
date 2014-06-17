/*
 Copyright (c) 2013 yvt
 
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
#include "NetworkClient.h"
#include "Host.h"
#include <random>
#include <Game/World.h>
#include <Core/Strings.h>
#include <Core/Thread.h>
#include <Core/Mutex.h>
#include <Core/AutoLocker.h>
#include <Core/IStream.h>
#include <Core/Math.h>
#include <Core/DeflateStream.h>
#include <list>
#include <thread>
#include <stdexcept>
#include <exception>
#include <Game/Entity.h>
#include <Game/AllEntities.h>
#include <Core/Settings.h>
#include <Game/Player.h>
#include "NetworkPlayer.h"

SPADES_SETTING(cg_mapQuality, "80");

namespace spades { namespace ngclient {
	
	namespace {
		std::random_device randomDevice;
		std::string GenerateNonce(std::size_t len) {
			SPADES_MARK_FUNCTION();
			
			std::string s;
			s.resize(len);
			
			std::uniform_int_distribution<> u(0, 255);
			
			for (auto& c: s) {
				c = static_cast<char>(u(randomDevice));
			}
			
			return s;
		}
	}
	
	class NetworkClient::MapLoader: Thread
	{
		Mutex mutex;
		using Block = std::vector<char>;
		
		std::list<Block> loadedBlocks;
		bool volatile completed = false;
		
		class Stream: public IStream {
			MapLoader& loader;
			void WaitForData() {
				SPADES_MARK_FUNCTION();
				
				while (true) {
					{
						AutoLocker lock(&loader.mutex);
						if (!loader.loadedBlocks.empty()) {
							return;
						}
						if (loader.completed) {
							return;
						}
					}
					std::this_thread::sleep_for
					(std::chrono::milliseconds(100));
				}
			}
			
			Block currentBlock;
			std::size_t currentBlockPos = 0;
		public:
			Stream(MapLoader& loader):
			loader(loader) { }
			int ReadByte() override {
				SPADES_MARK_FUNCTION();
				
				unsigned char c;
				if (Read(&c, 1) == 0) return -1;
				return c;
			}
			size_t Read(void *d, size_t bytes) override {
				SPADES_MARK_FUNCTION();
				
				char *bufout = reinterpret_cast<char *>(d);
				
				std::size_t requiredBytes = bytes;
				while (bytes == requiredBytes) {
					if (currentBlockPos == currentBlock.size()) {
						// fill the buffer
						WaitForData();
						
						AutoLocker lock(&loader.mutex);
						if (loader.loadedBlocks.empty()) {
							// no more data...
							break;
						}
						
						currentBlock = std::move(loader.loadedBlocks.front());
						loader.loadedBlocks.pop_front();
						currentBlockPos = 0;
					} else {
						auto copied = std::min
						((currentBlock.size()) - currentBlockPos,
						 bytes);
						std::memcpy(bufout, currentBlock.data() +
									currentBlockPos, copied);
						bytes -= copied;
						currentBlockPos += copied;
						bufout += copied;
					}
				}
			
				return requiredBytes - bytes;
			}
		};
		
		std::exception_ptr exptr;
		client::GameMap * volatile map = nullptr;
		float volatile progress = 0.f;
		Mutex mapMutex;
		
		void Run() override {
			SPADES_MARK_FUNCTION();
			
			try {
				Stream stream(*this);
				// TODO: progress notify
				auto *mp = client::GameMap::LoadNGMap(&stream, [&](float per) {
					progress = per;
				});
				if (!mp) {
					SPRaise("Null map loaded.");
				}
				AutoLocker lock(&mapMutex);
				map = mp;
			} catch (...) {
				exptr = std::current_exception();
				map = nullptr;
			}
		}
		
		std::size_t totalInputBytes = 0;
	public:
		MapLoader() { }
		
		~MapLoader() {
			SPADES_MARK_FUNCTION();
			
			// aborts loading process
			completed = true;
			Join();
			if (map) {
				map->Release();
			}
		}
		
		/** Returns loaded map, or null if it's still being loaded. */
		client::GameMap *GetMap() {
			SPADES_MARK_FUNCTION();
			
			AutoLocker lock(&mapMutex);
			if (exptr) {
				std::rethrow_exception(exptr);
			}
			return map;
		}
		
		float GetProgress() {
			SPADES_MARK_FUNCTION();
			
			AutoLocker lock(&mapMutex);
			return progress;
		}
		
		void Start() {
			SPADES_MARK_FUNCTION();
			
			Thread::Start();
		}
		
		void Load(const char *buffer, std::size_t length) {
			SPADES_MARK_FUNCTION();
			
			totalInputBytes += length;
			if (totalInputBytes > 128 * 1024 * 1024) {
				// too big!
				SPRaise("Map size limit reached (128MiB).");
			}
			
			Block block;
			block.resize(length);
			std::memcpy(block.data(), buffer, length);
			
			AutoLocker lock(&mutex);
			loadedBlocks.emplace_back(std::move(block));
		}
		
		void LoadEOF() {
			completed = true;
		}
		
		
	};
	
	NetworkClient::NetworkClient(const NetworkClientParams& params):
	params(params){
		SPADES_MARK_FUNCTION();
		
		host.reset(new Host());
		host->AddListener(this);
	}
	
	NetworkClient::~NetworkClient() {
		SPADES_MARK_FUNCTION();
		
		host->RemoveListener(this);
	}
	
	void NetworkClient::Update() {
		SPADES_MARK_FUNCTION();
		
		host->DoEvents();
		if (state == State::LoadingMap) {
			try {
				if (mapLoader->GetMap()) {
					// map loaded.
					state = State::LoadingGameState;
					
					protocol::MapDataAcknowledgePacket re;
					host->Send(re);
					
					progress = stmp::optional<float>();
				} else {
					progress = mapLoader->GetProgress();
				}
			} catch (const std::exception& ex) {
				SPLog("Error while decoding map: %s", ex.what());
				Kicked(_Tr("NetworkClient", "Error occured while processing a received "
						   "map data.\n\n{0}", ex.what()));
			}
		}
		
		if (player) {
			player->Update();
		}
	}
	
	void NetworkClient::Connect() {
		SPADES_MARK_FUNCTION();
		
		SPLog("Connecting to %s", params.address.asString().c_str());
		
		progress = stmp::optional<float>();
		progressMessage = _Tr("NetworkClient",
							  "Connecting to Server");
		
		try {
			host->Connect(params.address);
		} catch (const std::exception& ex) {
			SPLog("Fatal error in Host::Connect: %s", ex.what());
			auto s = _Tr("NetworkClient", "Fatal error occured while "
						 "starting a network connection:\n\n{0}", ex.what());
			for (auto *l: listeners)
				l->Disconnected(s);
		}
	}
	
	void NetworkClient::AddListener(NetworkClientListener *l) {
		SPADES_MARK_FUNCTION();
		
		SPAssert(l);
		listeners.insert(l);
	}
	
	void NetworkClient::RemoveListener(NetworkClientListener *l) {
		listeners.erase(l);
	}
	
	void NetworkClient::ConnectedToServer() {
		SPADES_MARK_FUNCTION();
		
		SPLog("Connected to server. Waiting for GreetingPacket...");
		state = State::WaitingForGreeting;
		progress = stmp::optional<float>();
		progressMessage = _Tr("NetworkClient",
							  "Waiting for Reply");
	}
	
	void NetworkClient::DisconnectedFromServer(protocol::DisconnectReason reason) {
		SPADES_MARK_FUNCTION();
		
		std::string str;
		SPLog("ENet-level disconnection.");
		if (state == State::NotConnected) {
			return;
		}
		switch (reason) {
			case protocol::DisconnectReason::InternalServerError:
				str = _Tr("NetworkClient", "Game server is having a technical problem.");
				break;
			case protocol::DisconnectReason::MalformedPacket:
				str = _Tr("NetworkClient", "There was a problem transfering a packet.");
				break;
			case protocol::DisconnectReason::Misc:
			case protocol::DisconnectReason::Unknown:
				str = _Tr("NetworkClient", "Unknown reason");
				break;
			case protocol::DisconnectReason::ServerFull:
				str = _Tr("NetworkClient", "Server is full.");
				break;
			case protocol::DisconnectReason::ServerStopped:
				str = _Tr("NetworkClient", "Game server has stopped.");
				break;
			case protocol::DisconnectReason::Timeout:
				str = _Tr("NetworkClient", "Connection timed out.");
				break;
			case protocol::DisconnectReason::ProtocolMismatch:
				str = _Tr("NetworkClient", "Protocol version mismatch.");
				break;
		}
		SPLog("Reason: %s", str.c_str());
		
		for (auto *l: listeners)
			l->Disconnected(str);
	}
	
#pragma mark - EntityUpdater
	class NetworkClient::EntityUpdater:
	public game::EntityVisitor {
		const protocol::EntityUpdateItem& item;
		bool forced;
		
		bool ShouldUpdateOwnedProperty(game::Entity& e) {
			return forced || !e.IsLocallyControlled();
		}
		
		void VisitCommon(game::Entity& e) {
			SPADES_MARK_FUNCTION();
			
			if (item.flags) {
				e.SetFlags(*item.flags);
			}
			if (item.trajectory && ShouldUpdateOwnedProperty(e)) {
				e.GetTrajectory() = *item.trajectory;
			}
			if (item.health) {
				e.SetHealth(*item.health);
			}
			
		}
		
	public:
		EntityUpdater(const protocol::EntityUpdateItem& item,
					  bool forced):
		item(item), forced(forced) { }
		
		
		void Visit(game::PlayerEntity& e) override {
			SPADES_MARK_FUNCTION();
			VisitCommon(e);
			if (item.blockColor) {
				e.SetBlockColor(*item.blockColor);
			}
			if (item.tool) {
				e.SetTool(*item.tool);
			}
			if (item.playerInput && ShouldUpdateOwnedProperty(e)) {
				e.SetPlayerInput(*item.playerInput);
			}
		}
		void Visit(game::GrenadeEntity& e) override {
			SPADES_MARK_FUNCTION();
			VisitCommon(e);
		}
		void Visit(game::RocketEntity& e) override {
			SPADES_MARK_FUNCTION();
			VisitCommon(e);
		}
		void Visit(game::CommandPostEntity& e) override {
			SPADES_MARK_FUNCTION();
			VisitCommon(e);
		}
		void Visit(game::FlagEntity& e) override {
			SPADES_MARK_FUNCTION();
			VisitCommon(e);
		}
		void Visit(game::CheckpointEntity& e) override {
			SPADES_MARK_FUNCTION();
			VisitCommon(e);
		}
		void Visit(game::VehicleEntity& e) override {
			SPADES_MARK_FUNCTION();
			VisitCommon(e);
		}
	};
	
#pragma mark - PacketVisitor
	class NetworkClient::PacketVisitor:
	public protocol::PacketVisitor {
		NetworkClient& c;
		static game::Entity *CreateEntity(game::World& w,
										  const protocol::EntityUpdateItem& upd) {
			SPADES_MARK_FUNCTION();
			
			const auto& cItem = *upd.createItem;
			auto type = cItem.type;
			
			game::Entity *e = nullptr;
			switch (type) {
				case game::EntityType::Player:
					{
						auto *pe = new game::PlayerEntity(w);
						e = pe;
						pe->SetBodySkin(cItem.bodySkin);
						for (int i = 0; i < 3; i++) {
							game::PlayerWeapon w;
							w.skin = cItem.weaponSkins[i];
							w.param = cItem.weaponParams[i];
							pe->SetWeapon(i, w);
						}
					}
					break;
				case game::EntityType::Checkpoint:
					e = new game::CheckpointEntity(w);
					break;
				case game::EntityType::CommandPost:
					e = new game::CommandPostEntity(w);
					break;
				case game::EntityType::Flag:
					e = new game::FlagEntity(w);
					break;
				case game::EntityType::Grenade:
					e = new game::GrenadeEntity(w);
					break;
				case game::EntityType::Rocket:
					e = new game::RocketEntity(w);
					break;
				case game::EntityType::Vehicle:
					e = new game::VehicleEntity(w);
					break;
			}
			if (e == nullptr) {
				SPRaise("Unknown entity type: %s",
						game::GetEntityTypeName(type).c_str());
			}
			return e;
		}
		
		void UpdatePlayer(game::Player& p,
						  const protocol::PlayerUpdateItem& u) {
			SPADES_MARK_FUNCTION();
			
			if (u.flags) {
				p.GetFlags() = *u.flags;
			}
			if (u.score) {
				p.SetScore(*u.score);
			}
		}
		
		void ApplyMapEdit(const std::vector<game::MapEdit>& edits) {
			SPADES_MARK_FUNCTION();
			SPNotImplemented();
		}
		
	public:
		PacketVisitor(NetworkClient& c): c(c) { }
		void visit(protocol::GreetingPacket& p) override {
			SPADES_MARK_FUNCTION();
			SPLog("Received GreetingPacket with %d byte(s) of nonce.",
				  (int)p.nonce.size());
			if (c.state == State::WaitingForGreeting) {
				c.progress = stmp::optional<float>();
				c.progressMessage = _Tr("NetworkClient",
										"Authenticating");
				
				// TODO: reject too short nonce
				c.SendInitiateConnection(p.nonce);
				c.state = State::WaitingForServerCertificate;
			} else {
				SPRaise("Unexpected GreetingPacket.");
			}
		}
		void visit(protocol::InitiateConnectionPacket& p) override {
			SPADES_MARK_FUNCTION();
			SPRaise("Unexpected InitiateConnectionPacket.");
		}
		void visit(protocol::ServerCertificatePacket& p) override {
			SPADES_MARK_FUNCTION();
			SPLog("Received ServerCertificatePacket.");
			if (p.isValid) {
				SPLog("ServerCertificatePacket contains %d byte(s) of"
					  " digital cert and %d byte(s) of signature.",
					  (int)p.certificate.size(),
					  (int)p.signature.size());
			} else {
				SPLog("ServerCertificatePacket didn't have a digital cert.");
			}
			// TODO: handle and validate server certificate
			c.SendClientCertificate();
		}
		void visit(protocol::ClientCertificatePacket& p) override {
			SPADES_MARK_FUNCTION();
			SPRaise("Unexpected ClientCertificatePacket.");
		}
		void visit(protocol::KickPacket& p) override {
			SPADES_MARK_FUNCTION();
			SPLog("Received KickPacket: %s", p.reason.c_str());
			c.Kicked(p.reason);
		}
		void visit(protocol::GameStateHeaderPacket& p) override {
			SPADES_MARK_FUNCTION();
			SPLog("Received GameStateHeaderPacket.");
			
			// this packet might be sent during game to
			// start another round.
			if (c.state != State::WaitingForGreeting &&
				c.state != State::WaitingForServerCertificate &&
				c.state != State::NotConnected) {
				
				c.state = State::LoadingMap;
				c.savedMapEdits.clear();
				
				c.progress = 0.f;
				c.progressMessage = _Tr("NetworkClient",
										"Loading Game State");
				
				c.SetWorld(nullptr);
				
				// start loading map.
				c.mapLoader.reset(new MapLoader());
				c.mapLoader->Start();
			} else {
				SPRaise("Unexpected GameStateHeaderPacket.");
			}
		}
		void visit(protocol::MapDataPacket& p) override {
			SPADES_MARK_FUNCTION();
			if (c.state == State::LoadingMap) {
				SPAssert(c.mapLoader);
				c.mapLoader->Load(p.fragment.data(),
								  p.fragment.size());
			} else {
				SPRaise("Unexpected MapDataPacket.");
			}
		}
		void visit(protocol::MapDataFinalPacket& p) override {
			SPADES_MARK_FUNCTION();
			SPLog("Received MapDataFinalPacket.");
			if (c.state == State::LoadingMap) {
				SPAssert(c.mapLoader);
				c.mapLoader->LoadEOF();
			} else {
				SPRaise("Unexpected GameStateFinalPacket.");
			}
		}
		void visit(protocol::MapDataAcknowledgePacket& p) override {
			SPADES_MARK_FUNCTION();
			SPRaise("Unexpected MapDataAcknowledgePacket.");
		}
		void visit(protocol::GameStateFinalPacket& p) override {
			SPADES_MARK_FUNCTION();
			SPLog("Received GameStateFinalPacket.");
			SPLog("GameStateFinalPacket contains %d entity(ies).",
				  (int)p.items.size());
			if (c.state == State::LoadingGameState) {
				SPAssert(c.mapLoader);
				game::WorldParameters params;
				for (const auto& pair: p.properties) {
					SPLog("World property '%s' = '%s'",
						  pair.first.c_str(),
						  pair.second.c_str());
					params.Update(pair.first, pair.second);
				}
				Handle<game::World> world(new game::World(params,
														  c.mapLoader->GetMap(),
														  false), false);
				c.mapLoader.reset();
				
				for (const auto& item: p.items) {
					Handle<game::Entity> e;
					if (!item.createItem) {
						SPRaise("no creation info");
					}
					e.Set(CreateEntity(*world, item), false);
					SPAssert(e);
					
					EntityUpdater update(item, true);
					e->Accept(update);
					world->LinkEntity(e, item.entityId);
				}
				
				// apply saved map edits
				SPLog("Applying saved %d map edit(s)",
					  (int)c.savedMapEdits.size());
				for (const auto& seq: c.savedMapEdits) {
					ApplyMapEdit(seq);
				}
				c.savedMapEdits.clear();
				
				c.state = State::Game;
				c.SetWorld(world.Unmanage());
				
			} else {
				SPRaise("Unexpected GameStateFinalPacket.");
			}
		}
		void visit(protocol::GenericCommandPacket& p) override {
			SPADES_MARK_FUNCTION();
			c.HandleGenericCommand(p.parts);
		}
		void visit(protocol::EntityUpdatePacket& p) override {
			SPADES_MARK_FUNCTION();
			if (c.state == State::Game) {
				game::World *w = c.world;
				SPAssert(w);
				for (const auto& item: p.items) {
					Handle<game::Entity> e(c.world->FindEntity(item.entityId));
					if (!e) {
						// create one
						if (item.createItem) {
							e.Set(CreateEntity(*w, item), false);
						} else {
							SPLog("entity %u doesn't exist. ignored",
								  (unsigned int)item.entityId);
							continue;
						}
					}
					
					SPAssert(e);
					
					EntityUpdater update(item, p.forced || item.createItem);
					e->Accept(update);
					
					if (!e->GetId()) {
						w->LinkEntity(e, item.entityId);
					}
				}
			} else {
				// ignore
			}
		}
		void visit(protocol::PlayerUpdatePacket& p) override {
			SPADES_MARK_FUNCTION();
			if (c.state == State::Game) {
				game::World *w = c.world;
				SPAssert(w);
				for (const auto& item: p.items) {
					Handle<game::Player> e(c.world->FindPlayer(item.playerId));
					if (!e) {
						// create one
						if (item.createItem) {
							const auto& cItem = *item.createItem;
							e.Set(new game::Player(*w, cItem.name),
								  false);
						} else {
							SPLog("player ID %u doesn't exist. ignored",
								  (unsigned int)item.playerId);
							continue;
						}
					}
					
					SPAssert(e);
					
					UpdatePlayer(*e, item);
					
					if (!e->GetId()) {
						w->CreatePlayer(e, item.playerId);
					}
				}
			} else {
				// ignore
			}
		}
		
		void visit(protocol::PlayerRemovePacket& p) override {
			SPADES_MARK_FUNCTION();
			if (c.state == State::Game) {
				for (auto id: p.players) {
					auto *e = c.world->FindPlayer(id);
					if (e) {
						c.world->RemovePlayer(e);
					} else {
						SPLog("player %u doesn't exist. ignored",
							  (unsigned int)id);
					}
				}
			} else {
				// ignore
			}
		}
		void visit(protocol::ClientSideEntityUpdatePacket& p) override {
			SPADES_MARK_FUNCTION();
			SPRaise("Unexpected ClientSideEntityUpdatePacket.");
		}
		void visit(protocol::TerrainUpdatePacket& p) override {
			SPADES_MARK_FUNCTION();
			if (c.state == State::Game) {
				ApplyMapEdit(p.edits);
			} else if (c.state == State::LoadingMap ||
					   c.state == State::LoadingGameState) {
				for (auto& edit: p.edits) {
					edit.createType = game::BlockCreateType::Unspecified;
					edit.destroyType = game::BlockDestroyType::Unspecified;
				}
				c.savedMapEdits.emplace_back(std::move(p.edits));
			} else {
				SPRaise("Unexpected TerrainUpdatePacket.");
			}
		}
		void visit(protocol::EntityEventPacket& p) override {
			SPADES_MARK_FUNCTION();
			if (c.state == State::Game) {
				auto *e = c.world->FindEntity(p.entityId);
				if (e) {
					e->EventTriggered(p.type, p.param);
				} else {
					SPLog("entity %u doesn't exist. ignored",
						  (unsigned int)p.entityId);
				}
			} else {
				// ignore
			}
		}
		void visit(protocol::EntityDiePacket& p) override {
			SPADES_MARK_FUNCTION();
			SPNotImplemented();
		}
		void visit(protocol::EntityRemovePacket& p) override {
			SPADES_MARK_FUNCTION();
			if (c.state == State::Game) {
				auto *e = c.world->FindEntity(p.entityId);
				if (e) {
					c.world->UnlinkEntity(e);
				} else {
					SPLog("entity %u doesn't exist. ignored",
						  (unsigned int)p.entityId);
				}
			} else {
				// ignore
			}
		}
		void visit(protocol::PlayerActionPacket& p) override {
			SPADES_MARK_FUNCTION();
			SPRaise("Unexpected PlayerActionPacket.");
		}
		void visit(protocol::HitEntityPacket& p) override {
			SPADES_MARK_FUNCTION();
			SPRaise("Unexpected HitEntityPacket.");
		}
		void visit(protocol::HitTerrainPacket& p) override {
			SPADES_MARK_FUNCTION();
			SPRaise("Unexpected HitTerrainPacket.");
		}
		void visit(protocol::DamagePacket& p) override {
			SPADES_MARK_FUNCTION();
			if (c.state == State::Game) {
				auto *e = c.world->FindEntity(p.entityId);
				if (e) {
					e->Damaged(p.damage, p.amount);
				} else {
					SPLog("entity %u doesn't exist. ignored",
						  (unsigned int)p.entityId);
				}
			} else {
				// ignore
			}
		}
	};
	
	void NetworkClient::PacketReceived(protocol::Packet &p) {
		SPADES_MARK_FUNCTION();
		try {
			PacketVisitor visitor(*this);
			p.Accept(visitor);
		} catch (const std::exception& ex) {
			SPLog("Error while handling received packet: %s", ex.what());
			Kicked(_Tr("NetworkClient", "Error occured while processing a received "
					   "network packet.\n\n{0}", ex.what()));
		}
	}
	
	void NetworkClient::SendInitiateConnection(const std::string& serverNonce) {
		SPADES_MARK_FUNCTION();
		
		auto re = protocol::InitiateConnectionPacket::CreateDefault();
		re.nonce = GenerateNonce(256);
		re.playerName = params.playerName;
		re.mapQuality = (int)cg_mapQuality;
		this->nonce = serverNonce + re.nonce;
		host->Send(re);
		
		state = State::WaitingForServerCertificate;
	}
	
	void NetworkClient::SendClientCertificate() {
		SPADES_MARK_FUNCTION();
		
		// TODO: use certificate
		protocol::ClientCertificatePacket re;
		re.isValid = false;
		host->Send(re);
		
		state = State::WaitingForGameState;
	}
	
	void NetworkClient::Kicked(const std::string &reason) {
		SPADES_MARK_FUNCTION();
		
		state = State::NotConnected;
		host->Disconnect();
		for (auto *l: listeners)
			l->Disconnected(reason);
	}
	
	void NetworkClient::HandleGenericCommand(const std::vector<std::string> &parts) {
		SPADES_MARK_FUNCTION();
		
		if (!parts.empty()) {
			if (parts[0] == "local-player" &&
				state == State::Game) {
				if (parts.size() >= 2) {
					int pId = std::stoi(parts[1]);
					
					world->SetLocalPlayerId((uint32_t)pId);
				} else {
					world->SetLocalPlayerId(stmp::optional<uint32_t>());
				}
				
				auto *p = world->GetLocalPlayer();
				if (p)
					player.reset(new NetworkPlayer(*this, *p));
				else
					player.reset();
				return;
			}
		}
		
		std::vector<char> str;
		for (const auto& s: parts) {
			if (!str.empty())
				str.push_back(' ');
			str.insert(str.end(),
					   s.begin(), s.end());
		}
		str.push_back(0);
		
		SPLog("Unhandled generic command: '%s'",
			  str.data());
	}
	
	void NetworkClient::SetWorld(game::World *w) {
		SPADES_MARK_FUNCTION();
		
		player.reset();
		
		world.Set(w, true);
		
		for (auto *l: listeners)
			l->WorldChanged(world);
	}
	
	void NetworkClient::SendGenericCommand(const std::vector<std::string> &parts) {
		SPADES_MARK_FUNCTION();
		
		if (host) {
			protocol::GenericCommandPacket p;
			p.parts = parts;
			host->Send(p);
		}
	}
	
} }
