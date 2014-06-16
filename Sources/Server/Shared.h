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

#include <vector>
#include <Core/Debug.h>
#include <Core/TMPUtils.h>
#include <memory>
#include <string>
#include <map>
#include <Game/Constants.h>
#include <cstdint>

namespace spades { namespace protocol {
	
	static const uint32_t connectMagic = 0x8101919;
	
	// disconnect reason code.
	// when possible, server sends the reason text, and
	// use DisconnectReason::Misc to disconnect the peer.
	enum class DisconnectReason {
		Unknown = 0,
		InternalServerError = 1,
		ServerFull = 2,
		Misc = 3, // reason is already sent
		ServerStopped = 4,
		Timeout = 5,
		MalformedPacket = 6,
		ProtocolMismatch = 7
	};
	
	enum class PacketType {
		// session
		Greeting = 1,
		InitiateConnection = 2,
		ServerCertificate = 3,
		ClientCertificate = 4,
		Kick = 5,
		
		// game state loader
		GameStateHeader = 6,
		MapData = 7,
		GameStateFinal = 8,
		MapDataAcknowledge = 10,
		MapDataFinal = 11,
		
		// generic
		GenericCommand = 9,
		
		// updates
		EntityUpdate = 30,
		EntityEvent = 31,
		EntityDie = 32,
		EntityRemove = 33,
		ClientSideEntityUpdate = 35,
		
		TerrainUpdate = 40,
		
		PlayerUpdate = 50,
		PlayerRemove = 51,
		
		// actions
		PlayerAction = 60,
		// TODO: ride weapon, use deployed weapon
		
		// interractions
		HitEntity = 90,
		HitTerrain = 91,
		Damage = 92,
		// TODO: playerstate
		// TODO: damage notify
		// TODO: chat
		// TODO: construction
	};
	
	enum class PacketUsage {
		ServerOnly,
		ClientOnly,
		ServerAndClient
	};
	
	using game::EntityType;
	using game::EntityFlags;
	using game::PlayerFlags;
	using game::EntityDeathType;
	using game::EntityEventType;
	using game::BlockCreateType;
	using game::BlockDestroyType;
	using game::TrajectoryType;
	using game::Trajectory;
	using game::PlayerInput;
	using game::ToolSlot;
	using game::DamageType;
	using game::HitType;
	using game::SkinId;
	using game::DamageInfo;
	using game::MapEdit;
	using game::WeaponParameters;
	using game::WeaponType;
	using TimeStampType = std::uint64_t;
	
	class GreetingPacket;
	class InitiateConnectionPacket;
	class ServerCertificatePacket;
	class ClientCertificatePacket;
	class KickPacket;
	class GameStateHeaderPacket;
	class MapDataPacket;
	class GameStateFinalPacket;
	class GenericCommandPacket;
	class EntityUpdatePacket;
	class EntityDiePacket;
	class ClientSideEntityUpdatePacket;
	class TerrainUpdatePacket;
	class EntityEventPacket;
	class PlayerRemovePacket;
	class PlayerUpdatePacket;
	class PlayerActionPacket;
	class HitEntityPacket;
	class HitTerrainPacket;
	class EntityRemovePacket;
	class DamagePacket;
	class MapDataAcknowledgePacket;
	class MapDataFinalPacket;
	
	static const char *ProtocolName = "WorldOfSpades 0.1";
	
	using PacketClassList = stmp::make_type_list
	<
	GreetingPacket,
	InitiateConnectionPacket,
	ServerCertificatePacket,
	ClientCertificatePacket,
	KickPacket,
	
	GameStateHeaderPacket,
	MapDataPacket,
	GameStateFinalPacket,
	MapDataAcknowledgePacket,
	MapDataFinalPacket,
	
	GenericCommandPacket,
	
	EntityUpdatePacket,
	ClientSideEntityUpdatePacket,
	TerrainUpdatePacket,
	EntityEventPacket,
	EntityDiePacket,
	EntityRemovePacket,
	PlayerRemovePacket,
	PlayerUpdatePacket,
	
	PlayerActionPacket,
	
	HitEntityPacket,
	HitTerrainPacket,
	DamagePacket
	>::list;
	
	class PacketVisitor : public stmp::visitor_generator<PacketClassList> {
	public:
		virtual ~PacketVisitor() {}
	};
	
	class ConstPacketVisitor : public stmp::const_visitor_generator<PacketClassList> {
	public:
		virtual ~ConstPacketVisitor() {}
	};
	
	class Packet {
	public:
		virtual ~Packet() {}
		virtual void Accept(PacketVisitor&) = 0;
		virtual void Accept(ConstPacketVisitor& visitor) const = 0;
		virtual std::vector<char> Generate() const = 0;
		virtual PacketUsage GetUsage() = 0;
		bool CanServerSend() { return GetUsage() != PacketUsage::ClientOnly; }
		bool CanClientSend() { return GetUsage() != PacketUsage::ServerOnly; }
		virtual PacketType GetType() = 0;
		static Packet *Decode(const std::vector<char>&);
	};
	
