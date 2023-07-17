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

#include <map>
#include <string>
#include <vector>

namespace spades {
	namespace draw {
		class IGLDevice;
		class GLImage;
		class GLRenderer;

		class GLImageManager {
			IGLDevice &device;
			std::map<std::string, GLImage *> images;
			GLImage *whiteImage;

			GLImage *CreateImage(const std::string &);

		public:
			GLImageManager(IGLDevice &);
			~GLImageManager();

			GLImage *RegisterImage(const std::string &);
			GLImage *GetWhiteImage();

			void DrawAllImages(GLRenderer *);

			void ClearCache();
		};
	} // namespace draw
} // namespace spades
