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
		InitiateConnection = 1
	};
	
	enum class PacketUsage {
		ServerOnly,
		ClientOnly,
		ServerAndClient
	};
	
	class InitiateConnectionPacket;
	
	static const char *ProtocolName = "WorldOfSpades 0.1";
	
	using PacketClassList = stmp::make_type_list
	<
	InitiateConnectionPacket
	>::list;
	
	class PacketVisitor : public stmp::visitor_generator<PacketClassList> {
	public:
	};
	
	class ConstPacketVisitor : public stmp::const_visitor_generator<PacketClassList> {
	public:
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
	};
	
	
	
} }

