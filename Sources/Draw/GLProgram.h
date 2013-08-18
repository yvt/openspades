//
//  GLProgram.h
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IGLDevice.h"

namespace spades {
	namespace draw {
		class GLShader;
		class GLProgramUniform;
		class GLProgram {
			IGLDevice *device;
			IGLDevice::UInteger handle;
			bool linked;
			std::string name;
			
		public:
			GLProgram(IGLDevice *, std::string name = "(unnamed)");
			~GLProgram();
			
			void Attach(GLShader *);
			void Attach(IGLDevice::UInteger shader);
			
			void Link();
			void Validate();
			
			bool IsLinked() const {return linked; }
			void Use();
			
			IGLDevice::UInteger GetHandle() const {return  handle;}
			
			IGLDevice *GetDevice() const { return device; }
		};
	}
}
