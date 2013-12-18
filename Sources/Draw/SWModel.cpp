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

#include "SWModel.h"
#include <Core/FileManager.h>
#include <Core/IStream.h>

namespace spades {
	namespace draw {
		SWModel::SWModel(VoxelModel *m):
		rawModel(m) {}
		
		SWModel::~SWModel() {
		}
		
		SWModelManager::~SWModelManager() {
			for(auto it = models.begin(); it != models.end(); it++)
				it->second->Release();
		}
		
		SWModel *SWModelManager::RegisterModel(const std::string &name) {
			auto it = models.find(name);
			if(it == models.end()) {
				auto *str = FileManager::OpenForReading(name.c_str());
				Handle<VoxelModel> vm;
				try {
					vm.Set(VoxelModel::LoadKV6(str), false);
					auto *m = CreateModel(vm);
					models.insert(std::make_pair(name, m));
					m->AddRef();
					return m;
				} catch (...) {
					delete str;
					throw;
				}
			}else{
				auto *model = it->second;
				model->AddRef();
				return model;
			}
		}
		
		SWModel *SWModelManager::CreateModel(spades::VoxelModel *vm) {
			return new SWModel(vm);
		}
	}
}


