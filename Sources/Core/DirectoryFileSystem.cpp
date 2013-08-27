//
//  DirectoryFileSystem.cpp
//  OpenSpades
//
//  Created by yvt on 7/13/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "DirectoryFileSystem.h"
#include "StdStream.h"
#include "Exception.h"
#include <sys/stat.h>
#include "Debug.h"

#ifdef WIN32
#include <windows.h>
#include <io.h>
#ifdef _MSC_VER
#include <direct.h>
#define mkdir		_mkdir
#endif
#else
#include <dirent.h>
#endif

namespace spades {

	DirectoryFileSystem::DirectoryFileSystem(const std::string& r, bool canWrite):
	rootPath(r), canWrite(canWrite) {
		SPADES_MARK_FUNCTION();
		SPLog("Directory File System Initialized: %s (%s)",
			  r.c_str(), canWrite ? "Read/Write" : "Read-only");
	}
	
	DirectoryFileSystem::~DirectoryFileSystem(){
		SPADES_MARK_FUNCTION();
		
	}
	
	std::string DirectoryFileSystem::physicalPath(const std::string &lg) {
		// TODO: check ".."?
		return rootPath + '/' + lg;
	}
	
	std::vector<std::string> DirectoryFileSystem::EnumFiles(const char *p){
#ifdef WIN32
		WIN32_FIND_DATA fd;
		HANDLE h;
		std::vector<std::string>  ret;
		std::string filePath;
		
		std::string path = physicalPath(p);
		// open the Win32 find handle.
		h=FindFirstFileEx((path+"\\*").c_str(),
						   FindExInfoStandard, &fd,
						   FindExSearchNameMatch,
						   NULL, 0);
		
		if(h==INVALID_HANDLE_VALUE){
			// couldn't open. return the empty vector.
			return ret;
		}
		
		do{
			// is it a directory?
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY){
				// "." and ".." mustn't be included.
				if(strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..")){
					filePath=fd.cFileName;
					ret.push_back(filePath);
				}
			}else{
				// usual file.
				filePath=fd.cFileName;
				ret.push_back(filePath);
			}
			
			// iterate!
		}while(FindNextFile(h, &fd));
		
		// close the handle.
		FindClose(h);
		
		return ret;
#else
		// open the directory.
		
		std::string path = physicalPath(p);
		DIR *dir=opendir(path.c_str());
		struct dirent *ent;
		
		std::vector<std::string>  ret;
		std::string filePath;
		
		// if couldn't open the directory, return the empty vector.
		if(!dir)
			return ret;
		
		// read an entry.
		while((ent=readdir(dir))){
			if(ent->d_name[0]=='.')
				continue;
			
			// make it full-path.
			filePath=ent->d_name;
			
			// add to the result vector.
			ret.push_back(filePath);
		}
		
		// close the directory.
		closedir(dir);
		
		return ret;
#endif
	}
	
	IStream *DirectoryFileSystem::OpenForReading(const char *fn){
		SPADES_MARK_FUNCTION();
		
		std::string path = physicalPath(fn);
		FILE *f = fopen(path.c_str(), "rb");
		if(f == NULL) {
			SPRaise("I/O error while opening %s for reading", fn);
		}
		return new StdStream(f, true);
	}
	
	IStream *DirectoryFileSystem::OpenForWriting(const char *fn) {
		SPADES_MARK_FUNCTION();
		if(!canWrite){
			SPRaise("Writing prohibited for root path '%s'", rootPath.c_str());
		}
		
		std::string path = physicalPath(fn);
		
		// create required directory
		if(path.find_first_of("/\\") != std::string::npos){
			size_t pos = path.find_first_of("/\\") + 1;
			while(pos < path.size()){
				size_t nextPos = pos;
				while(nextPos < path.size() &&
					  path[nextPos] != '/' &&
					  path[nextPos] != '\\')
					nextPos++;
				if(nextPos == path.size())
					break;
#ifdef WIN32
				mkdir(path.substr(0, nextPos).c_str());
#else
				mkdir(path.substr(0, nextPos).c_str(), 0774);
#endif
				pos = nextPos + 1;
			}
		}
		
		FILE *f = fopen(path.c_str(), "wb+");
		if(f == NULL) {
			SPRaise("I/O error while opening %s for writing", fn);
		}
		return new StdStream(f, true);
	}
	
	bool DirectoryFileSystem::FileExists(const char *fn) {
		SPADES_MARK_FUNCTION();
		
		
		std::string path = physicalPath(fn);
		struct stat s;
		if(stat(path.c_str(), &s)){
			return false;
		}else{
			return true;
		}
	}
	
}
