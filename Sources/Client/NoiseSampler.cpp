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

#include "NoiseSampler.h"
#include <Core/Math.h>

using namespace std;

namespace spades {
	namespace client {

		CoherentNoiseSampler1D::CoherentNoiseSampler1D() {
			values.resize(1024);
			for (float &value : values) {
				value = SampleRandomFloat() - SampleRandomFloat();
			}
		}

		CoherentNoiseSampler1D::~CoherentNoiseSampler1D() {}

		float CoherentNoiseSampler1D::Sample(float x) {
			// Wrap around
			x /= static_cast<float>(values.size());
			x -= floor(x);
			x *= static_cast<float>(values.size());

			int intCoord = static_cast<int>(floor(x));
			float fracCoord = x - floor(x);

			// Sample values
			const int mask = static_cast<int>(values.size() - 1);
			float v1 = values[intCoord & mask];
			float v2 = values[(intCoord + 1) & mask];

			// Create and evaluate a polynomial with given values
			return Mix(v1, v2, SmoothStep(fracCoord)) + gcns.Sample(x);
		}

		GradientCoherentNoiseSampler1D::GradientCoherentNoiseSampler1D() {
			derivatives.resize(1024);
			for (float &derivative : derivatives) {
				derivative = (SampleRandomFloat() - SampleRandomFloat()) * 4.f;
			}
		}

		GradientCoherentNoiseSampler1D::~GradientCoherentNoiseSampler1D() {}

		float GradientCoherentNoiseSampler1D::Sample(float x) {
			// Wrap around
			x /= static_cast<float>(derivatives.size());
			x -= floor(x);
			x *= static_cast<float>(derivatives.size());

			int intCoord = static_cast<int>(floor(x));
			float fracCoord = x - floor(x);

			// Sample derivatives
			const int mask = static_cast<int>(derivatives.size() - 1);
			float d1 = derivatives[intCoord & mask];
			float d2 = derivatives[(intCoord + 1) & mask];

			// Create and evaluate a polynomial with given derivatives
			return (d1 * (1.f - fracCoord) - d2 * fracCoord) * fracCoord * (1.f - fracCoord);
		}
	}
}
