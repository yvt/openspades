/*
 Copyright (c) 2016 yvt

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

#include <vector>

namespace spades {
	namespace client {
		class GradientCoherentNoiseSampler1D {
		public:
			GradientCoherentNoiseSampler1D();
			~GradientCoherentNoiseSampler1D();

			float Sample(float x);

		private:
			std::vector<float> derivatives;
		};

		class CoherentNoiseSampler1D {
		public:
			CoherentNoiseSampler1D();
			~CoherentNoiseSampler1D();

			float Sample(float x);

		private:
			std::vector<float> values;
			GradientCoherentNoiseSampler1D gcns;
		};
	} // namespace client
} // namespace spades
