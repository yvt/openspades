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

#include "IFileSystem.h"
#include <stdint.h>
#include <map>
#include <string>

extern "C"{
	typedef void *unzFile;
	struct zlib_filefunc_def_s;
	typedef struct zlib_filefunc_def_s zlib_filefunc_def;
	struct unz_file_pos_s;
}
	
namespace spades {
	class ZipFileSystem: public IFileSystem {
		class ZipFileInputStream;
		class ZipFileHandle;
		
		IStream *baseStream;
		bool autoClose;
		unzFile zip;
		
		std::map<std::string, unz_file_pos_s> files;
		
		ZipFileInputStream *currentStream;
		
		uint64_t cursorPos;
		
		static ZipFileHandle *InternalOpen(ZipFileSystem *fs,
										   const char *fn,
										   int mode);
		
		static uint32_t InternalRead(ZipFileSystem *fs,
										   ZipFileHandle *h,
										   void *buf, uint32_t size);
		static uint32_t InternalWrite(ZipFileSystem *fs,
										   ZipFileHandle *h,
										   void *buf, uint32_t size);
		static long InternalTell(ZipFileSystem *fs,
									  ZipFileHandle *h);
		static long InternalSeek(ZipFileSystem *fs,
								 ZipFileHandle *h,
								 int32_t offset, int origin);
		static int InternalClose(ZipFileSystem *fs,
								 ZipFileHandle *h);
		static int InternalTestError(ZipFileSystem *fs,
								 ZipFileHandle *h);
		
		zlib_filefunc_def CreateZLibFileFunc();
		bool MoveToFile(const char *);
	public:
		ZipFileSystem(IStream *, bool autoClose = true);
		virtual ~ZipFileSystem();
		
		virtual std::vector<std::string> EnumFiles(const char *);
		
		virtual IStream *OpenForReading(const char *);
		virtual IStream *OpenForWriting(const char *);
		virtual bool FileExists(const char *);
	};
}
