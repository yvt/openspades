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

#pragma once

#include <enet/enet.h>
#include <vector>
#include <Core/Math.h>

namespace spades {
	
	class NetPacketReader {
	protected:
		std::vector<char> data;
		size_t pos;
	public:
		NetPacketReader(ENetPacket *packet);
		
		NetPacketReader(const std::vector<char>& inData);
		unsigned int GetTypeRaw() {
			return static_cast<unsigned int>(data[0]);
		}
		uint32_t ReadInt();
		uint16_t ReadShort();
		uint8_t ReadByte();
		float ReadFloat();
		
		IntVector3 ReadIntColor();
		
		Vector3 ReadFloatColor();
		Vector2 ReadVector2();
		Vector3 ReadVector3();
		Vector4 ReadVector4();
		
		std::vector<char> GetData() {
			return data;
		}
		
		std::string ReadData(size_t siz);
		std::string ReadRemainingData();
		
		bool IsEndOfPacket() { return pos == data.size(); }
		
		void DumpDebug();
	};
	
	
	class NetPacketWriter {
	protected:
		std::vector<char> data;
	public:
		NetPacketWriter(unsigned int type);
		
		void Write(uint8_t v);
		void Write(uint16_t v);
		void Write(uint32_t v);
		void Write(float v);
		void Write(const Vector2&);
		void Write(const Vector3&);
		void Write(const Vector4&);
		void WriteColor(IntVector3 v);
		
		void Write(const std::string& str);
		
		std::vector<char> ToArray(bool move = true);
		std::vector<char>& GetArray() { return data; }
		
		ENetPacket *CreatePacket(int flag = ENET_PACKET_FLAG_RELIABLE) {
			return enet_packet_create(data.data(),
									  data.size(),
									  flag);
		}
	};
	
	
	
}
