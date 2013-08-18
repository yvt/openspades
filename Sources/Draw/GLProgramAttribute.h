//
//  GLProgramAttribute.h
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
		class GLProgramAttribute {
			GLProgram *last;
			int loc;
			std::string name;
		public:
			GLProgramAttribute(const std::string&);
			int operator()() {
				return loc;
			}
			int operator()(GLProgram *);
		};
	}
}
