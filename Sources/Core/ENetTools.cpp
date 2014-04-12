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

#include "ENetTools.h"
#include <Core/Debug.h>
#include <Core/Exception.h>

namespace spades {
	
#pragma mark - NetPacketReader
	
	NetPacketReader::NetPacketReader(ENetPacket *packet){
		SPADES_MARK_FUNCTION();
		
		data.resize(packet->dataLength);
		memcpy(data.data(), packet->data, packet->dataLength);
		enet_packet_destroy(packet);
		pos = 1;
	}
	
	NetPacketReader::NetPacketReader(const std::vector<char> inData){
		data = inData;
		pos = 1;
	}
	
	uint32_t NetPacketReader::ReadInt() {
		SPADES_MARK_FUNCTION();
		
		uint32_t value = 0;
		if(pos + 4 > data.size()){
			SPRaise("Received packet truncated");
		}
		value |= ((uint32_t)(uint8_t)data[pos++]);
		value |= ((uint32_t)(uint8_t)data[pos++]) << 8;
		value |= ((uint32_t)(uint8_t)data[pos++]) << 16;
		value |= ((uint32_t)(uint8_t)data[pos++]) << 24;
		return value;
	}
	uint16_t NetPacketReader::ReadShort() {
		SPADES_MARK_FUNCTION();
		
		uint32_t value = 0;
		if(pos + 2 > data.size()){
			SPRaise("Received packet truncated");
		}
		value |= ((uint32_t)(uint8_t)data[pos++]);
		value |= ((uint32_t)(uint8_t)data[pos++]) << 8;
		return (uint16_t)value;
	}
	uint8_t NetPacketReader::ReadByte() {
		SPADES_MARK_FUNCTION();
		
		if(pos >= data.size()){
			SPRaise("Received packet truncated");
		}
		return (uint8_t)data[pos++];
	}
	float NetPacketReader::ReadFloat() {
		SPADES_MARK_FUNCTION();
		union {
			float f;
			uint32_t v;
		};
		v = ReadInt();
		return f;
	}
	
	IntVector3 NetPacketReader::ReadIntColor() {
		SPADES_MARK_FUNCTION();
		IntVector3 col;
		col.z = ReadByte();
		col.y = ReadByte();
		col.x = ReadByte();
		return col;
	}
	
	Vector3 NetPacketReader::ReadFloatColor() {
		SPADES_MARK_FUNCTION();
		Vector3 col;
		col.z = ReadByte() / 255.f;
		col.y = ReadByte() / 255.f;
		col.x = ReadByte() / 255.f;
		return col;
	}
	
	std::string NetPacketReader::ReadData(size_t siz) {
		if(pos + siz > data.size()){
			SPRaise("Received packet truncated");
		}
		std::string s = std::string(data.data() + pos, siz);
		pos += siz;
		return s;
	}
	std::string NetPacketReader::ReadRemainingData() {
		return std::string(data.data() + pos,
						   data.size() - pos);
	}
	
	void NetPacketReader::DumpDebug() {
#if 1
		char buf[1024];
		std::string str;
		sprintf(buf, "Packet 0x%02x [len=%d]", GetTypeRaw(),
				(int)data.size());
		str = buf;
		int bytes = (int)data.size();
		if(bytes > 64){
			bytes = 64;
		}
		for(int i = 0; i < bytes; i++){
			sprintf(buf, " %02x", (unsigned int)(unsigned char)data[i]);
			str += buf;
		}
		
		
		SPLog("%s", str.c_str());
#endif
		
	}
	
#pragma mark - NetPacketWriter
	
	NetPacketWriter::NetPacketWriter(unsigned int type) {
		data.push_back(static_cast<char>(type));
	}
	void NetPacketWriter::Write(uint8_t v){
		SPADES_MARK_FUNCTION_DEBUG();
		data.push_back(v);
	}
	void NetPacketWriter::Write(uint16_t v){
		SPADES_MARK_FUNCTION_DEBUG();
		data.push_back((char)(v));
		data.push_back((char)(v >> 8));
	}
	void NetPacketWriter::Write(uint32_t v){
		SPADES_MARK_FUNCTION_DEBUG();
		data.push_back((char)(v));
		data.push_back((char)(v >> 8));
		data.push_back((char)(v >> 16));
		data.push_back((char)(v >> 24));
	}
	void NetPacketWriter::Write(float v){
		SPADES_MARK_FUNCTION_DEBUG();
		union {
			float f; uint32_t i;
		};
		f = v;
		Write(i);
	}
	void NetPacketWriter::WriteColor(IntVector3 v){
		Write((uint8_t)v.z);
		Write((uint8_t)v.y);
		Write((uint8_t)v.x);
	}
	void NetPacketWriter::Write(const std::string& str){
		data.insert(data.end(),
					str.begin(),
					str.end());
	}
}
