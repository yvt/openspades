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

#include <string>

#include <Core/ServerAddress.h>

namespace spades {
	namespace client {
		/**
		 * Stores properties that affect the gameplay.
		 */
		struct GameProperties {
			/**
			 * Construct and initialize `GameProperties` with values optimal for the compatibility
			 * with the original client.
			 */
			GameProperties(ProtocolVersion protocol) : protocolVersion{protocol} {}

			/**
			 * Parse a given server message and speculatively update the properties.
			 */
			void HandleServerMessage(const std::string &msg);

			/**
			 * The protocol version affecting various physics properties
			 * e.g., weapon spread values.
			 */
			ProtocolVersion protocolVersion;

			/**
			 * Indicates whether the server does not provide the game properties in a way OpenSpades
			 * can reliably parse and it must speculate them using non-program friendly information
			 * (e.g., server message).
			 *
			 * Reserved for a future extension.
			 */
			bool useHeuristics = true;

			bool clearCorpseOnRespawn = false;

			/**
			 * Raises the upper limit of the number of player slots to 128.
			 *
			 * Reserved for a future extension.
			 */
			bool manyPlayers = false;

			int GetMaxNumPlayerSlots() const { return manyPlayers ? 128 : 32; }
		};
	}
}
