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

#include "GLProgramAttribute.h"
#include <Core/Debug.h>
#include <Core/Exception.h>
#include <Core/IStream.h>

namespace spades {
	namespace draw {
		GLProgramAttribute::GLProgramAttribute(const std::string &n)
		    : last(NULL), loc(-1), name(n) {}
		int GLProgramAttribute::operator()(spades::draw::GLProgram *prog) {

			SPADES_MARK_FUNCTION_DEBUG();

			if (prog == NULL) {
				SPInvalidArgument("prog");
			}
			if (prog != last) {
				last = prog;
				loc = last->GetDevice()->GetAttribLocation(last->GetHandle(), name.c_str());
				if (loc == -1) {
					fprintf(stderr, "WARNING: GLSL attribute '%s' not found\n", name.c_str());
				}
			}
			return loc;
		}
	}
}
