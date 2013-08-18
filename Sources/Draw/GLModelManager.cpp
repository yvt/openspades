//
//  GLModelManager.cpp
//  OpenSpades
//
//  Created by yvt on 7/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

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
