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

#include <cstdint>

#include "GLProgramAttribute.h"
#include "GLQuadRenderer.h"

namespace spades {
	namespace draw {
		GLQuadRenderer::GLQuadRenderer(IGLDevice *device) : device(device) {}

		GLQuadRenderer::~GLQuadRenderer() {}

		void GLQuadRenderer::SetCoordAttributeIndex(IGLDevice::UInteger idx) { attrIndex = idx; }

		void GLQuadRenderer::Draw() {
			static const uint8_t vertices[][4] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};

			device->EnableVertexAttribArray(attrIndex, true);
			device->VertexAttribPointer(attrIndex, 2, IGLDevice::UnsignedByte, false, 4, vertices);
			device->DrawArrays(IGLDevice::TriangleFan, 0, 4);
			device->EnableVertexAttribArray(attrIndex, false);
		}
	}
}