	template<class T, PacketUsage usage, PacketType type>
	class BasePacket : public Packet {
		// make
		//   class T : public BasePacket<S, ...>
		// where T != S
		friend T;
		BasePacket() {}
	public:
		
		// compile-time constants
		static const bool IsServerPacket = usage != PacketUsage::ClientOnly;
		static const bool IsClientPacket = usage != PacketUsage::ServerOnly;
		static const PacketUsage Usage = usage;
		static const PacketType Type = type;
		static const unsigned int TypeId = static_cast<unsigned int>(type);
		
		virtual void Accept(PacketVisitor& visitor) {
			SPADES_MARK_FUNCTION();
			visitor.visit(static_cast<T&>(*this));
		}
		virtual void Accept(ConstPacketVisitor& visitor) const {
			SPADES_MARK_FUNCTION();
			visitor.visit(static_cast<const T&>(*this));
		}
		virtual PacketUsage GetUsage() { return usage; }
		virtual PacketType GetType() { return type; }
		
	};
	
	struct EntityCreateItem {
		EntityType type;
		
		std::array<std::string, 3> weaponSkins;
		std::array<WeaponParameters, 3> weaponParams;
		std::string bodySkin;
		
	};
	
	struct EntityUpdateItem {
		uint32_t entityId;
		
		stmp::optional<EntityCreateItem> createItem;
		
		stmp::optional<EntityFlags> flags;
		stmp::optional<Trajectory> trajectory;
		stmp::optional<uint8_t> health;
		
		stmp::optional<PlayerInput> playerInput;
		stmp::optional<ToolSlot> tool;
		stmp::optional<IntVector3> blockColor;
	};
	
	struct PlayerCreateItem {
		std::string name;
	};
	
	struct PlayerUpdateItem {
		uint32_t playerId;
		
		stmp::optional<PlayerCreateItem> createItem;
		
		stmp::optional<PlayerFlags> flags;
		stmp::optional<uint32_t> score;
	};
	
	
	/** GreetingPacket is sent by server when a client connects the server,
	 * before any other packets. */
	class GreetingPacket : public BasePacket
	<GreetingPacket,
	PacketUsage::ServerOnly, PacketType::Greeting> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~GreetingPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::string nonce; // used in authentication
		
	};
	
	/** InitiateConnectionPacket is sent by client to initiate the conenction.
	 * When server receives this packet, verifies protocol name and rejects
	 * the client when protocol name doesn't match. */
	class InitiateConnectionPacket : public BasePacket
	<InitiateConnectionPacket,
	PacketUsage::ClientOnly, PacketType::InitiateConnection> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~InitiateConnectionPacket() {}
		
		static InitiateConnectionPacket CreateDefault();
		
		virtual std::vector<char> Generate() const;
		
