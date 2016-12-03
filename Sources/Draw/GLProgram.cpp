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
#include <cstring>

#include <Core/Debug.h>
#include <Core/Exception.h>
#include "GLProgram.h"
#include "GLShader.h"

namespace spades {
	namespace draw {
		GLProgram::GLProgram(IGLDevice *d, std::string name) : device(d), name(name) {
			SPADES_MARK_FUNCTION();
			handle = device->CreateProgram();
		}
		GLProgram::~GLProgram() {
			SPADES_MARK_FUNCTION();
			device->DeleteProgram(handle);
		}

		void GLProgram::Attach(spades::draw::GLShader *shader) {
			SPADES_MARK_FUNCTION();
			device->AttachShader(handle, shader->GetHandle());
		}

		void GLProgram::Attach(IGLDevice::UInteger shader) {
			SPADES_MARK_FUNCTION();
			device->AttachShader(handle, shader);
		}

		void GLProgram::Link() {
			SPADES_MARK_FUNCTION();
			device->LinkProgram(handle);

			std::vector<char> errMsg;
			errMsg.resize(device->GetProgramInteger(handle, IGLDevice::InfoLogLength) + 1);

			IGLDevice::Sizei outLen;
			device->GetProgramInfoLog(handle, static_cast<IGLDevice::Sizei>(errMsg.size()), &outLen,
			                          errMsg.data());
			errMsg[outLen] = 0;

			std::string err = errMsg.data();

			if (device->GetProgramInteger(handle, IGLDevice::LinkStatus) == 0) {
				// error

				SPRaise("Error while linking a program '%s':\n\n%s", name.c_str(), err.c_str());
			} else {
				if (errMsg.size() > 4) {
					SPLog("Messages for linking program '%s':\n%s", name.c_str(), err.c_str());
				}
			}

			linked = true;
		}

		void GLProgram::Validate() {
			SPADES_MARK_FUNCTION();
			device->ValidateProgram(handle);

			if (device->GetProgramInteger(handle, IGLDevice::ValidateStatus) == 0) {
				// error
				std::vector<char> errMsg;
				errMsg.resize(device->GetProgramInteger(handle, IGLDevice::InfoLogLength) + 1);

				IGLDevice::Sizei outLen;
				device->GetShaderInfoLog(handle, static_cast<IGLDevice::Sizei>(errMsg.size()),
				                         &outLen, errMsg.data());
				errMsg[outLen] = 0;

				std::string err = errMsg.data();
				SPRaise("Validation error while linking a program '%s':\n\n%s", name.c_str(),
				        errMsg.data());
			}
		}

		void GLProgram::Use() {
			SPADES_MARK_FUNCTION();
			device->UseProgram(handle);
		}
	}
}
