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
#include <memory>

#include "GLModelManager.h"
#include "GLRenderer.h"
#include "GLVoxelModel.h"
#include <Core/Debug.h>
#include <Core/IStream.h>
#include <Core/Settings.h>
#include <Core/VoxelModel.h>
#include <Core/VoxelModelLoader.h>

namespace spades {
	namespace draw {
		GLModelManager::GLModelManager(GLRenderer &r) : renderer{r} { SPADES_MARK_FUNCTION(); }
		GLModelManager::~GLModelManager() { SPADES_MARK_FUNCTION(); }

		Handle<GLModel> GLModelManager::RegisterModel(const char *name) {
			SPADES_MARK_FUNCTION();

			auto it = models.find(std::string(name));
			if (it == models.end()) {
				Handle<GLModel> m = CreateModel(name);
				models[name] = m;
				return m;
			}
			return it->second;
		}

		Handle<GLModel> GLModelManager::CreateModel(const char *name) {
			SPADES_MARK_FUNCTION();

			auto voxelModel = VoxelModelLoader::Load(name);

			return renderer.CreateModelOptimized(*voxelModel).Cast<GLModel>();
		}

		void GLModelManager::ClearCache() { models.clear(); }
	} // namespace draw
} // namespace spades
