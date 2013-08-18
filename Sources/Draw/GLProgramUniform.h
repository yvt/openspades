//
//  GLProgramUniform.h
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "GLProgram.h"
#include <string>

namespace spades {
	namespace draw {
		class GLProgramUniform {
			GLProgram *last;
			int loc;
			std::string name;
		public:
			GLProgramUniform(const std::string&);
			void SetProgram(GLProgram *);
			void operator ()(GLProgram *p) {
				SetProgram(p);
			}
			void SetValue(IGLDevice::Float);
			void SetValue(IGLDevice::Float,
						  IGLDevice::Float);
			void SetValue(IGLDevice::Float,
						  IGLDevice::Float,
						  IGLDevice::Float);
			void SetValue(IGLDevice::Float,
						  IGLDevice::Float,
						  IGLDevice::Float,
						  IGLDevice::Float);
			void SetValue(IGLDevice::Integer);
			void SetValue(IGLDevice::Integer,
						  IGLDevice::Integer);
			void SetValue(IGLDevice::Integer,
						  IGLDevice::Integer,
						  IGLDevice::Integer);
			void SetValue(IGLDevice::Integer,
						  IGLDevice::Integer,
						  IGLDevice::Integer,
						  IGLDevice::Integer);
			void SetValue(const Matrix4&, bool transpose = false);
			
		};
	}
}
