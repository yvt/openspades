/*
 Copyright (c) 2013 yvt, learn_more
 
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

#include "ServerAddress.h"
#include <enet/enet.h>

namespace spades
{

// unsigned long version of atol, cross-platform (including MSVC)
uint32_t ServerAddress::ParseIntegerAddress( const std::string& str )
{
	uint32_t vl = 0;
	for( size_t i = 0; i < str.size(); i++ ) {
		char c = str[i];
		if( c >= '0' && c<= '9' ){
			vl *= 10;
			vl += (uint32_t)(c - '0');
		} else {
			break;
		}
	}
	
	return vl;
}


ServerAddress::ServerAddress( std::string address, ProtocolVersion::Version version )
: mAddress( address ), mVersion( version )
{
}

std::string ServerAddress::stripProtocol( const std::string& address ) const
{
	if(address.find("aos:///") == 0) {
		return address.substr(7);
	} else if(address.find("aos://") == 0) {
		return address.substr(6);
	}
	return address;
}

ENetAddress ServerAddress::asAddress() const
{
	std::string address = stripProtocol( mAddress );

	ENetAddress addr;
	size_t pos = address.find(':');
	if(pos == std::string::npos){
		//addr = address;
		addr.port = 32887;
	}else{
		addr.port = atoi(address.substr(pos+1).c_str());
		address = address.substr(0, pos);
	}
	
	if(address.find('.') != std::string::npos){
		enet_address_set_host( &addr, address.c_str() );
	}else{
		addr.host = ParseIntegerAddress( address );
	}
	return addr;
}

std::string ServerAddress::asString( bool includeProtocol ) const
{
	if( !includeProtocol ) {
		return stripProtocol( mAddress );
	}
	return mAddress;
}


}; //namespace spades