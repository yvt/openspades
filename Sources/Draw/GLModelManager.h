//
//  GLModelManager.h
//  OpenSpades
//
//  Created by yvt on 7/15/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "../Client/IModel.h"
#include <map>
#include <string>

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
