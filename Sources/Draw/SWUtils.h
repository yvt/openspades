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

#include <algorithm>
#include <array>
#include <memory>

#include <Core/ConcurrentDispatch.h>
#include <Core/Debug.h>

namespace spades {
	namespace draw {
		int GetNumSWRendererThreads();

		template <class F> static void InvokeParallel(F f, unsigned int numThreads) {
			SPAssert(numThreads <= 32);
			std::array<std::unique_ptr<ConcurrentDispatch>, 32> disp;
			for (auto i = 1U; i < numThreads; i++) {
				auto ff = [i, &f]() { f(i); };
				disp[i] = std::unique_ptr<ConcurrentDispatch>(
				  static_cast<ConcurrentDispatch *>(new FunctionDispatch<decltype(ff)>(ff)));
				disp[i]->Start();
			}
			f(0);
			for (auto i = 1U; i < numThreads; i++) {
				disp[i]->Join();
			}
		}

		template <class F> static void InvokeParallel2(F f) {

			unsigned int numThreads = static_cast<unsigned int>(GetNumSWRendererThreads());
			numThreads = std::max(numThreads, 1U);
			numThreads = std::min(numThreads, 32U);

			std::array<std::unique_ptr<ConcurrentDispatch>, 32> disp;
			for (auto i = 1U; i < numThreads; i++) {
				auto ff = [i, &f, numThreads]() { f(i, numThreads); };
				disp[i] = std::unique_ptr<ConcurrentDispatch>(
				  static_cast<ConcurrentDispatch *>(new FunctionDispatch<decltype(ff)>(ff)));
				disp[i]->Start();
			}
			f(0, numThreads);
			for (auto i = 1U; i < numThreads; i++) {
				disp[i]->Join();
			}
		}

		static inline PURE int ToFixed8(float v) {
			int i = static_cast<int>(v * 255.f + .5f);
			return std::max(std::min(i, 255), 0);
		}

		static inline PURE int ToFixedFactor8(float v) {
			int i = static_cast<int>(v * 256.f + .5f);
			return std::max(std::min(i, 256), 0);
		}
	} // namespace draw
} // namespace spades
