//
//  GLShader.cpp
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLShader.h"
#include "../Core/Exception.h"
#include "../Core/Debug.h"
#include <string.h>

namespace spades {
	namespace draw {
		GLShader::GLShader(IGLDevice *dev, Type type):
		device(dev), compiled(false){
			SPADES_MARK_FUNCTION();
			
			switch(type){
				case VertexShader:
					handle = device->CreateShader(IGLDevice::VertexShader);
					break;
				case FragmentShader:
					handle = device->CreateShader(IGLDevice::FragmentShader);
					break;
				default:
					SPInvalidEnum("type", type);
			}
		}
		GLShader::~GLShader() {
			SPADES_MARK_FUNCTION();
			
			device->DeleteShader(handle);
		}
		
		void GLShader::AddSource(const std::string &src) {
			SPADES_MARK_FUNCTION();
			
			sources.push_back(src);
		}
		
		void GLShader::Compile() {
			SPADES_MARK_FUNCTION();
			
			std::vector<const char *> srcs;
			std::vector<int> lens;
			
			for(size_t i = 0; i < sources.size(); i++){
				srcs.push_back(sources[i].c_str());
				lens.push_back(sources[i].size());
			}
			
			device->ShaderSource(handle, srcs.size(),
								 srcs.data(), lens.data());
			
			device->CompileShader(handle);
			
			if(device->GetShaderInteger(handle, IGLDevice::CompileStatus) == 0){
				// error
				std::vector<char> errMsg;
				errMsg.resize(device->GetShaderInteger(handle, IGLDevice::InfoLogLength) + 1);
				
				IGLDevice::Sizei outLen;
				device->GetShaderInfoLog(handle, errMsg.size(), &outLen, errMsg.data());
				errMsg[outLen] = 0;
				
				std::string src;
				for(size_t i = 0; i < sources.size(); i++){
					src += sources[i];
				}
				
				std::string err = errMsg.data();
				SPRaise("Error while compiling a shader:\n\n%s\n\n"
						"Shader source:\n\n%s\n", errMsg.data(),
						src.c_str());
			}
			
			compiled = true;
		}
		
		
	}
}
