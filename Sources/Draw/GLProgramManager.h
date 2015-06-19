/*
 Copyright (c) 2013 OpenSpades Developers
 
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
