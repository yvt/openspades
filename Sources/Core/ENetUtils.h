/*
 Copyright (c) 2017 yvt

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

#include <Core/Strings.h>
#include <enet/enet.h>

inline bool operator==(const ENetAddress &a, const ENetAddress &b) {
	return a.host == b.host && a.port == b.port;
}

namespace spades {
	template <> std::string ToString<ENetAddress>(const ENetAddress &v) {
		return Format("{0}:{1}", v.host, v.port);
	}
}

namespace std {
	template <> struct hash<ENetAddress> {
		using argument_type = ENetAddress;
		using result_type = std::size_t;

		result_type operator()(argument_type const &x) const noexcept {
			return hash<enet_uint32>{}(x.host) ^ (hash<enet_uint16>{}(x.port) << 1);
		}
	};
}
