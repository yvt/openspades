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

#include <enet/enet.h>
#include <regex>

#include "ServerAddress.h"

namespace spades {

	// unsigned long version of atol, cross-platform (including MSVC)
	uint32_t ServerAddress::ParseIntegerAddress(const std::string &str) {
		uint32_t vl = 0;
		for (size_t i = 0; i < str.size(); i++) {
			char c = str[i];
			if (c >= '0' && c <= '9') {
				vl *= 10;
				vl += (uint32_t)(c - '0');
			} else {
				break;
			}
		}

		return vl;
	}

	ServerAddress::ServerAddress(std::string address, ProtocolVersion version)
	: mAddress(address), mVersion(version) {
		static std::regex v075regex {"(.*):0?\\.?75"};
		static std::regex v076regex {"(.*):0?\\.?76"};

		std::smatch matchResult;

		if (std::regex_match(address, matchResult, v075regex)) {
			version = ProtocolVersion::v075;
			mAddress = matchResult[1];
		} else if (std::regex_match(address, matchResult, v076regex)) {
			version = ProtocolVersion::v076;
			mAddress = matchResult[1];
		}
	}

	std::string ServerAddress::StripProtocol(const std::string &address) {
		if (address.find("aos:///") == 0) {
			return address.substr(7);
		} else if (address.find("aos://") == 0) {
			return address.substr(6);
		}
		return address;
	}

	ENetAddress ServerAddress::GetENetAddress() const {
		std::string address = StripProtocol(mAddress);

		ENetAddress addr;
		size_t pos = address.find(':');
		if (pos == std::string::npos) {
			// addr = address;
			addr.port = 32887;
		} else {
			addr.port = atoi(address.substr(pos + 1).c_str());
			address = address.substr(0, pos);
		}

		if (address.find('.') != std::string::npos) {
			enet_address_set_host(&addr, address.c_str());
		} else {
			addr.host = ParseIntegerAddress(address);
		}
		return addr;
	}

	std::string ServerAddress::ToString(bool includeProtocol) const {
		if (!includeProtocol) {
			return StripProtocol(mAddress);
		}
		return mAddress;
	}

}; // namespace spades
