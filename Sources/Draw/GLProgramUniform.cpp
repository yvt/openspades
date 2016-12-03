/*
 Copyright (c) 2013 yvt

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

#include <cstdio>

#include "GLProgramUniform.h"
#include <Core/Exception.h>
#include <Core/Debug.h>

namespace spades {
	namespace draw {
		GLProgramUniform::GLProgramUniform(const std::string& n):
		name(n){
			last = NULL;
		}

		void GLProgramUniform::SetProgram(spades::draw::GLProgram *p){
			SPADES_MARK_FUNCTION_DEBUG();

			if(p == NULL) {
				SPInvalidArgument("prog");
			}
			if(last != p){
				last = p;
				last->Use();
				loc = p->GetDevice()->GetUniformLocation
				(p->GetHandle(), name.c_str())
				 ;
				if(loc == -1){
					fprintf(stderr,"WARNING: uniform '%s' not found\n",
							name.c_str());
				}
			}
		}

		void GLProgramUniform::SetValue(IGLDevice::Float v){
			SPADES_MARK_FUNCTION_DEBUG();
			last->GetDevice()->Uniform(loc, v);
		}
		void GLProgramUniform::SetValue(IGLDevice::Float x,
										IGLDevice::Float y){
			SPADES_MARK_FUNCTION_DEBUG();
			last->GetDevice()->Uniform(loc, x, y);
		}
		void GLProgramUniform::SetValue(IGLDevice::Float x,
										IGLDevice::Float y,
										IGLDevice::Float z){
			SPADES_MARK_FUNCTION_DEBUG();
			last->GetDevice()->Uniform(loc, x, y, z);
		}
		void GLProgramUniform::SetValue(IGLDevice::Float x,
										IGLDevice::Float y,
										IGLDevice::Float z,
										IGLDevice::Float w){
			SPADES_MARK_FUNCTION_DEBUG();
			last->GetDevice()->Uniform(loc, x, y, z, w);
		}

		void GLProgramUniform::SetValue(IGLDevice::Integer x){
			SPADES_MARK_FUNCTION_DEBUG();
			last->GetDevice()->Uniform(loc, x);
		}
		void GLProgramUniform::SetValue(IGLDevice::Integer x,
										IGLDevice::Integer y){
			SPADES_MARK_FUNCTION_DEBUG();
			last->GetDevice()->Uniform(loc, x, y);
		}
		void GLProgramUniform::SetValue(IGLDevice::Integer x,
										IGLDevice::Integer y,
										IGLDevice::Integer z){
			SPADES_MARK_FUNCTION_DEBUG();
			last->GetDevice()->Uniform(loc, x, y, z);
		}
		void GLProgramUniform::SetValue(IGLDevice::Integer x,
										IGLDevice::Integer y,
										IGLDevice::Integer z,
										IGLDevice::Integer w){
			SPADES_MARK_FUNCTION_DEBUG();
			last->GetDevice()->Uniform(loc, x, y, z, w);
		}
		void GLProgramUniform::SetValue(const spades::Matrix4 &m,
										bool transpose) {
			SPADES_MARK_FUNCTION_DEBUG();
			last->GetDevice()->Uniform(loc, transpose, m);
		}
	}
}
