//
//  ZipFileSystem.cpp
//  OpenSpades
//
//  Created by yvt on 8/8/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "ZipFileSystem.h"
#include "../unzip/unzip.h"
#include "../unzip/ioapi.h"
#include "../Core/Debug.h"
#include "../Core/Exception.h"
#include "IStream.h"
#include <string.h>
#include "DynamicMemoryStream.h"

namespace spades {
	class ZipFileSystem::ZipFileInputStream: public IStream {
		ZipFileSystem *fs;
		unzFile zip;
		uint64_t pos;
		
		bool streaming;
		
		// unzip library doesn't support opening multiple
		// file, but ZipFileSystem has to.
		// so when second file is open, the contents of
		// first file is read and is closed in unzip level.
		// read contents are stored in remainingData.
		// streaming becomes false.
		std::vector<unsigned char> remainingData;
		size_t remDataPos;
	public:
		
		ZipFileInputStream(ZipFileSystem *fs, unzFile zf):
		streaming(true),zip(zf),fs(fs){
			SPADES_MARK_FUNCTION();
		}
		
		virtual ~ZipFileInputStream(){
			SPADES_MARK_FUNCTION();
			if(streaming){
				unzCloseCurrentFile(zip);
				SPAssert(this == fs->currentStream);
				fs->currentStream = NULL;
			}
		}
		
		void ForceCloseUnzipFile() {
			SPADES_MARK_FUNCTION();
			if(streaming){
				streaming = false;
				unsigned char buf[1024];
				int bytes;
				while((bytes = unzReadCurrentFile(zip, buf, sizeof(buf))) > 0) {
					remainingData.insert(remainingData.end(), buf, buf+bytes);
				}
				remDataPos = 0;
				
				int ret = unzCloseCurrentFile(zip);
				if(ret != UNZ_OK) {
					SPRaise("Zip file close error: 0x%08x", ret);
				}
				
				SPAssert(this == fs->currentStream);
				fs->currentStream = NULL;
			}
		}
		
		virtual int ReadByte(){
			SPADES_MARK_FUNCTION();
			int byte;
			if(streaming) {
				unsigned char b;
				int bytes = unzReadCurrentFile(zip, &b, 1);
				if(bytes < 0){
					SPRaise("Unzip error: 0x%08x", bytes);
				}else if(bytes == 0){
					byte = -1;
				}else{
					byte = b;
					pos++;
				}
			}else{
				if(remDataPos >= remainingData.size()){
					return -1;
				}else{
					pos++;
					byte = remainingData[remDataPos];
					remDataPos++;
				}
			}
			return byte;
		}
		virtual size_t Read(void *buf, size_t bytes){
			SPADES_MARK_FUNCTION();
			size_t outBytes;
			if(streaming){
				int out = unzReadCurrentFile(zip, buf, bytes);
				if(out < 0){
					SPRaise("Unzip error: 0x%08x", bytes);
				}
				outBytes = (size_t)out;
			}else{
				outBytes = bytes;
				size_t rem = remainingData.size() - remDataPos;
				if(outBytes > rem)
					outBytes = rem;
				memcpy(buf, remainingData.data() + remDataPos, outBytes);
				
				remDataPos += outBytes;
			}
			
			pos += outBytes;
			return outBytes;
		}
		
		virtual void WriteByte(int){
			SPADES_MARK_FUNCTION();
			SPNotImplemented();
		}
		virtual void Write(const void *, size_t bytes){
			SPADES_MARK_FUNCTION();
			SPNotImplemented();
		}
		virtual void Write(const std::string&){
			SPADES_MARK_FUNCTION();
			SPNotImplemented();
		}
		
		virtual uint64_t GetPosition(){
			return pos;
		}
		virtual void SetPosition(uint64_t){
			SPADES_MARK_FUNCTION();
			SPNotImplemented();
		}
		
		virtual uint64_t GetLength(){
			SPADES_MARK_FUNCTION();
			ForceCloseUnzipFile();
			return pos + remainingData.size() - remDataPos;
		}
		virtual void SetLength(uint64_t){
			SPADES_MARK_FUNCTION();
			SPNotImplemented();
		}
	};
	
#pragma mark - Interface between ZipFileSystem and unzip
	
	class ZipFileSystem::ZipFileHandle {
		ZipFileSystem *fs;
		uint64_t pos;
		
		void ValidateCursor() {
			SPADES_MARK_FUNCTION();
			if(fs->cursorPos != pos) {
				fs->cursorPos = pos;
				fs->baseStream->SetPosition(pos);
			}
		}
	public:
		ZipFileHandle(ZipFileSystem *fs):fs(fs){
			pos = 0;
		}
		
		uint32_t Read(void *buf, uint32_t size) {
			SPADES_MARK_FUNCTION();
			ValidateCursor();
			size_t ret = fs->baseStream->Read(buf, size);
			pos += ret;
			fs->cursorPos += ret;
			return (uint32_t)ret;
		}
		
		uint32_t Write(void *buf, uint32_t size) {
			SPADES_MARK_FUNCTION();
			ValidateCursor();
			try{
				fs->baseStream->Write(buf, size);
			}catch(...){
				// how many bytes written?
				size_t written = fs->baseStream->GetPosition() - pos;
				fs->cursorPos += written;
				pos += written;
				return pos;
			}
			pos += size;
			fs->cursorPos += size;
			return (uint32_t)size;
		}
		
		long Tell() {
			return (long)pos;
		}
		
