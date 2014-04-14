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
#include <Core//Settings.h>

SPADES_SETTING(core_locale, "");

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
		
		std::string ReadBytes(){
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
		
		std::string ReadString(){
			return ReadBytes();
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
		
		void WriteBytes(std::string str){
			SPADES_MARK_FUNCTION();
			
			WriteVariableInteger(str.size());
			Write(str);
		}
		void WriteString(std::string str){
			WriteBytes(str);
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
	
	
	Packet *GreetingPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<GreetingPacket> p(new GreetingPacket());
		PacketReader reader(data);
		
		auto magic = reader.ReadString();
		if(magic != "Hello") {
			SPRaise("Invalid magic.");
		}
		p->nonce = reader.ReadBytes();
		
		return p.release();
	}
	
	std::vector<char> GreetingPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.WriteString("Hello");
		writer.WriteBytes(nonce);
		
		return std::move(writer.ToArray());
	}
	
	
	InitiateConnectionPacket InitiateConnectionPacket::CreateDefault() {
		SPADES_MARK_FUNCTION();
		
		InitiateConnectionPacket ret;
		ret.protocolName = ProtocolName;
		if(ret.protocolName.size() > 256) ret.protocolName.resize(256);
		ret.majorVersion = OpenSpades_VERSION_MAJOR;
		ret.minorVersion = OpenSpades_VERSION_MINOR;
		ret.revision = OpenSpades_VERSION_REVISION;
		ret.packageString = PACKAGE_STRING;
		if(ret.packageString.size() > 256) ret.packageString.resize(256);
		ret.environmentString = VersionInfo::GetVersionInfo();
		if(ret.environmentString.size() > 1024) ret.environmentString.resize(1024);
		ret.locale = static_cast<std::string>(core_locale);
		if(ret.locale.size() > 256) ret.locale.resize(256);
		
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
		p->packageString = reader.ReadString();
		p->environmentString = reader.ReadString();
		p->locale = reader.ReadString();
		p->nonce = reader.ReadBytes();
		
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
		writer.WriteString(locale);
		writer.WriteBytes(nonce);
		
		return std::move(writer.ToArray());
	}
	
	
	Packet *ServerCertificatePacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<ServerCertificatePacket> p(new ServerCertificatePacket());
		PacketReader reader(data);
		
		p->isValid = reader.ReadByte() != 0;
		
		if(p->isValid) {
			p->certificate = reader.ReadBytes();
			p->signature = reader.ReadBytes();
		}
		
		return p.release();
	}
	
	std::vector<char> ServerCertificatePacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.Write(static_cast<uint8_t>(isValid ? 1 : 0));
		if(isValid) {
			writer.WriteBytes(certificate);
			writer.WriteBytes(signature);
		}
			
		return std::move(writer.ToArray());
	}
	
	
	Packet *ClientCertificatePacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<ClientCertificatePacket> p(new ClientCertificatePacket());
		PacketReader reader(data);
		
		p->isValid = reader.ReadByte() != 0;
		
		if(p->isValid) {
			p->certificate = reader.ReadBytes();
			p->signature = reader.ReadBytes();
		}
		
		return p.release();
	}
	
	std::vector<char> ClientCertificatePacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		PacketWriter writer(Type);
		
		writer.Write(static_cast<uint8_t>(isValid ? 1 : 0));
		if(isValid) {
			writer.WriteBytes(certificate);
			writer.WriteBytes(signature);
		}
		
		return std::move(writer.ToArray());
	}

} }
