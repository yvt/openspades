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

#include <memory>
#include <string>

#include <Core/TMPUtils.h>

namespace spades {
	struct PingTesterResult {
		/** Round-trip time, measured in milliseconds. */
		stmp::optional<int> ping;
	};

	class PingTester {
		class Private;
		std::unique_ptr<Private> priv;

	public:
		PingTester();
		~PingTester();

		/**
		 * Adds the specified address to the measurement list. If the
		 * address is already added, it does nothing.
		 *
		 * After that, you must call `Update()` periodically.
		 * The result will eventually be available via `GetTargetResult()`.
		 * (Eventually = in an unbounded time)
		 */
		void AddTarget(const std::string &);

		void Update();

		/**
		 * Retrieves the measurement result for the specified address.
		 */
		stmp::optional<PingTesterResult> GetTargetResult(const std::string &);
	};
}
