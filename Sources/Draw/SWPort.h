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

#include <Core/Bitmap.h>
#include <Core/RefCountedObject.h>

namespace spades {
	namespace draw {
		class SWPort : public virtual RefCountedObject {
		protected:
			~SWPort() {}

		public:
			/**
			 * Returns a `Bitmap` on which the scene is rendered.
			 */
			virtual Bitmap &GetFramebuffer() = 0;

			/**
			 * Presents the contents of the framebuffer (returned by
			 * `GetFramebuffer`) to the screen.
			 */
			virtual void Swap() = 0;
		};
	} // namespace draw
} // namespace spades
