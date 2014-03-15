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

#include "FileManager.h"
#include "IFileSystem.h"
#include <list>
#include "Exception.h"
#include "IStream.h"
#include "Debug.h"
#include <set>

namespace spades {
	static std::list<IFileSystem *> g_fileSystems;
	IStream *FileManager::OpenForReading(const char *fn) {
		SPADES_MARK_FUNCTION();
		if(!fn) SPInvalidArgument("fn");
		if(fn[0] == 0) SPFileNotFound(fn);
		for(auto *fs: g_fileSystems){
			if(fs->FileExists(fn))
				return fs->OpenForReading(fn);
		}
		SPFileNotFound(fn);
	}
	IStream *FileManager::OpenForWriting(const char *fn) {
		SPADES_MARK_FUNCTION();
		if(!fn) SPInvalidArgument("fn");
		if(fn[0] == 0) SPFileNotFound(fn);
		for(auto *fs: g_fileSystems){
			if(fs->FileExists(fn))
				return fs->OpenForWriting(fn);
		}
		
		// create file
		for(auto *fs: g_fileSystems){
			try{
				return fs->OpenForWriting(fn);
			}catch(...){
			}
		}
		
		SPRaise("No filesystem is writable");
	}
	bool FileManager::FileExists(const char *fn) {
		SPADES_MARK_FUNCTION();
		if(!fn) SPInvalidArgument("fn");
		
		for(auto *fs: g_fileSystems){
			if(fs->FileExists(fn))
				return true;
		}
		return false;
	}
	
	void FileManager::AddFileSystem(spades::IFileSystem *fs){
		SPADES_MARK_FUNCTION();
		AppendFileSystem(fs);
	}
	
	
	void FileManager::AppendFileSystem(spades::IFileSystem *fs){
		SPADES_MARK_FUNCTION();
		if(!fs) SPInvalidArgument("fs");
		
		g_fileSystems.push_back(fs);
	}
	void FileManager::PrependFileSystem(spades::IFileSystem *fs){
		SPADES_MARK_FUNCTION();
		if(!fs) SPInvalidArgument("fs");
		
		g_fileSystems.push_front(fs);
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
		if(!path) SPInvalidArgument("path");
		
		for(auto *fs: g_fileSystems){
			std::vector<std::string> l = fs->EnumFiles(path);
			for(size_t i = 0; i < l.size(); i++)
				set.insert(l[i]);
		}
		
		for(auto& s: set)
			list.push_back(s);
		
		return list;
	}
	
}

