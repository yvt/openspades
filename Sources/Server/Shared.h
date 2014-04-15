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
	
	// disconnect reason code.
	// when possible, server sends the reason text, and
	// use DisconnectReason::Misc to disconnect the peer.
	enum class DisconnectReason {
		Unknown = 0,
		InternalServerError = 1,
		ServerFull = 2,
		Misc = 3 // reason is already sent
	};
	
	enum class PacketType {
		Greeting = 1,
		InitiateConnection = 2,
		ServerCertificate = 3,
		ClientCertificate = 4,
		Kick = 5,
		GameStateHeader = 6,
		MapData = 7,
		GameStateFinal = 8,
		EntityUpdate = 20,
		ClientSideEntityUpdate = 21,
		JumpAction = 30,
		ReloadWeapon = 31
	};
	
	enum class PacketUsage {
		ServerOnly,
		ClientOnly,
		ServerAndClient
	};
	
	using game::EntityType;
	using game::EntityFlags;
	using game::TrajectoryType;
	using game::Trajectory;
	using game::PlayerInput;
	using game::ToolSlot;
	using game::SkinId;
	using TimeStampType = std::uint64_t;
	
	class GreetingPacket;
	class InitiateConnectionPacket;
	class ServerCertificatePacket;
	class ClientCertificatePacket;
	class KickPacket;
	class GameStateHeaderPacket;
	class MapDataPacket;
	class GameStateFinalPacket;
	class EntityUpdatePacket;
	class ClientSideEntityUpdatePacket;
	class JumpActionPacket;
	class ReloadWeaponPacket;
	
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
	EntityUpdatePacket,
	ClientSideEntityUpdatePacket,
	JumpActionPacket,
	ReloadWeaponPacket
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
		virtual std::vector<char> Generate() = 0;
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
	
	/** GreetingPacket is sent by server when a client connects the server,
	 * before any other packets. */
	class GreetingPacket : public BasePacket
	<GreetingPacket,
	PacketUsage::ServerOnly, PacketType::Greeting> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~GreetingPacket() {}
		
		virtual std::vector<char> Generate();
		
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
		
		virtual std::vector<char> Generate();
		
		std::string protocolName;
		uint16_t majorVersion;
		uint16_t minorVersion;
		uint16_t revision;
		std::string packageString;
		std::string environmentString;
		std::string locale;
		std::string playerName;
		std::string nonce; // used in authentication
	};
	
	/** After server receives InitiateConnectionPacket,
	 * sends ServerCertificatePacket. */
	class ServerCertificatePacket : public BasePacket
	<ServerCertificatePacket,
	PacketUsage::ServerOnly, PacketType::ServerCertificate> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~ServerCertificatePacket() {}
		
		virtual std::vector<char> Generate();
		
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
		
		virtual std::vector<char> Generate();
		
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
		
		virtual std::vector<char> Generate();
		
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
		
		virtual std::vector<char> Generate();
		
		std::map<std::string, std::string> properties;
	};
	
	/** Fragment of map data. */
	class MapDataPacket : public BasePacket
	<MapDataPacket,
	PacketUsage::ServerOnly, PacketType::MapData> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~MapDataPacket() {}
		
		virtual std::vector<char> Generate();
		
		std::string fragment;
	};
	
	/** Sent by server after all of game states were sent. */
	class GameStateFinalPacket : public BasePacket
	<GameStateFinalPacket,
	PacketUsage::ServerOnly, PacketType::GameStateFinal> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~GameStateFinalPacket() {}
		
		virtual std::vector<char> Generate();
		
		std::map<std::string, std::string> properties;
	};
	
	struct EntityUpdateItem {
		bool create;
		
		uint32_t entityId;
		
		// type is sent only for new entities
		EntityType type;
		
		bool includeFlags;
		EntityFlags flags;
		
		bool includeTrajectory;
		Trajectory trajectory;
		
		bool includePlayerInput;
		PlayerInput playerInput;
		
		bool includeTool;
		ToolSlot tool;
		
		bool includeBlockColor;
		IntVector3 blockColor;
		
		bool includeHealth;
		uint8_t health;
		
		bool includeSkin;
		std::string weaponSkin1;
		std::string weaponSkin2;
		std::string weaponSkin3;
		std::string bodySkin;
		
		
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
		
		virtual std::vector<char> Generate();
		
		std::vector<EntityUpdateItem> items;
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
		
		virtual std::vector<char> Generate();
		
		TimeStampType timestamp;
		std::vector<EntityUpdateItem> items;
	};
	
	class JumpActionPacket : public BasePacket
	<JumpActionPacket,
	PacketUsage::ClientOnly, PacketType::JumpAction> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~JumpActionPacket() {}
		
		virtual std::vector<char> Generate();
		
		TimeStampType timestamp;
	};
	
	class ReloadWeaponPacket : public BasePacket
	<ReloadWeaponPacket,
	PacketUsage::ClientOnly, PacketType::ReloadWeapon> {
	public:
		static Packet *Decode(const std::vector<char>&);
		virtual ~ReloadWeaponPacket() {}
		
		virtual std::vector<char> Generate();
		
		TimeStampType timestamp;
	};
	
	// TODO: player state
	
} }