		std::string protocolName;
		uint16_t majorVersion;
		uint16_t minorVersion;
		uint16_t revision;
		std::string packageString;
		std::string environmentString;
		std::string locale;
		std::string playerName;
		std::string nonce; // used in authentication
		uint8_t mapQuality;
	};
	
	/** After server receives InitiateConnectionPacket,
	 * sends ServerCertificatePacket. */
	class ServerCertificatePacket : public BasePacket
	<ServerCertificatePacket,
	PacketUsage::ServerOnly, PacketType::ServerCertificate> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~ServerCertificatePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		bool isValid; /** some servers don't use certificate */
		std::string certificate;
		std::string signature; /** session nonce signed with the server private key */
	};
	
	/** After client receive ServerCertificatePacket and authenticates the server,
	 * sends ClientCertificatePacket.
	 * Client might send right after InitiateConnectionPacket if client have no
	 * credentials. */
	class ClientCertificatePacket : public BasePacket
	<ClientCertificatePacket,
	PacketUsage::ClientOnly, PacketType::ClientCertificate> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~ClientCertificatePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		bool isValid; /** some clients don't use certificate */
		std::string certificate;
		std::string signature; /** session nonce signed with the client private key */
	};
	
	/** Sent when server kicks a client. */
	class KickPacket : public BasePacket
	<KickPacket,
	PacketUsage::ServerOnly, PacketType::Kick> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~KickPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::string reason;
	};
	
	/** During gamestate transfer procedure, the server sends 
	 * StatePacket before sending the GameMap data.
	 * Gamestate transfer procedure occurs right after server sends
	 * ClientCertificatePacket, new round starts, or world is reseted.
	 * */
	class GameStateHeaderPacket : public BasePacket
	<GameStateHeaderPacket,
	PacketUsage::ServerOnly, PacketType::GameStateHeader> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~GameStateHeaderPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::map<std::string, std::string> properties;
	};
	
	/** Fragment of map data. */
	class MapDataPacket : public BasePacket
	<MapDataPacket,
	PacketUsage::ServerOnly, PacketType::MapData> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~MapDataPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::string fragment;
	};
	
	/** Sent by server after all of map data were sent. */
	class MapDataFinalPacket : public BasePacket
	<MapDataFinalPacket,
	PacketUsage::ServerOnly, PacketType::MapDataFinal> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~MapDataFinalPacket() {}
		
		virtual std::vector<char> Generate() const;
	};
	
	
	struct EntityUpdateItem;
	/** Sent by server after all of game state were sent. */
	class GameStateFinalPacket : public BasePacket
	<GameStateFinalPacket,
	PacketUsage::ServerOnly, PacketType::GameStateFinal> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~GameStateFinalPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::map<std::string, std::string> properties;
		std::vector<EntityUpdateItem> items;
		std::vector<PlayerUpdateItem> players;
	};
	
	/** Sent by client to acknoledge decoding of map data. */
	class MapDataAcknowledgePacket : public BasePacket
	<MapDataAcknowledgePacket,
	PacketUsage::ClientOnly, PacketType::MapDataAcknowledge> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~MapDataAcknowledgePacket() {}
		
		virtual std::vector<char> Generate() const;
	};
	
	class GenericCommandPacket : public BasePacket
	<GenericCommandPacket,
	PacketUsage::ServerAndClient, PacketType::GenericCommand> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~GenericCommandPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::vector<std::string> parts;
	};
	
	
	/** Sent by server to notify the latest state of entity.
	 * Only updated parts are sent.
	 */
	class EntityUpdatePacket : public BasePacket
	<EntityUpdatePacket,
	PacketUsage::ServerOnly, PacketType::EntityUpdate> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~EntityUpdatePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::vector<EntityUpdateItem> items;
		
		/** Updates even if client owns the entities. */
		bool forced;
	};
	
	/** Sent by client to update the latest client-side state of entity.
	 * Only updated parts are sent. Updated entity must be relevant to the
	 * client's player.
	 * No new entities can be created.
	 */
	class ClientSideEntityUpdatePacket : public BasePacket
	<ClientSideEntityUpdatePacket,
	PacketUsage::ClientOnly, PacketType::ClientSideEntityUpdate> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~ClientSideEntityUpdatePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		TimeStampType timestamp;
		std::vector<EntityUpdateItem> items;
	};
	
	class TerrainUpdatePacket : public BasePacket
	<TerrainUpdatePacket,
	PacketUsage::ServerOnly, PacketType::TerrainUpdate> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~TerrainUpdatePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::vector<MapEdit> edits;
	};
	
	class EntityEventPacket : public BasePacket
	<EntityEventPacket,
	PacketUsage::ServerOnly, PacketType::EntityEvent> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~EntityEventPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		uint32_t entityId;
		EntityEventType type;
		uint64_t param;
	};
	
	class EntityDiePacket : public BasePacket
	<EntityDiePacket,
	PacketUsage::ServerOnly, PacketType::EntityDie> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~EntityDiePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		uint32_t entityId;
		EntityDeathType type;
		uint64_t param;
		
		stmp::optional<DamageInfo> damage;
	};
	
	class EntityRemovePacket : public BasePacket
	<EntityRemovePacket,
	PacketUsage::ServerOnly, PacketType::EntityRemove> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~EntityRemovePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		uint32_t entityId;
	};
	
	
	/** Sent by server to notify the latest state of player.
	 * Only updated parts are sent.
	 */
	class PlayerUpdatePacket : public BasePacket
	<PlayerUpdatePacket,
	PacketUsage::ServerOnly, PacketType::PlayerUpdate> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~PlayerUpdatePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::vector<PlayerUpdateItem> items;
	};
	
	
	/** Sent by server to notify some players have left.
	 * Only updated parts are sent.
	 */
	class PlayerRemovePacket : public BasePacket
	<PlayerRemovePacket,
	PacketUsage::ServerOnly, PacketType::PlayerRemove> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~PlayerRemovePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		std::vector<uint32_t> players;
	};
	
	
	class PlayerActionPacket : public BasePacket
	<PlayerActionPacket,
	PacketUsage::ClientOnly, PacketType::PlayerAction> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~PlayerActionPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		TimeStampType timestamp;
		EntityEventType type;
		uint64_t param;
	};
	
	
	class HitEntityPacket : public BasePacket
	<HitEntityPacket,
	PacketUsage::ClientOnly, PacketType::HitEntity> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~HitEntityPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		TimeStampType timestamp;
		uint32_t entityId;
		
		HitType type;
		DamageType damageType;
		
		Vector3 firePosition;
		Vector3 hitPosition;
	};
	
	class HitTerrainPacket : public BasePacket
	<HitTerrainPacket,
	PacketUsage::ClientOnly, PacketType::HitTerrain> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~HitTerrainPacket() {}
		
		virtual std::vector<char> Generate() const;
		
		TimeStampType timestamp;
		IntVector3 blockPosition;
		
		DamageType damageType;
		
		Vector3 firePosition;
		Vector3 hitPosition;
	};
	
	class DamagePacket : public BasePacket
	<DamagePacket,
	PacketUsage::ServerOnly, PacketType::Damage> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~DamagePacket() {}
		
		virtual std::vector<char> Generate() const;
		
		uint32_t entityId;
		DamageInfo damage;
		uint8_t amount;
	};
	
	// TODO: player state
	
} }

