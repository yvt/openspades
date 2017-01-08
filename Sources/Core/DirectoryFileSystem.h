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

#include "IFileSystem.h"

namespace spades {
	class DirectoryFileSystem : public IFileSystem {
		std::string rootPath;
		bool canWrite;

		std::string physicalPath(const std::string &);

	public:
		DirectoryFileSystem(const std::string &root, bool canWrite = true);
		~DirectoryFileSystem();

		std::vector<std::string> EnumFiles(const char *) override;

		IStream *OpenForReading(const char *) override;
		IStream *OpenForWriting(const char *) override;
		bool FileExists(const char *) override;
	};
}
