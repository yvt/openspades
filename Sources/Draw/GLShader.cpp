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

#include <cstring>

#include "GLShader.h"
#include <Core/Debug.h>
#include <Core/Exception.h>

namespace spades {
	namespace draw {
		GLShader::GLShader(IGLDevice *dev, Type type) : device(dev), compiled(false) {
			SPADES_MARK_FUNCTION();

			switch (type) {
				case VertexShader: handle = device->CreateShader(IGLDevice::VertexShader); break;
				case GeometryShader: handle = device->CreateShader(IGLDevice::GeometryShader); break;
				case FragmentShader:
					handle = device->CreateShader(IGLDevice::FragmentShader);
					break;
				default: SPInvalidEnum("type", type);
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

			for (size_t i = 0; i < sources.size(); i++) {
				srcs.push_back(sources[i].c_str());
				lens.push_back(static_cast<int>(sources[i].size()));
			}

			device->ShaderSource(handle, static_cast<IGLDevice::Sizei>(srcs.size()), srcs.data(),
			                     lens.data());

			device->CompileShader(handle);

			if (device->GetShaderInteger(handle, IGLDevice::CompileStatus) == 0) {
				// error
				std::vector<char> errMsg;
				errMsg.resize(device->GetShaderInteger(handle, IGLDevice::InfoLogLength) + 1);

				IGLDevice::Sizei outLen;
				device->GetShaderInfoLog(handle, static_cast<IGLDevice::Sizei>(errMsg.size()),
				                         &outLen, errMsg.data());
				errMsg[outLen] = 0;

				std::string src;
				for (size_t i = 0; i < sources.size(); i++) {
					src += sources[i];
				}

				std::string err = errMsg.data();
				SPRaise("Error while compiling a shader:\n\n%s\n\n"
				        "Shader source:\n\n%s\n",
				        errMsg.data(), src.substr(0, 500).c_str());
			}

			compiled = true;
		}
	}
}
