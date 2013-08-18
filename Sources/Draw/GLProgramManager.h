//
//  GLProgramManager.h
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <map>
#include <string>

namespace spades {
	namespace draw {
		class IGLDevice;
		class GLProgram;
		class GLShader;
		class IGLShadowMapRenderer;
		class GLProgramManager {
			IGLDevice *device;
			IGLShadowMapRenderer *shadowMapRenderer;
			
			std::map<std::string, GLProgram *> programs;
			std::map<std::string, GLShader *> shaders;
			
			GLProgram *CreateProgram(const std::string& name);
			GLShader *CreateShader(const std::string& name);
			
		public:
			GLProgramManager(IGLDevice *, IGLShadowMapRenderer *shadowMapRenderer);
			~GLProgramManager();
			
			GLProgram *RegisterProgram(const std::string& name);
			GLShader *RegisterShader(const std::string& name);
			
		};
	}
}
