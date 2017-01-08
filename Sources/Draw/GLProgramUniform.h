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

#pragma once

#include <string>

#include "GLProgram.h"

namespace spades {
	namespace draw {
		class GLProgramUniform {
			GLProgram *last;
			int loc;
			std::string name;

		public:
			GLProgramUniform(const std::string &);
			void SetProgram(GLProgram *);
			void operator()(GLProgram *p) { SetProgram(p); }
			void SetValue(IGLDevice::Float);
			void SetValue(IGLDevice::Float, IGLDevice::Float);
			void SetValue(IGLDevice::Float, IGLDevice::Float, IGLDevice::Float);
			void SetValue(IGLDevice::Float, IGLDevice::Float, IGLDevice::Float, IGLDevice::Float);
			void SetValue(IGLDevice::Integer);
			void SetValue(IGLDevice::Integer, IGLDevice::Integer);
			void SetValue(IGLDevice::Integer, IGLDevice::Integer, IGLDevice::Integer);
			void SetValue(IGLDevice::Integer, IGLDevice::Integer, IGLDevice::Integer,
			              IGLDevice::Integer);
			void SetValue(const Matrix4 &, bool transpose = false);
			bool IsActive() const { return loc != -1; }
		};
	}
}
