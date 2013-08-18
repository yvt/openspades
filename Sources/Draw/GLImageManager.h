//
//  GLImageManager.h
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <vector>
#include <string>
#include <map>

namespace spades {
	namespace draw {
		class IGLDevice;
		class GLImage;
		
		class GLImageManager {
			IGLDevice *device;
			std::map<std::string, GLImage *> images;
			
			GLImage *CreateImage(const std::string&);
		public:
			GLImageManager(IGLDevice *);
			~GLImageManager();
			
			GLImage *RegisterImage(const std::string&);
		};
	}
}
