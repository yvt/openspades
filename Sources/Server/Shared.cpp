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
	
	Packet *Packet::Decode() {
		// TODO: implement!
		return nullptr;
	}
	
	ConnectRequestPacket *ConnectRequestPacket::Decode(const std::vector<char> &data) {
		SPADES_MARK_FUNCTION();
		
		std::unique_ptr<ConnectRequestPacket> p(new ConnectRequestPacket());
		NetPacketReader reader(data);
		
		return p.release();
	}
	
	std::vector<char> ConnectRequestPacket::Generate() {
		SPADES_MARK_FUNCTION();
		
		NetPacketWriter writer(TypeId);
		SPNotImplemented();
		
		return std::move(writer.ToArray());
	}
	
} }
