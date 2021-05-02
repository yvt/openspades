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

#include <vector>

#include "IGLDevice.h"

namespace spades {
	namespace draw {
		class GLShader {
			IGLDevice &device;
			IGLDevice::UInteger handle;
			std::vector<std::string> sources;
			bool compiled;

		public:
			enum Type { VertexShader, FragmentShader, GeometryShader };

			GLShader(IGLDevice &, Type);
			~GLShader();

			void AddSource(const std::string &);

			void Compile();
			IGLDevice::UInteger GetHandle() const { return handle; }

			bool IsCompiled() const { return compiled; }

			IGLDevice &GetDevice() const { return device; }
		};
	} // namespace draw
} // namespace spades
