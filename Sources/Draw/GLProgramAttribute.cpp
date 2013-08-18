//
//  GLProgramAttribute.cpp
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "GLProgramAttribute.h"
#include "../Core/Exception.h"
#include "../Core/Debug.h"
#include "../Core/IStream.h"

namespace spades {
	namespace draw {
		GLProgramAttribute::GLProgramAttribute(const std::string& n):
		name(n), last(NULL), loc(-1){}
		int GLProgramAttribute::operator()(spades::draw::GLProgram *prog){
			
			SPADES_MARK_FUNCTION_DEBUG();
			
			if(prog == NULL) {
				SPInvalidArgument("prog");
			}
			if(prog != last){
				last = prog;
				loc = last->GetDevice()->GetAttribLocation(last->GetHandle(), name.c_str());
				if(loc == -1){
					fprintf(stderr, "WARNING: GLSL attribute '%s' not found\n", name.c_str());
				}
			}
			return loc;
		}
	}
}
