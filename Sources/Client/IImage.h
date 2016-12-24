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

#include <Core/RefCountedObject.h>

namespace spades {

	class Bitmap;

	namespace client {
		class LowLevelNativeImage;
		class IImage : public RefCountedObject {
		protected:
			virtual ~IImage() {}

		public:
			IImage() {}

			virtual void Update(Bitmap &, int x, int y) = 0;

			virtual float GetWidth() = 0;
			virtual float GetHeight() = 0;
		};
	}
}
