//
//  IBitmapCodec.cpp
//  OpenSpades
//
//  Created by yvt on 7/22/13.
//  Copyright (c) 2013 yvt.jp. All rights reserved.
//

#include "IBitmapCodec.h"
#include "Debug.h"
#include "Exception.h"
#include <ctype.h>

namespace spades {
	
	static std::vector<IBitmapCodec *> *allCodecs = NULL;
	
	static void InitCodecList() {
		if(allCodecs)
			return;
		allCodecs = new std::vector<IBitmapCodec *>();
	}
	
	std::vector<IBitmapCodec *> IBitmapCodec::GetAllCodecs() {
		return *allCodecs;
	}
	
	bool IBitmapCodec::EndsWith(const std::string &filename,
								const std::string &extension){
		SPADES_MARK_FUNCTION_DEBUG();
		
		if(filename.size() < extension.size())
			return false;
		for(size_t i = 0; i < extension.size(); i++){
			int a = tolower(extension[i]);
			int b = tolower(filename[i + filename.size() - extension.size()]);
			if(a != b)
				return false;
		}
		return true;
	}

	IBitmapCodec::IBitmapCodec(){
		SPADES_MARK_FUNCTION();
		
		InitCodecList();
		allCodecs->push_back(this);
	}
	
	IBitmapCodec::~IBitmapCodec() {
		SPADES_MARK_FUNCTION();
		// FIXME: uninstall bitmap codec?
	}
}
