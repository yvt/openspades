//
//  IFileSystem.h
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
	class IFileSystem {
	public:
		virtual std::vector<std::string> EnumFiles(const char *) = 0;
		virtual IStream *OpenForReading(const char *) = 0;
		virtual IStream *OpenForWriting(const char *) = 0;
		virtual bool FileExists(const char *) = 0;
		
	};
}
