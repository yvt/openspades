//
//  FileManager.cpp
//  OpenSpades
//
//  Created by yvt on 7/12/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "FileManager.h"
#include "IFileSystem.h"
#include <vector>
#include "Exception.h"
#include "IStream.h"
#include "Debug.h"
#include <set>

namespace spades {
	static std::vector<IFileSystem *> g_fileSystems;
	IStream *FileManager::OpenForReading(const char *fn) {
		SPADES_MARK_FUNCTION();
		
		for(size_t i = 0; i < g_fileSystems.size(); i++){
			IFileSystem *fs = g_fileSystems[i];
			if(fs->FileExists(fn))
				return fs->OpenForReading(fn);
		}
		SPFileNotFound(fn);
	}
	IStream *FileManager::OpenForWriting(const char *fn) {
		SPADES_MARK_FUNCTION();
		
		for(size_t i = 0; i < g_fileSystems.size(); i++){
			IFileSystem *fs = g_fileSystems[i];
			if(fs->FileExists(fn))
				return fs->OpenForWriting(fn);
		}
		
		// create file
		for(size_t i = 0; i < g_fileSystems.size(); i++){
			IFileSystem *fs = g_fileSystems[i];
			try{
				return fs->OpenForWriting(fn);
			}catch(...){
			}
		}
		
		SPRaise("No filesystem is writable");
	}
	bool FileManager::FileExists(const char *fn) {
		SPADES_MARK_FUNCTION();
		
		for(size_t i = 0; i < g_fileSystems.size(); i++){
			IFileSystem *fs = g_fileSystems[i];
			if(fs->FileExists(fn))
				return true;
		}
		return false;
	}
	
	void FileManager::AddFileSystem(spades::IFileSystem *fs){
		SPADES_MARK_FUNCTION();
		
		g_fileSystems.push_back(fs);
	}
	
	std::string FileManager::ReadAllBytes(const char *fn) {
		SPADES_MARK_FUNCTION();
		
		IStream *stream = OpenForReading(fn);
		try{
			std::string ret = stream->ReadAllBytes();
			delete stream;
			return ret;
		}catch(...){
			delete stream;
			throw;
		}
	}
	
	std::vector<std::string> FileManager::EnumFiles(const char *path) {
		std::vector<std::string> list;
		std::set<std::string> set;
		
		for(size_t i = 0; i < g_fileSystems.size(); i++){
			IFileSystem *fs = g_fileSystems[i];
			std::vector<std::string> l = fs->EnumFiles(path);
			for(size_t i = 0; i < l.size(); i++)
				set.insert(l[i]);
		}
		
		for(std::set<std::string>::iterator it =
			set.begin(); it != set.end(); it++)
			list.push_back(*it);
		
		return list;
	}
	
}

