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

#include <Client/IModel.h>

namespace spades {
	namespace draw {
		class GLModel;
		class GLRenderer;
		class GLModelManager {
			GLRenderer *renderer;
			std::map<std::string, GLModel *> models;
			GLModel *CreateModel(const char *);

		public:
			GLModelManager(GLRenderer *);
			~GLModelManager();
			GLModel *RegisterModel(const char *);
		};
	}
}
