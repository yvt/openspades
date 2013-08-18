//
//  GLShader.h
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IGLDevice.h"
#include <vector>

namespace spades {
	namespace draw {
		class GLShader {
			IGLDevice *device;
			IGLDevice::UInteger handle;
			std::vector<std::string> sources;
			bool compiled;
		public:
			
			enum Type {
				VertexShader,
				FragmentShader
			};
			
			GLShader(IGLDevice *, Type);
			~GLShader();
			
			void AddSource(const std::string&);
			
			void Compile();
			IGLDevice::UInteger GetHandle() const {return handle;}
			
			bool IsCompiled() const { return compiled; }
			
			IGLDevice *GetDevice() const { return device; }
			
		};
	}
}