		long Seek(int32_t offset, int origin) {
			switch(origin){
				case ZLIB_FILEFUNC_SEEK_CUR:
					break;
				case ZLIB_FILEFUNC_SEEK_END:
					pos = fs->baseStream->GetLength();
					break;
				case ZLIB_FILEFUNC_SEEK_SET:
					pos = 0;
					break;
			}
			pos += offset;
			return 0; // (long)pos;
		}
		
		int TestError() {
			return 0; // ??
		}
	};
	
	ZipFileSystem::ZipFileHandle *ZipFileSystem::InternalOpen(spades::ZipFileSystem *fs,
															  const char *fn, int mode) {
		SPADES_MARK_FUNCTION();
		return new ZipFileHandle(fs);
	}
	
	uint32_t ZipFileSystem::InternalRead(spades::ZipFileSystem *fs,
										 spades::ZipFileSystem::ZipFileHandle *h,
										 void *buf,
										 uint32_t size) {
		SPADES_MARK_FUNCTION_DEBUG();
		return h->Read(buf, size);
	}
	
	uint32_t ZipFileSystem::InternalWrite(spades::ZipFileSystem *fs,
										  spades::ZipFileSystem::ZipFileHandle *h,
										  void *buf,
										  uint32_t size) {
		SPADES_MARK_FUNCTION_DEBUG();
		return h->Write(buf, size);
	}
	
	long ZipFileSystem::InternalTell(spades::ZipFileSystem *fs,
									 spades::ZipFileSystem::ZipFileHandle *h) {
		SPADES_MARK_FUNCTION_DEBUG();
		return h->Tell();
	}
	
	long ZipFileSystem::InternalSeek(spades::ZipFileSystem *fs,
									 spades::ZipFileSystem::ZipFileHandle *h,
									 int32_t offset,
									 int origin) {
		SPADES_MARK_FUNCTION_DEBUG();
		return h->Seek(offset, origin);
	}
	
	int ZipFileSystem::InternalClose(spades::ZipFileSystem *fs,
									 spades::ZipFileSystem::ZipFileHandle *h) {
		SPADES_MARK_FUNCTION();
		delete h;
		return 0;
	}
	
	int ZipFileSystem::InternalTestError(spades::ZipFileSystem *fs,
										 spades::ZipFileSystem::ZipFileHandle *h) {
		SPADES_MARK_FUNCTION_DEBUG();
		return h->TestError();
	}
	
	zlib_filefunc_def ZipFileSystem::CreateZLibFileFunc() {
		zlib_filefunc_def def;
		def.opaque = this;
		def.zopen_file = (open_file_func)InternalOpen;
		def.zclose_file = (close_file_func)InternalClose;
		def.zread_file = (read_file_func)InternalRead;
		def.zwrite_file = (write_file_func)InternalWrite;
		def.ztell_file = (tell_file_func)InternalTell;
		def.zseek_file = (seek_file_func)InternalSeek;
		def.zerror_file = (testerror_file_func)InternalTestError;
		return def;
	}
	
#pragma mark - Zip file
	
	ZipFileSystem::ZipFileSystem(IStream *stream, bool autoClose):
	baseStream(stream), autoClose(autoClose){
		SPADES_MARK_FUNCTION();
		
		cursorPos = stream->GetPosition();
		zlib_filefunc_def def = CreateZLibFileFunc();
		zip = unzOpen2("ZipFile.zip", &def);
		if(!zip){
			SPRaise("Failed to open ZIP stream.");
		}
		
		currentStream = NULL;
	}
	
	ZipFileSystem::~ZipFileSystem() {
		SPADES_MARK_FUNCTION();
		if(currentStream){
			currentStream->ForceCloseUnzipFile();
		}
		
		unzClose(zip);
	}
	
	IStream *ZipFileSystem::OpenForReading(const char *fn) {
		SPADES_MARK_FUNCTION();
		
		if(currentStream){
			currentStream->ForceCloseUnzipFile();
		}
		
		if(unzLocateFile(zip, fn, 2) != UNZ_OK){
			SPFileNotFound(fn);
		}
		
		unzOpenCurrentFile(zip);
		try{
			currentStream = new ZipFileInputStream(this, zip);
#if 0
			return currentStream;
#else
			// load all data to allow seeking
			DynamicMemoryStream *stream;
			stream = new DynamicMemoryStream();
			try{
				char buf[4096];
				size_t rd;
				while((rd = currentStream->Read(buf, 4096)) > 0) {
					stream->Write(buf, rd);
				}
				currentStream->ForceCloseUnzipFile();
				stream->SetPosition(0);
				delete currentStream;
				return stream;
			}catch(...){
				delete stream;
				delete currentStream;
				throw;
			}
#endif
		}catch(...){
			unzCloseCurrentFile(zip);
			throw;
		}
	}
	
	IStream *ZipFileSystem::OpenForWriting(const char *fn){
		SPADES_MARK_FUNCTION();
		SPRaise("ZIP file system doesn't support writing");
	}
	
	std::vector<std::string> ZipFileSystem::EnumFiles(const char *path) {
		// TODO: implement
		return std::vector<std::string> ();
	}
	
	bool ZipFileSystem::FileExists(const char *fn) {
		SPADES_MARK_FUNCTION();
		
		if(currentStream){
			currentStream->ForceCloseUnzipFile();
		}
		
		if(unzLocateFile(zip, fn, 2) != UNZ_OK){
			return false;
		}
		return true;
	}
}

