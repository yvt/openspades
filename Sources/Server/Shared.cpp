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
#include <Core/VersionInfo.h>
#include <OpenSpades.h>

namespace spades { namespace protocol {
	
	typedef Packet *(*PacketDecodeFuncType)(const std::vector<char>&);
	
	class PacketTypeFinder {
		PacketType type;
	public:
		constexpr PacketTypeFinder(PacketType type): type(type) {}
		template<class T> inline constexpr bool evaluate() const { return type == T::Type; }
	};
	class PacketTypeToDecoder {
	public:
		template<class T> inline constexpr PacketDecodeFuncType evaluate() const { return &T::Decode; }
		inline constexpr PacketDecodeFuncType not_found() const { return nullptr; }
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
		
		uint64_t ReadVariableInteger() {
			SPADES_MARK_FUNCTION();
			
			uint32_t v = 0;
			int shift = 0;
			while(true) {
				uint8_t b = ReadByte();
				v |= static_cast<uint32_t>(b & 0x7f) << shift;
				if(b & 0x80) {
					shift += 7;
				}else{
					break;
				}
			}
			return v;
		}
		
		std::string ReadString(){
			SPADES_MARK_FUNCTION();
			
			auto len = ReadVariableInteger();
			if(len > 1024 * 1024) {
				SPRaise("String too long.: %llu",
						static_cast<unsigned long long>(len));
			}
			
			// convert to C string once so that
			// null-chars are removed
			std::string s = ReadData(static_cast<std::size_t>(len)).c_str();
			return s;
		}
		
	};
	class PacketWriter: public NetPacketWriter {
	public:
		PacketWriter(PacketType type):
		NetPacketWriter(static_cast<unsigned int>(type)) {}
		
		void WriteVariableInteger(uint64_t i) {
			SPADES_MARK_FUNCTION();
			
			while(true) {
				uint8_t b = static_cast<uint8_t>(i & 0x7f);
				i >>= 7;
				if(i) {
					b |= 0x80;
					Write(b);
				}else{
					Write(b);
					break;
				}
			}
		}
		
		void WriteString(std::string str){
			SPADES_MARK_FUNCTION();
			
			WriteVariableInteger(str.size());
			Write(str);
		}
	};
	
	Packet *Packet::Decode(const std::vector<char>& data) {
		SPADES_MARK_FUNCTION();
		
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
	
	InitiateConnectionPacket InitiateConnectionPacket::CreateDefault() {
		SPADES_MARK_FUNCTION();
		
		InitiateConnectionPacket ret;
		ret.protocolName = ProtocolName;
		ret.majorVersion = OpenSpades_VERSION_MAJOR;
		ret.minorVersion = OpenSpades_VERSION_MINOR;
		ret.revision = OpenSpades_VERSION_REVISION;
		ret.packageString = PACKAGE_STRING;
		ret.environmentString = VersionInfo::GetVersionInfo();
		return ret;
	}
	
	Packet *InitiateConnectionPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<InitiateConnectionPacket> p(new InitiateConnectionPacket());
		PacketReader reader(data);
		
		p->protocolName = reader.ReadString();
		p->majorVersion = reader.ReadShort();
		p->minorVersion = reader.ReadShort();
		p->revision = reader.ReadShort();
		p->packageString = reader.ReadShort();
		p->environmentString = reader.ReadShort();
		
		return p.release();
	}
	
	std::vector<char> InitiateConnectionPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteString(protocolName);
		writer.Write(majorVersion);
		writer.Write(minorVersion);
		writer.Write(revision);
		writer.WriteString(packageString);
		writer.WriteString(environmentString);
		
		return std::move(writer.ToArray());
	}
	
} }
