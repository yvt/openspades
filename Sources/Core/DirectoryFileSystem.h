//
//  DirectoryFileSystem.h
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include "IFileSystem.h"
#include <string>

namespace spades {
	class DirectoryFileSystem: public IFileSystem {
		std::string rootPath;
		bool canWrite;
		
		std::string physicalPath(const std::string&);
	public:
		DirectoryFileSystem(const std::string& root, bool canWrite = true);
		virtual ~DirectoryFileSystem();
		
		virtual std::vector<std::string> EnumFiles(const char *);
		
		virtual IStream *OpenForReading(const char *);
		virtual IStream *OpenForWriting(const char *);
		virtual bool FileExists(const char *);
		
	};
}
