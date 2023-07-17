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

#include "GLFramebufferManager.h"

namespace spades {
	namespace draw {
		class GLRenderer;
		class GLProgram;

		/**
		 * Implementation of the temporal anti-aliasing filter.
		 *
		 * The current implementation requires `BlitFramebuffer` for simplicity.
		 */
		class GLTemporalAAFilter {
			GLRenderer &renderer;
			GLProgram *program;

			struct HistoryBuffer {
				bool valid = false;
				int width, height;

				IGLDevice::UInteger framebuffer;
				IGLDevice::UInteger texture;
			} historyBuffer;

			Matrix4 prevMatrix;
			Vector3 prevViewOrigin;
			std::size_t jitterTableIndex = 0;

			void DeleteHistoryBuffer();

		public:
			GLTemporalAAFilter(GLRenderer &);
			~GLTemporalAAFilter();

			Vector2 GetProjectionMatrixJitter();

			GLColorBuffer Filter(GLColorBuffer, bool useFxaa);
		};
	}
}
