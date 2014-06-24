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
#include <Core/Strings.h>

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
		SPADES_MARK_FUNCTION();
		
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
	
	std::string FileManager::ResolvePath(const std::string &path,
										 const std::string &basePath,
										 bool secure) {
		SPADES_MARK_FUNCTION();
		if (basePath.empty() || basePath[basePath.size() - 1] != '/') {
			SPRaise("Base path should end with '/'");
		}
		
		if (secure) {
			// validate characters
			if (path.size() > 256) {
				SPRaise("Insecure path (too long)");
			}
			for (auto c: path) {
				if (c >= 'a' && c <= 'z') continue;
				if (c >= 'A' && c <= 'Z') continue;
				if (c >= '0' && c <= '9') continue;
				if (c == '/' || c == '_' || c == '.' ||
					c == ' ') continue;
				SPRaise("Insecure path (disallowed character 0x%02x): %s", c, path.c_str());
			}
			if (path[0] == '/') {
				SPRaise("Insecure path (absolute path): %s", path.c_str());
			}
			
		}
		
		auto parts = Split(basePath, "/");
		auto exParts = Split(path, "/");
		//parts.pop_back();
		
		auto initSize = parts.size();
		
		for (auto it = exParts.begin(); it != exParts.end(); ++it) {
			const auto& s = *it;
			if (s.empty() && it == exParts.begin()) {
				if (secure) {
					SPRaise("Insecure path (absolute path): %s", path.c_str());
				} else {
					parts.clear(); parts.push_back("");
				}
			} else if (s == ".") {
				continue;
			} else if (s == "..") {
				parts.pop_back();
				if (secure && parts.size() < initSize) {
					SPRaise("Insecure path (escaping from the base directory): %s", path.c_str());
				}
				if (parts.empty()) {
					SPRaise("Cannot go outside the root directory: %s "
							"(basepath = '%s')", path.c_str(), basePath.c_str());
				}
			} else {
				if (s.empty() && parts.back().empty()) {
					continue;
				}
				if (!s.empty() && s[0] == '.' && secure) {
					SPRaise("Insecure path (dotfile): %s", path.c_str());
				}
				parts.push_back(s);
			}
		}
		
		// make path
		std::string s = parts[0];
		SPAssert(!parts.empty());
		for (std::size_t i = 1; i < parts.size(); ++i)
			s += "/" + parts[i];
		return s;
	}
	
	std::string FileManager::CombinePath(const std::string &path1,
										 const std::string &path2) {
		if (path2.empty()) return path1;
		if (path2[0] == '/') return path2;
		if (path1.empty()) return path2;
		if (path1.back() == '/') {
			return path1 + path2;
		} else {
			return path1 + "/" + path2;
		}
	}
	
	std::string FileManager::GetPathWithoutFileName(const std::string &path) {
		auto idx = path.rfind('/');
		if (idx == std::string::npos) {
			return "";
		} else {
			return path.substr(0, idx + 1);
		}
	}
	
}

