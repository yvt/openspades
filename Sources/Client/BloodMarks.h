/*
 Copyright (c) 2021 yvt

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

#include <Core/Math.h>

namespace spades {
	namespace client {
		struct BloodMarksImpl;
		class Client;

		class BloodMarks {
			std::unique_ptr<BloodMarksImpl> impl;

		public:
			BloodMarks(Client &);
			~BloodMarks();

			/** Remove all local entities. */
			void Clear();

			/** Oh no, someone got hurt by a high speed projectile! */
			void Spatter(const Vector3 &position, const Vector3 &velocity, bool byLocalPlayer);

			/** Update the entities' states. */
			void Update(float dt);

			/** Issue drawing commands. */
			void Draw();
		};
	} // namespace client
} // namespace spades
