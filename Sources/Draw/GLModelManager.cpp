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

#include "GLModelManager.h"
#include "GLVoxelModel.h"
#include "../Core/VoxelModel.h"
#include "../Core/FileManager.h"
#include "../Core/IStream.h"
#include "../Core/Debug.h"
#include "GLRenderer.h"
#include "../Core/Settings.h"

namespace spades {
	namespace draw {
		GLModelManager::GLModelManager(GLRenderer *r) {
			SPADES_MARK_FUNCTION();
			
			renderer = r;
		}
		GLModelManager::~GLModelManager() {
			SPADES_MARK_FUNCTION();
			
			for(std::map<std::string, GLModel *>::iterator it = models.begin();
				it != models.end(); it++){
				delete it->second;
			}
		}
		
		GLModel *GLModelManager::RegisterModel(const char *name){
			SPADES_MARK_FUNCTION();
			
			std::map<std::string, GLModel *>::iterator it;
			it = models.find(std::string(name));
			if(it == models.end()){
				GLModel *m = CreateModel(name);
				models[name] = m;
				return m;
			}
			return it->second;
		}
		
		GLModel *GLModelManager::CreateModel(const char *name) {
			SPADES_MARK_FUNCTION();
			
			VoxelModel *bmp;
			IStream *stream = FileManager::OpenForReading(name);
			try{
				bmp = VoxelModel::LoadKV6(stream);
				delete stream;
			}catch(...){
				delete stream;
				throw;
			}
			
			return static_cast<GLModel *>(renderer->CreateModelOptimized(bmp)); //new GLVoxelModel(bmp, renderer);
		}
		
	}
}
