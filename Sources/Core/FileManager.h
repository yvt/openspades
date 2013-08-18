//
//  FileManager.h
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

namespace spades {
	class IStream;
	class IFileSystem;
	class FileManager {
		FileManager() {}
	public:
		static IStream *OpenForReading(const char *);
		static IStream *OpenForWriting(const char *);
		static bool FileExists(const char *);
		static void AddFileSystem(IFileSystem *);
		static std::vector<std::string> EnumFiles(const char *);
		static std::string ReadAllBytes(const char *);
	};
};
