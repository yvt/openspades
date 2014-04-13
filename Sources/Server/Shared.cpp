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

#include "Shared.h"
#include <Core/ENetTools.h>
#include <Core/Debug.h>

namespace spades { namespace protocol {
	
	typedef Packet *(*PacketDecodeFuncType)(const std::vector<char>&);
	
	class PacketTypeFinder {
		PacketType type;
	public:
		constexpr PacketTypeFinder(PacketType type): type(type) {}
		template<class T> constexpr bool evaluate() const { return type == T::Type; }
	};
	class PacketTypeToDecoder {
	public:
		template<class T> constexpr PacketDecodeFuncType evaluate() const { return &T::Decode; }
		constexpr PacketDecodeFuncType not_found() const { return nullptr; }
	};
	
	
	class PacketDecodeTableGenerator {
	public:
		constexpr PacketDecodeFuncType operator [](std::size_t index) const {
			return stmp::find_type_list<PacketTypeFinder, PacketTypeToDecoder, PacketClassList>
			(PacketTypeFinder(static_cast<PacketType>(index)), PacketTypeToDecoder()).evaluate();
		}
	};
	
	static constexpr auto packetDecodeTable = stmp::make_static_table<128>(PacketDecodeTableGenerator());
	
	class PacketReader: public NetPacketReader {
	public:
		PacketReader(const std::vector<char>& bytes):
		NetPacketReader(bytes) {}
		
	};
	class PacketWriter: public NetPacketWriter {
	public:
		PacketWriter(PacketType type):
		NetPacketWriter(static_cast<unsigned int>(type)) {}
	};
	
	Packet *Packet::Decode(const std::vector<char>& data) {
		if(data.size() == 0) {
			SPRaise("Packet truncated");
		}
		
		auto typeIndex = static_cast<std::size_t>(data[0]);
		if(typeIndex >= packetDecodeTable.size()) {
			return nullptr;
		}
		
		auto *ptr = packetDecodeTable[typeIndex];
		if(ptr == nullptr) {
			return nullptr;
		}
		
		return ptr(data);
	}
	
	Packet *ConnectRequestPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<ConnectRequestPacket> p(new ConnectRequestPacket());
		PacketReader reader(data);
		
		SPNotImplemented();
		
		return p.release();
	}
	
	std::vector<char> ConnectRequestPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		SPNotImplemented();
		
		return std::move(writer.ToArray());
	}
	
} }
